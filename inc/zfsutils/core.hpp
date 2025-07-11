#pragma once

#include <functional>
#include <memory>
#include <stdexcept>

#include <libzfs.h>

namespace zfsutils {

namespace internal {

/* Helper template for handles */
template <typename T, void (*closer)(T *handle)> class HandleHelper {
    std::unique_ptr<T, decltype(closer)> handle;

  public:
    HandleHelper(T *raw_handle) : handle{raw_handle, closer} {};

    bool hasHandle() {
        if (handle) {
            return true;
        }
        return false;
    }

    void assertHandle() {
        if (!handle) {
            throw std::logic_error(
                "zfsutils::internal::rawHandle assertHandle failed!");
        }
    }

    T *getHandle() {
        assertHandle();
        return handle.get();
    }
};

/* libzfs_handle singleton */
class LibzfsHandle {
  private:
    libzfs_handle_t *libzfs_handle{nullptr};

    LibzfsHandle() : libzfs_handle(libzfs_init()) {
        if (!libzfs_handle) {
            throw std::runtime_error("Failed to initialize libzfs");
        }
    }

    ~LibzfsHandle() {
        if (libzfs_handle) {
            libzfs_fini(libzfs_handle);
            libzfs_handle = nullptr;
        }
    }

    LibzfsHandle(const LibzfsHandle &) = delete;
    LibzfsHandle &operator=(const LibzfsHandle &) = delete;

  public:
    static LibzfsHandle &instance() {
        static LibzfsHandle libzfsHandle{};

        return libzfsHandle;
    }

    libzfs_handle_t *get() const { return libzfs_handle; }

    static libzfs_handle_t *Handle() { return LibzfsHandle::instance().get(); }
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
 *  IterHelper<zfs_handle_t> iterHelper;
 *  iterHelper.callback = [&some_captured_variable](zfs_handle_t *zh) -> int {
 *      // do something
 *      // Either close or keep zfs_handle_t
 *      return 0 if you want to keep iterating, 1 if you are done iterating
 *  }
 *  zfs_iter_filesystems_v2(some_zfs_handle_t, 0, iterHelper.zfs_callback,
 *                          &iterhelper);
 */
template <typename T> class IterHelper {
  public:
    // This is the callback that zfs should use.
    // It converts the context passed through the iterator into the instance of
    // the IterHelper and then calls its registered callback
    static int zfs_callback(T *zh, void *context) {
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
    std::function<int(T *zh)> callback;
};

} // namespace internal

} // namespace zfsutils
