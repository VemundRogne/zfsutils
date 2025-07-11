#include "zfsutils/pool.hpp"

namespace zfsutils {

Pool Pool::open(std::string name) {
    zpool_handle_t *zh =
        zpool_open(zfsutils::internal::LibzfsHandle::Handle(), name.c_str());
    if (!zh) {
        throw std::invalid_argument{"Pool of that name does not exist"};
    }
    return Pool{zh};
}

std::vector<Pool> Pool::getPools() {
    std::vector<Pool> pools;

    zfsutils::internal::IterHelper<zpool_handle_t> iterHelper;
    iterHelper.callback = [&pools](zpool_handle_t *zh) -> int {
        pools.push_back(Pool{zh});

        return 0;
    };

    zpool_iter(zfsutils::internal::LibzfsHandle::Handle(),
               iterHelper.zfs_callback, &iterHelper);

    return pools;
}

std::string Pool::name() const {
    return std::string{zpool_get_name(getHandle())};
};

Dataset Pool::createDataset(std::string name) {
    return Dataset::create(this->name() + "/" + name);
}

std::optional<Dataset> Pool::openDataset(std::string name) {
    return Dataset::open(this->name() + "/" + name);
}

} // namespace zfsutils
