#pragma once

#include "zfsutils/interface/dataset.hpp"

namespace zfsutils {
namespace interface {

class ISnapshot {
  public:
    virtual std::string name() const = 0;
};

} // namespace interface
} // namespace zfsutils
