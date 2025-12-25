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
};

} // namespace interface
} // namespace zfsutils
