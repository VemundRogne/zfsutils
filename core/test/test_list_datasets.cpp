#include "zfsutils/dataset.hpp"
#include "zfsutils/pool.hpp"

#include <iostream>
#include <vector>

int main() {
    auto toplevel = zfsutils::Dataset::open("testpool");

    if (toplevel.has_value()) {
        for (auto &dataset : toplevel->getDatasets()) {
            std::cout << " - " << dataset.name() << std::endl;
            for (auto &dataset_ : dataset.getDatasets()) {
                std::cout << " -- " << dataset_.name() << std::endl;
            }
        }
    }
}
