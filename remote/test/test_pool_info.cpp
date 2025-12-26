// Goal is to print info about pools on this system, and on other systems
// (through gRPC)
//
#include <iostream>

#include "pool.grpc.pb.h"
#include "pool.pb.h"
#include <grpcpp/grpcpp.h>

#include "zfsutils/pool.hpp"

#include "zfsutils/remote/pool.hpp"

int main() {
    using namespace zfsutils::remote;

    std::cout << "Local pools: " << std::endl;
    for (auto &pool : zfsutils::Pool::getPools()) {
        pool.printPoolInfo();
    }

    std::cout << "gRPC pools: " << std::endl;

    auto client = std::make_shared<PoolInterfaceClient>(
        PoolInterfaceClient{grpc::CreateChannel(
            "localhost:50000", grpc::InsecureChannelCredentials())});

    std::vector<RemotePool> remotePools;

    auto poolInfo = client->ListPools();
    for (auto &pool : poolInfo) {
        remotePools.push_back(RemotePool{client, pool.name, pool.hostname});
    }

    for (auto &pool : remotePools) {
        pool.printPoolInfo();
    }

    zfsutils::Pool myPool = zfsutils::Pool::open("testpool");
    std::shared_ptr<zfsutils::Pool> myPoolShared =
        std::make_shared<zfsutils::Pool>(std::move(myPool));
    std::vector<std::shared_ptr<zfsutils::interface::IPool>> mixedPools{};
    mixedPools.push_back(myPoolShared);
    std::shared_ptr<RemotePool> myRemotePoolShared =
        std::make_shared<RemotePool>(remotePools[0]);
    mixedPools.push_back(myRemotePoolShared);

    std::cout << "name: " << mixedPools[0]->name() << std::endl;
    for (std::shared_ptr<zfsutils::interface::IPool> pool : mixedPools) {
        pool->printPoolInfo();
    }
}
