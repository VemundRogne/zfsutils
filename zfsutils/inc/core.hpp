#pragma once

#include <csignal>
#include <exception>
#include <libzfs.h>
#include <stdexcept>

#include <functional>
#include <vector>

#include <iostream>

namespace zfs {

/**
 * Singleton for the ZFS core handle
 */
class ZFSHandle {
  public:
    static ZFSHandle &instance() {
        static ZFSHandle instance;
        return instance;
    }

    ZFSHandle(const ZFSHandle &) = delete;
    ZFSHandle &operator=(const ZFSHandle &) = delete;

    libzfs_handle_t *get() const { return handle_; }

    ~ZFSHandle() {
        if (handle_) {
            std::cout << "Destroying libzfs handle!" << std::endl;
            libzfs_fini(handle_);
            handle_ = nullptr;
        }
    }

    libzfs_handle_t *handle_;

  private:
    ZFSHandle() : handle_(libzfs_init()) {
        if (!handle_) {
            throw std::runtime_error("Failed to intitialize libzfs handle");
        }
    }
};

/*
 * IterHelper is a class to make iterating with zfs easier
 *
 * The problem we need to solve is how to iterate _in the context of a class_
 * This is not possible with just a lambda directly into the zfs-iterator
 *
 * The solution is to use the (void* data) context variable in the zfs_iterators
 *
 * General usage:
 *  IterHelper iterHelper;
 *  iterHelper.callback = [&some_captured_variable](zfs_handle_t *zh) -> int {
 *      // do something
 *      // Either close or keep zfs_handle_t
 *      return 0 if you want to keep iterating, 1 if you are done iterating
 *  }
 *  zfs_iter_filesystems_v2(some_zfs_handle_t, 0, iterHelper.zfs_callback,
 *                          &iterhelper);
 */
class IterHelper {
  public:
    // This is the callback that zfs should use.
    // It converts the context passed through the iterator into the instance of
    // the IterHelper and then calls its registered callback
    static int zfs_callback(zfs_handle_t *zh, void *context) {
        auto *self = static_cast<IterHelper *>(context);
        if (self->callback) {
            return self->callback(zh);
        } else {
            // Stop iterating if we do not have a callback
            return 1;
        }
    }

    // And this is the callback back to my context (typically a lambda in a
    // class)
    std::function<int(zfs_handle_t *zh)> callback;
};

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
        IterHelper iterHelper;

        iterHelper.callback = [&snaps](zfs_handle_t *zh) -> int {
            snaps.push_back(Snapshot{zh});

            // stop at once.
            return 1;
        };

        zfs_iter_snapshots_sorted_v2(handle_, 0, iterHelper.zfs_callback,
                                     &iterHelper, 0, 0);
        return snaps;
    }

    Snapshot openSnapshot(std::string snapshotName) {
        zfs::ZFSHandle &zfsHandle = zfs::ZFSHandle::instance();

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
        zfs::ZFSHandle &zfsHandle = zfs::ZFSHandle::instance();

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

        IterHelper iterHelper;
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
        IterHelper iterHelper;
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

/**
 * Handler for the zpool_handle
 */
class Pool {
  public:
    // Constructor just stores a pointer to the handle
    // I think lifetime is for the ZFS filesystem
    Pool(zpool_handle_t *handle) {
        handle_ = handle;
        std::cout << "Pool construct" << std::endl;
    };

    // Destructor should close the handle _if_ we own it
    ~Pool() {
        // Use a function here, since we need this in the move assignment
        // operator
        closeZpoolIfOwned();
    };

    // Delete the copy constructor -- we only want move
    Pool(const Pool &) = delete;
    // Delete the copy assignment operator
    Pool &operator=(const Pool &) = delete;

    // Move-constructor
    //   The original object is made unusable
    //   The new object owns the data
    Pool(Pool &&other) noexcept {
        std::cout << "Pool move" << std::endl;
        handle_ = other.handle_;
        other.handle_ = nullptr;
    }

    // Move assignment operator
    Pool &operator=(Pool &&other) noexcept {
        if (this != &other) {
            // If we _do_ own some object we must free it
            closeZpoolIfOwned();

            handle_ = other.handle_;
            other.handle_ = nullptr;
            std::cout << "Move assignment operator called" << std::endl;
        }
        return *this;
    }

    std::string name(void) {
        assertHandle();

        return std::string{zpool_get_name(handle_)};
    };

    void createDataset(std::string name) {
        nvlist_t *props;
        nvlist_alloc(&props, NV_UNIQUE_NAME, 0);
        std::string dataset_name = this->name() + "/" + name;

        std::cout << "Trying to make dataset " << dataset_name << std::endl;

        zfs::ZFSHandle &zfsHandle = zfs::ZFSHandle::instance();

        int retval = zfs_create(zfsHandle.get(), dataset_name.c_str(),
                                ZFS_TYPE_FILESYSTEM, props);
        if (retval != 0) {
            throw std::logic_error{"Could not create dataset with name '" +
                                   name + "' in pool '" + this->name() + "'"};
        }
    }

    Dataset openDataset(std::string datasetName) {
        zfs::ZFSHandle &zfsHandle = zfs::ZFSHandle::instance();

        std::string fullDatasetName = name() + "/" + datasetName;

        zfs_handle_t *zh = zfs_open(zfsHandle.get(), fullDatasetName.c_str(),
                                    ZFS_TYPE_FILESYSTEM);
        if (!zh) {
            std::cout << "Failed to open dataset" << std::endl;
            throw std::invalid_argument{"Dataset of that name does not exist"};
        }
        return Dataset{zh};
    }

    zpool_handle_t *handle_ = nullptr;

  private:
    /* Throw logic_error if we do not have a handle
     * Functions that use the handle should call this before doing anything
     * */
    void assertHandle() {
        if (handle_ == nullptr) {
            throw std::logic_error(
                "Pool pointer is invalid -- maybe you copied the Pool object?");
        }
    }

    void closeZpoolIfOwned() {
        if (handle_ != nullptr) {
            std::cout << "Pool closing handle" << handle_ << std::endl;
            zpool_close(handle_);
        } else {
            std::cout << "Pool did not close handle" << std::endl;
        }
    }
}; // namespace zfs

class ZFS {
  public:
    static Pool getPoolByName(std::string name) {
        zfs::ZFSHandle &zfsHandle = zfs::ZFSHandle::instance();

        zpool_handle_t *zh = zpool_open(zfsHandle.get(), name.c_str());
        if (!zh) {
            throw std::invalid_argument{"Pool of that name does not exist"};
        }
        return Pool{zh};
    }

    static Dataset getDatasetByName(std::string name) {
        zfs::ZFSHandle &zfsHandle = zfs::ZFSHandle::instance();
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
