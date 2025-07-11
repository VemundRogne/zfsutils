#pragma once

#include "zfsutils/core.hpp"
#include "zfsutils/dataset.hpp"

#include <optional>

namespace zfsutils {

class Pool : private internal::HandleHelper<zpool_handle_t, zpool_close> {
    using Base = internal::HandleHelper<zpool_handle_t, zpool_close>;

  private:
    Pool(zpool_handle_t *handle) : Base{handle} {}

  public:
    using Base::hasHandle;

    /* Escape-hatch */
    using Base::getHandle;

    static Pool open(std::string name);
    static std::vector<Pool> getPools();

    std::string name() const;

    Dataset createDataset(std::string name);
    std::optional<Dataset> openDataset(std::string name);
};

} // namespace zfsutils
