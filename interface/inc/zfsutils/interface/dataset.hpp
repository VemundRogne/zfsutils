#pragma once

#include "zfsutils/interface/pool.hpp"

namespace zfsutils {
namespace interface {

enum DatasetProperty {
    type,
    creation,
    used,
    available,
    referenced,
    compressratio,
    mounted,
    origin,
    quota
};

class IDataset {
  public:
    virtual std::string name() const = 0;
};

} // namespace interface
} // namespace zfsutils
