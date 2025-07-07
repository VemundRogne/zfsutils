#pragma once

#include <csignal>
#include <exception>
#include <libzfs.h>
#include <stdexcept>

#include <functional>
#include <vector>

#include <iostream>

#include "zfsutils/core.hpp"

namespace zfs {

class Snapshot {
  public:
    Snapshot(zfs_handle_t *handle) { handle_ = handle; };
    ~Snapshot() { closeZfsHandleIfOwned(); };

    // Delete copy-constructor
    Snapshot(const Snapshot &) = delete;
    // Delete copy-assignment operator
    Snapshot &operator=(const Snapshot &) = delete;

    // Move constructor
    Snapshot(Snapshot &&other) noexcept {
        handle_ = other.handle_;
        other.handle_ = nullptr;
    }

    Snapshot &operator=(Snapshot &&other) noexcept {
        if (this != &other) {
            closeZfsHandleIfOwned();

            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    std::string fullName(void) {
        assertHandle();

        return std::string{zfs_get_name(handle_)};
    };

    std::string name(void) {
        assertHandle();

        std::string fullName{zfs_get_name(handle_)};
        size_t delim_pos = fullName.find("@");
        return std::string{&fullName[delim_pos] + 1,
                           fullName.length() - 1 - delim_pos};
    }

    zfs_handle_t *getHandle() {
        assertHandle();
        return handle_;
    };

  private:
    zfs_handle_t *handle_;

    void assertHandle() {
        if (handle_ == nullptr) {
            throw std::logic_error("Dataset handle is invalid!");
        }
    }

    void closeZfsHandleIfOwned() {
        if (handle_ != nullptr) {
            zfs_close(handle_);
        }
    }
};

class Dataset {
  public:
    Dataset(zfs_handle_t *handle) {
        handle_ = handle;
        // std::cout << "Opened handle to Dataset: " << name() << std::endl;
    };
    ~Dataset() { closeZfsHandleIfOwned(); }

    // Delete copy-constructor
    Dataset(const Dataset &) = delete;
    // Delete copy assignment operator
    Dataset &operator=(const Dataset &) = delete;

    // Move constructor
    Dataset(Dataset &&other) noexcept {
        handle_ = other.handle_;
        other.handle_ = nullptr;
    }

    // Move assignment operator
    Dataset &operator=(Dataset &&other) noexcept {
        if (this != &other) {
            // We have to close a handle if we happen to own one
            closeZfsHandleIfOwned();

            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    void setMountpoint(std::string mountPoint) {
        int retval =
            zfs_prop_set(getHandle(), "mountpoint", mountPoint.c_str());
        if (retval != 0) {
            throw std::logic_error{"Could not set mountpoint for dataset '" +
                                   name() + "' to '" + mountPoint + "'"};
        }
    }

    std::string getMountpoint() {
        char mountpoint[512]{0};
        int retval = zfs_prop_get(getHandle(), ZFS_PROP_MOUNTPOINT,
                                  &mountpoint[0], 512, NULL, 0, 0, B_FALSE);
        if (retval != 0) {
            throw std::logic_error{"Could not get mountpoint for dataset '" +
                                   name() + "'"};
        }

        return std::string{mountpoint};
    }

    std::vector<Snapshot> get_snapshots(void) {
        assertHandle();

        std::vector<Snapshot> snaps;
        zfsutils::internal::IterHelper iterHelper;

        iterHelper.callback = [&snaps](zfs_handle_t *zh) -> int {
            snaps.push_back(Snapshot{zh});

            // Note that zfs_iter_snapshots_sorted_v2 does not care about this
            // return-value
            //
            // the non-sorted variant _does care_
            return 0;
        };

        zfs_iter_snapshots_sorted_v2(handle_, 0, iterHelper.zfs_callback,
                                     &iterHelper, 0, 0);
        return snaps;
    }

    Snapshot openSnapshot(std::string snapshotName) {
        auto &zfsHandle = zfsutils::internal::LibzfsHandle::instance();

        std::string fullSnapName = name() + "@" + snapshotName;

        zfs_handle_t *zh =
            zfs_open(zfsHandle.get(), fullSnapName.c_str(), ZFS_TYPE_SNAPSHOT);
        if (!zh) {
            throw std::invalid_argument{"Snapshot '" + fullSnapName +
                                        "' does not exist"};
        }

        return Snapshot{zh};
    }

    Snapshot createSnapshot(std::string snapshotName) {
        auto &zfsHandle = zfsutils::internal::LibzfsHandle::instance();

        // First try to open a snapshot with that name
        try {
            Snapshot snap = openSnapshot(snapshotName);

            throw std::logic_error{"snapshot '" + snapshotName +
                                   "' already exists!"};

        } catch (std::invalid_argument &e) {
            // Do nothing on this catch; we expect this to be thrown
        }

        std::string fullSnapName = name() + "@" + snapshotName;

        int retval = zfs_snapshot(zfsHandle.get(), fullSnapName.c_str(),
                                  B_FALSE, nullptr);
        if (retval != 0) {
            throw std::logic_error{"Could not create snapshot '" +
                                   fullSnapName + "'"};
        }

        return openSnapshot(snapshotName);
    }

    std::vector<Dataset> get_children(void) {
        assertHandle();
        std::vector<Dataset> children;

        zfsutils::internal::IterHelper iterHelper;
        iterHelper.callback = [&children](zfs_handle_t *zh) -> int {
            /* Can either do this: */
            // Dataset child{zh};
            // children.push_back(std::move(child));

            /* Or this: */
            children.emplace_back(Dataset{zh});

            /* Or this: */
            // children.push_back(Dataset{zh});
            return 0;
        };
        zfs_iter_filesystems_v2(handle_, 0, iterHelper.zfs_callback,
                                &iterHelper);

        return children;
    }

    void print_dependents() {
        std::cout << "Printing dependents of " << name() << std::endl;
        zfsutils::internal::IterHelper iterHelper;
        iterHelper.callback = [](zfs_handle_t *zh) -> int {
            std::cout << zfs_get_name(zh) << std::endl;
            zfs_close(zh);
            return 0;
        };

        zfs_iter_dependents_v2(handle_, 0, B_TRUE, iterHelper.zfs_callback,
                               &iterHelper);
    }

    std::string name(void) {
        assertHandle();

        return std::string{zfs_get_name(handle_)};
    };

    zfs_handle_t *getHandle() {
        assertHandle();
        return handle_;
    };

  private:
    zfs_handle_t *handle_ = nullptr;
    void closeZfsHandleIfOwned() {
        if (handle_ != nullptr) {
            // std::cout << "Closing dataset handle: " << name() << std::endl;
            zfs_close(handle_);
        }
    }

    /* Throw logic_error if we do not have a handle
     * Functions that use the handle should call this before doing anything
     * */
    void assertHandle() {
        if (handle_ == nullptr) {
            throw std::logic_error("Dataset handle is invalid!");
        }
    }
};

class ZFS {
  public:
    static Dataset getDatasetByName(std::string name) {
        auto &zfsHandle = zfsutils::internal::LibzfsHandle::instance();
        zfs_handle_t *zh =
            zfs_open(zfsHandle.get(), name.c_str(), ZFS_TYPE_FILESYSTEM);
        if (!zh) {
            std::cout << "Failed to open dataset" << std::endl;
            throw std::invalid_argument{"Dataset of that name does not exist"};
        }
        return Dataset{zh};
    }
};

} // namespace zfs
