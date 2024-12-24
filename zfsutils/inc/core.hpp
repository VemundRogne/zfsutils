#pragma once

#include <csignal>
#include <libzfs.h>
#include <stdexcept>

#include <iostream>
#include <vector>

namespace zfs {

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
    ~Pool() {
        std::cout << "Pool closing handle" << handle_ << std::endl;
        zpool_close(handle_);
    };

    // Deprecating copy-operator
    // If we can copy, then we might be able to store a pointer to an invalid
    // pool object.
    [[deprecated("You should not use copy-operator of the Pool handle")]] Pool(
        const Pool &) = default;

    std::string name(void) {
        if (handle_ != nullptr) {

            return std::string{zpool_get_name(handle_)};
        }

        throw std::logic_error{"You tried to get the name of a zfs::Pool with "
                               "no handle (the handle must have been deleted, "
                               "which only happens if deletePool is called)"};
    };

    // If we need to delete the pool we might get away with simply invalidating
    // this handle (and ensuring that all accesses respect this handle !=
    // nullptr)
    void deletePool() { handle_ = nullptr; };

    zpool_handle_t *handle_;

  private:
}; // namespace zfs

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

    Pool getPoolByName(std::string name) {
        zpool_handle_t *zh = zpool_open(handle_, name.c_str());
        if (!zh) {
            throw std::invalid_argument{"Pool of that name does not exist"};
        }
        return Pool{zh};
    }

    /* I want this in the constructor, but it seems I can not provide a lambda
     * iterator that has acces to the pools-vector... So now the user must call
     * this init...
     * */
    [[deprecated]] void initPoolHandles(void) {
        if (poolHandlesInitDone) {
            throw std::logic_error{"Can't init pool handles multiple times"};
        }
        poolHandlesInitDone = true;

        // Zpool access is with iterators with specific
        auto zpool_iter_callback = [](zpool_handle_t *zhp, void *data) -> int {
            // ZFS gives some data; I do now know what it contains
            (void)data;

            zfs::ZFSHandle &zfsHandle = zfs::ZFSHandle::instance();
            zfsHandle.pools.push_back(Pool{zhp});

            // Returning 0 just keeps the iterator running
            return 0;
        };
        zpool_iter(handle_, zpool_iter_callback, nullptr);
    }

    [[deprecated]] std::vector<Pool> pools;

  private:
    libzfs_handle_t *handle_;

    [[deprecated]] bool poolHandlesInitDone{false};

    ZFSHandle() : handle_(libzfs_init()) {
        if (!handle_) {
            throw std::runtime_error("Failed to intitialize libzfs handle");
        }
    }
};

} // namespace zfs
