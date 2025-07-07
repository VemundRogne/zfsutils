#include "zfsutils/pool.hpp"
#include <iostream>

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
}
