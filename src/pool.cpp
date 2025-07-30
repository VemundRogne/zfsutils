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

std::string Pool::getProp(PoolProperty property) const {
    char buffer[512]{0};

    zpool_prop_t prop;

    switch (property) {
    case zfsutils::PoolProperty::size:
        prop = ZPOOL_PROP_SIZE;
        break;

    case zfsutils::PoolProperty::capacity:
        prop = ZPOOL_PROP_CAPACITY;
        break;

    case zfsutils::PoolProperty::altroot:
        prop = ZPOOL_PROP_ALTROOT;
        break;

    case zfsutils::PoolProperty::health:
        prop = ZPOOL_PROP_HEALTH;
        break;

    case zfsutils::PoolProperty::version:
        prop = ZPOOL_PROP_VERSION;
        break;

    case zfsutils::PoolProperty::free:
        prop = ZPOOL_PROP_FREE;
        break;

    case zfsutils::PoolProperty::allocated:
        prop = ZPOOL_PROP_ALLOCATED;
        break;

    case zfsutils::PoolProperty::fragmentation:
        prop = ZPOOL_PROP_FRAGMENTATION;
        break;
    }

    zpool_get_prop(getHandle(), prop, &buffer[0], sizeof(buffer), 0, B_FALSE);
    return std::string{buffer};
}

Dataset Pool::createDataset(std::string name) {
    return Dataset::create(this->name() + "/" + name);
}

std::optional<Dataset> Pool::openDataset(std::string name) {
    return Dataset::open(this->name() + "/" + name);
}

} // namespace zfsutils
