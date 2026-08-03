#pragma once
#include <iostream>
#include <string>

namespace zfsutils {
namespace interface {

enum PoolProperty {
    size,
    capacity,
    altroot,
    health,
    version,
    free,
    allocated,
    fragmentation
};

class IPool {
  public:
    virtual std::string name() const = 0;
    virtual std::string getProp(PoolProperty property) const = 0;
    virtual std::string getHostname() const = 0;

    void printPoolInfo() {
        std::cout << " --- POOL --- " << std::endl;
        std::cout << "  '" << name() << "' on " << getHostname() << std::endl;

        std::cout << "    size: " << getProp(PoolProperty::size) << std::endl;

        std::cout << "    capacity: "
                  << getProp(zfsutils::interface::PoolProperty::capacity)
                  << std::endl;
    }
};

} // namespace interface
} // namespace zfsutils
