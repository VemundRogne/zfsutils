#include "zfsutils/snapshot.hpp"

#include <string>

namespace zfsutils {

std::string Snapshot::fullName() const {
    return std::string{zfs_get_name(getHandle())};
}

std::string Snapshot::name() const {
    std::string fullName = this->fullName();

    size_t delim_pos = fullName.find("@");

    return std::string{&fullName[delim_pos] + 1,
                       fullName.length() - 1 - delim_pos};
}

std::optional<Snapshot> Snapshot::open(std::string fullpath) {
    zfs_handle_t *zh = zfs_open(zfsutils::internal::LibzfsHandle::Handle(),
                                fullpath.c_str(), ZFS_TYPE_SNAPSHOT);

    if (!zh) {
        return {};
    }
    return Snapshot{zh};
}

Snapshot Snapshot::create(std::string fullpath) {
    if (Snapshot::open(fullpath).has_value()) {
        throw std::logic_error{"Snapshot already exists!"};
    }

    int retval = zfs_snapshot(internal::LibzfsHandle::Handle(),
                              fullpath.c_str(), B_FALSE, nullptr);

    if (retval != 0) {
        throw std::runtime_error{"Could not create snapshot '" + fullpath +
                                 "'"};
    }

    return Snapshot::open(fullpath).value();
}

} // namespace zfsutils
