#pragma once

#include "zfsutils/core.hpp"
#include "zfsutils/interface/dataset.hpp"
#include "zfsutils/snapshot.hpp"

#include <optional>

namespace zfsutils {

class Dataset : public zfsutils::interface::IDataset,
                private internal::HandleHelper<zfs_handle_t, zfs_close> {
    using Base = internal::HandleHelper<zfs_handle_t, zfs_close>;

  public:
    Dataset(zfs_handle_t *handle) : Base{handle} {}

    using Base::hasHandle;

    /* Escape-hatch */
    using Base::getHandle;

    std::string name() const override;

    static std::vector<Dataset> getTopLevelDatasets();
    std::vector<Dataset> getDatasets();

    static std::optional<Dataset> open(std::string path);
    static Dataset create(std::string path);

    void setMountpoint(std::string mountPoint);
    std::string getMountpoint();

    zfsutils::Snapshot createSnapshot(std::string name);
    std::optional<zfsutils::Snapshot> openSnapshot(std::string name);

    std::vector<zfsutils::Snapshot> getSnapshots();

    std::string getProp(zfsutils::interface::DatasetProperty property);
};

} // namespace zfsutils
