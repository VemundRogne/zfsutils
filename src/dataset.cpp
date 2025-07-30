#include "zfsutils/dataset.hpp"

namespace zfsutils {

std::string Dataset::name() { return std::string{zfs_get_name(getHandle())}; }

std::optional<Dataset> Dataset::open(std::string path) {
    zfs_handle_t *zh = zfs_open(internal::LibzfsHandle::Handle(), path.c_str(),
                                ZFS_TYPE_FILESYSTEM);
    if (!zh) {
        return {};
    }
    return Dataset{zh};
}

std::vector<Dataset> Dataset::getTopLevelDatasets() {
    std::vector<Dataset> datasets;

    zfsutils::internal::IterHelper<zfs_handle_t> iterHelper;
    iterHelper.callback = [&datasets](zfs_handle_t *zh) -> int {
        datasets.push_back(Dataset{zh});
        return 0;
    };

    zfs_iter_root(zfsutils::internal::LibzfsHandle::Handle(),
                  iterHelper.zfs_callback, &iterHelper);

    return datasets;
}

Dataset Dataset::create(std::string path) {
    nvlist_t *props;
    nvlist_alloc(&props, NV_UNIQUE_NAME, 0);

    // std::cout << "Trying to make dataset " << path << std::endl;

    auto &zfsHandle = zfsutils::internal::LibzfsHandle::instance();

    int retval =
        zfs_create(zfsHandle.get(), path.c_str(), ZFS_TYPE_FILESYSTEM, props);
    if (retval != 0) {
        throw std::logic_error{"Could not create dataset with name '" + path +
                               "'"};
    }
    return open(path).value();
}

void Dataset::setMountpoint(std::string mountPoint) {
    int retval = zfs_prop_set(getHandle(), "mountpoint", mountPoint.c_str());
    if (retval != 0) {
        throw std::logic_error{"Could not set mountpoint for dataset '" +
                               name() + "' to '" + mountPoint + "'"};
    }
}

std::string Dataset::getMountpoint() {
    char mountpoint[512]{0};
    int retval = zfs_prop_get(getHandle(), ZFS_PROP_MOUNTPOINT, &mountpoint[0],
                              512, NULL, 0, 0, B_FALSE);
    if (retval != 0) {
        throw std::logic_error{"Could not get mountpoint for dataset '" +
                               name() + "'"};
    }

    return std::string{mountpoint};
}

Snapshot Dataset::createSnapshot(std::string name) {
    return Snapshot::create(this->name() + "@" + name);
}

std::optional<Snapshot> Dataset::openSnapshot(std::string name) {
    return Snapshot::open(this->name() + "@" + name);
}

std::vector<Snapshot> Dataset::getSnapshots() {
    std::vector<Snapshot> snapshots;

    internal::IterHelper<zfs_handle_t> iterHelper;
    iterHelper.callback = [&snapshots](zfs_handle_t *zh) -> int {
        snapshots.push_back(Snapshot{zh});
        // Note that zfs_iter_snapshots_sorted_v2 does not care about this
        // return-value
        //
        // the non-sorted variant _does care_
        return 0;
    };

    zfs_iter_snapshots_sorted_v2(getHandle(), 0, iterHelper.zfs_callback,
                                 &iterHelper, 0, 0);

    return snapshots;
}

} // namespace zfsutils
