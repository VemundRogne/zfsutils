#pragma once

#include <optional>

#include "zfsutils/core.hpp"
#include "zfsutils/interface/snapshot.hpp"

namespace zfsutils {

class Snapshot : public interface::ISnapshot,
                 private internal::HandleHelper<zfs_handle_t, zfs_close> {
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

    std::string fullName() const;
    std::string name() const;

    /* TODO: Implement properties for Snapshots.
     * I believe Snapshots and Datasets have so much in common that it makes
     * sense to have some inheritance for the properties.
     *
     * Anyway the enum-solution I have now namespace to zfsutils. This does not
     * work for both dataset and snapshot because they have the same properites
     */
};

} // namespace zfsutils
