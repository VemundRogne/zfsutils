#pragma once

#include <optional>

#include "zfsutils/core.hpp"

namespace zfsutils {

class Snapshot : private internal::HandleHelper<zfs_handle_t, zfs_close> {
    using Base = internal::HandleHelper<zfs_handle_t, zfs_close>;
    friend class Dataset;

  private:
    Snapshot(zfs_handle_t *handle) : Base{handle} {}

  public:
    using Base::hasHandle;

    /* Escape-hatch */
    using Base::getHandle;

    static std::optional<Snapshot> open(std::string fullpath);
    static Snapshot create(std::string fullpath);

    std::string fullName();
    std::string name();
};

} // namespace zfsutils
