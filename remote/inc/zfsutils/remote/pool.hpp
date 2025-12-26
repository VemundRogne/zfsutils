#pragma once

#include "pool.grpc.pb.h"
#include "pool.pb.h"
#include "zfsutils/interface/pool.hpp"
#include <grpcpp/grpcpp.h>

namespace zfsutils {
namespace remote {

struct RemotePoolInfo {
    std::string name;
    std::string hostname;
};

class PoolInterfaceClient {
  public:
    PoolInterfaceClient(std::shared_ptr<grpc::Channel> channel)
        : stub_(remotezfs::v1::PoolInterface::NewStub(channel)) {}

    std::vector<RemotePoolInfo> ListPools() {
        remotezfs::v1::ListPoolsRequest request;

        remotezfs::v1::Pool reply;

        grpc::ClientContext context;
        std::unique_ptr<grpc::ClientReader<remotezfs::v1::Pool>> reader(
            stub_->ListPools(&context, request));

        std::vector<RemotePoolInfo> remotePools;

        while (reader->Read(&reply)) {
            remotePools.push_back(
                RemotePoolInfo{reply.name(), reply.hostname()});
        }
        return remotePools;
    }

  private:
    std::unique_ptr<remotezfs::v1::PoolInterface::Stub> stub_;
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

} // namespace remote

} // namespace zfsutils
