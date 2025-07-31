#pragma once

#include "zfsutils/core.hpp"
#include "zfsutils/snapshot.hpp"

#include <optional>

namespace zfsutils {
enum DatasetProperty {
    type,
    creation,
    used,
    available,
    referenced,
    compressratio,
    mounted,
    origin,
    quota
};
}

namespace zfsutils {

class Dataset : private internal::HandleHelper<zfs_handle_t, zfs_close> {
    using Base = internal::HandleHelper<zfs_handle_t, zfs_close>;

  private:
    Dataset(zfs_handle_t *handle) : Base{handle} {}

  public:
    using Base::hasHandle;

    /* Escape-hatch */
    using Base::getHandle;

    std::string name() const;

    static std::vector<Dataset> getTopLevelDatasets();

    static std::optional<Dataset> open(std::string path);
    static Dataset create(std::string path);

    void setMountpoint(std::string mountPoint);
    std::string getMountpoint();

    Snapshot createSnapshot(std::string name);
    std::optional<Snapshot> openSnapshot(std::string name);

    std::vector<Snapshot> getSnapshots();

    std::string getProp(zfsutils::DatasetProperty property);
};

} // namespace zfsutils
