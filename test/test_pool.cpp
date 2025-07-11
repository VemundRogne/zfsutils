#include "libzfs.h"
#include "zfsutils/core.hpp"
#include "zfsutils/pool.hpp"
#include <iostream>
#include <ranges>

void usePool(zfsutils::Pool pool) {
    std::cout << "I have pool: " << pool.name() << std::endl;
}

int main() {
    try {
        zfsutils::Pool testpool_A = zfsutils::Pool::open("zfsutils_testpool_A");
        assert(testpool_A.name() == "zfsutils_testpool_A");

        assert(testpool_A.hasHandle() == true);
        usePool(std::move(testpool_A));
        assert(testpool_A.hasHandle() == false);

    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    // Try to iterate top-level datasets:
    zfsutils::internal::IterHelper<zfs_handle_t> iterHelper;
    iterHelper.callback = [](zfs_handle_t *zh) -> int {
        std::cout << std::string{zfs_get_name(zh)} << std::endl;
        return 0;
    };
    zfs_iter_root(zfsutils::internal::LibzfsHandle::Handle(),
                  iterHelper.zfs_callback, &iterHelper);

    auto pools = zfsutils::Pool::getPools();

    auto pool_in_vector = [](const std::vector<zfsutils::Pool> &pools,
                             const std::string &name) {
        return std::find_if(pools.begin(), pools.end(),
                            [&](const zfsutils::Pool &pool) {
                                return pool.name() == name;
                            }) != pools.end();
    };

    if (pool_in_vector(pools, "zfsutils_testpool_A") != true) {
        throw std::runtime_error{"zfsutils_testpool_A not found :("};
    }

    if (pool_in_vector(pools, "zfsutils_testpool_B") != true) {
        throw std::runtime_error{"zfsutils_testpool_B not found :("};
    }
}
