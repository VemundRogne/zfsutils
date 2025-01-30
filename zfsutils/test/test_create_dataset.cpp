#include "core.hpp"
#include "libzfs.h"

int main() {
    try {
        zfs::Pool testpool_A = zfs::ZFS::getPoolByName("zfsutils_testpool_A");
        int retval = testpool_A.createDataset("testDataset");
        if (retval != 0) {
            return -1;
        }
    } catch (std::logic_error &e) {
        std::cout << "Caught logic_error!" << std::endl;
        std::cout << e.what() << std::endl;
        return -1;
    } catch (std::exception &e) {
        std::cout << "Caught some exception!" << std::endl;
        std::cout << e.what() << std::endl;
        return -1;
    }

    std::cout << "I think I made a dataset? :)" << std::endl;
    return 0;
}
