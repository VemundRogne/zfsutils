// Goal is to print info about pools on this system, and on other systems
// (through gRPC)
//
#include <iostream>

#include "pool.grpc.pb.h"
#include "pool.pb.h"
#include <grpcpp/grpcpp.h>

#include "zfsutils/pool.hpp"

struct RemotePoolInfo {
    std::string name;
    std::string hostname;
};

class PoolInterfaceClient {
  public:
    PoolInterfaceClient(std::shared_ptr<grpc::Channel> channel)
        : stub_(PoolInterface::NewStub(channel)) {}

    std::vector<RemotePoolInfo> ListPools() {
        ListPoolsRequest request;
        request.set_nmax(0);

        Pool reply;

        grpc::ClientContext context;
        std::unique_ptr<grpc::ClientReader<Pool>> reader(
            stub_->ListPools(&context, request));

        std::vector<RemotePoolInfo> remotePools;

        while (reader->Read(&reply)) {
            remotePools.push_back(
                RemotePoolInfo{reply.name(), reply.hostname()});
        }
        return remotePools;
    }

  private:
    std::unique_ptr<PoolInterface::Stub> stub_;
};

class RemotePool : public zfsutils::interface::IPool {
  public:
    RemotePool(std::shared_ptr<PoolInterfaceClient> poolInterfaceClient,
               std::string name, std::string hostname)
        : poolInterfaceClient{poolInterfaceClient}, name_{name},
          hostname_{hostname} {}

    std::string name() const override { return name_; }
    std::string getHostname() const override { return hostname_; }
    std::string
    getProp(zfsutils::interface::PoolProperty property) const override {
        return "NOT IMPLEMENTED";
    }

  private:
    std::shared_ptr<PoolInterfaceClient> poolInterfaceClient;

    std::string name_;
    std::string hostname_;
};

void printPoolInfo(zfsutils::interface::IPool &pool) {
    std::cout << " --- POOL --- " << std::endl;
    std::cout << "  '" << pool.name() << "'on " << pool.getHostname()
              << std::endl;
    std::cout << "      size: "
              << pool.getProp(zfsutils::interface::PoolProperty::size)
              << std::endl;
    std::cout << "  capacity: "
              << pool.getProp(zfsutils::interface::PoolProperty::capacity)
              << std::endl;
}

int main() {
    std::cout << "Local pools: " << std::endl;
    for (auto &pool : zfsutils::Pool::getPools()) {
        printPoolInfo(pool);
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
        printPoolInfo(pool);
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
        printPoolInfo(*pool);
    }
}
