#pragma once

#include "zfsutils/core.hpp"
#include "zfsutils/dataset.hpp"
#include "zfsutils/interface/pool.hpp"

#include <optional>

#include <limits.h>
#include <unistd.h>

namespace zfsutils {


class Pool : public interface::IPool,
             private internal::HandleHelper<zpool_handle_t, zpool_close> {
    using Base = internal::HandleHelper<zpool_handle_t, zpool_close>;

  private:
    Pool(zpool_handle_t *handle) : Base{handle} {}

  public:
    using Base::hasHandle;

    /* Escape-hatch */
    using Base::getHandle;

    static Pool open(std::string name);
    static std::vector<Pool> getPools();

    std::string getHostname() const override {
        char hostname[HOST_NAME_MAX];
        if (gethostname(hostname, HOST_NAME_MAX) == 0) {
            return std::string{hostname};
        }
        throw std::runtime_error{"Pool::getHostname failed..."};
    };

    std::string name() const override;

    Dataset createDataset(std::string name);
    std::optional<Dataset> openDataset(std::string name);

    std::string getProp(interface::PoolProperty property) const override;
};

} // namespace zfsutils
