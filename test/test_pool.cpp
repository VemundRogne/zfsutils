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

        std::cout << testpool_A.name() << " size: "
                  << testpool_A.getProp(zfsutils::PoolProperty::size)
                  << std::endl;

        assert(testpool_A.getProp(zfsutils::PoolProperty::health) == "ONLINE");

        assert(testpool_A.hasHandle() == true);
        usePool(std::move(testpool_A));
        assert(testpool_A.hasHandle() == false);

    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

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
