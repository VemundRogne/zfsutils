#pragma once

#include "pool.grpc.pb.h"
#include "pool.pb.h"
#include "zfsutils/interface/dataset.hpp"
#include "zfsutils/interface/pool.hpp"
#include <grpcpp/grpcpp.h>

namespace zfsutils {
namespace remote {

struct RemotePoolInfo {
    std::string name;
};

struct RemoteDatasetInfo {
    std::string name;
};

class PoolInterfaceClient {
  public:
    PoolInterfaceClient(std::shared_ptr<grpc::Channel> channel)
        : channel(channel),
          stub_(remotezfs::v1::PoolInterface::NewStub(channel)) {}

    std::vector<RemotePoolInfo> ListPools() {
        remotezfs::v1::ListPoolsRequest request;

        remotezfs::v1::ListPoolsReply reply;

        grpc::ClientContext context;
        stub_->ListPools(&context, request, &reply);

        std::vector<RemotePoolInfo> remotePools;

        for (auto &pool : reply.pools()) {
            remotePools.push_back(RemotePoolInfo{pool.name()});
        }
        return remotePools;
    }

    std::vector<RemoteDatasetInfo>
    ListDatasets(zfsutils::interface::IPool &targetPool) {
        std::cout << "Calling listDatasets on " << targetPool.name()
                  << std::endl;
        std::vector<RemoteDatasetInfo> datasets;

        remotezfs::v1::ListDatasetsRequest request;
        request.mutable_target_pool()->set_name(targetPool.name());

        remotezfs::v1::ListDatasetsReply reply;

        grpc::ClientContext context;
        stub_->ListDatasets(&context, request, &reply);

        for (auto &dataset : reply.datasets()) {
            std::cout << "Dataset: " << dataset.name() << std::endl;
        }

        return datasets;
    }

  private:
    // Keep the channel, so outputs can get the channel in their new calls
    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<remotezfs::v1::PoolInterface::Stub> stub_;
};

class RemotePool : public zfsutils::interface::IPool {
  public:
    RemotePool(std::shared_ptr<grpc::Channel> channel, std::string name,
               std::string hostname)
        : poolInterfaceClient{channel}, name_{name}, hostname_{hostname} {}

    std::string name() const override { return name_; }
    std::string getHostname() const override { return hostname_; }
    std::string
    getProp(zfsutils::interface::PoolProperty property) const override {
        return "NOT IMPLEMENTED";
    }

  private:
    PoolInterfaceClient poolInterfaceClient;

    std::string name_;
    std::string hostname_;
};

class RemoteDataset : public zfsutils::interface::IDataset {
  public:
    // A dataset is either top-level or a decendant of a top-level dataset
    // We can just store the full path of each dataset as its name
    RemoteDataset(std::shared_ptr<grpc::Channel> channel,
                  std::string fullPath) {}

  private:
    std::string fullPath; // example: store/datasetA/datasetA1
};

} // namespace remote

} // namespace zfsutils
