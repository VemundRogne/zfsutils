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
        assertPointer();

        return std::string{zpool_get_name(handle_)};
    };

    zpool_handle_t *handle_ = nullptr;

  private:
    /* Throw logic_error if we do not have a handle
     * Functions that use the handle should call this before doing anything
     * */
    void assertPointer() {
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

  private:
    libzfs_handle_t *handle_;

    ZFSHandle() : handle_(libzfs_init()) {
        if (!handle_) {
            throw std::runtime_error("Failed to intitialize libzfs handle");
        }
    }
};

} // namespace zfs
