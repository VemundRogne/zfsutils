#pragma once

#include "zfsutils/interface/dataset.hpp"
#include "zfsutils/interface/pool.hpp"

#pragma push_macro("verify")
#undef verify
#include "remotezfs.grpc.pb.h"
#include "remotezfs.pb.h"
#include <grpcpp/grpcpp.h>
#pragma pop_macro("verify")

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
        : channel(channel), stub_(remotezfs::RemotePool::NewStub(channel)) {}

    std::vector<RemotePoolInfo> ListPools() {
        remotezfs::ListPoolsRequest request;

        remotezfs::ListPoolsReply reply;

        grpc::ClientContext context;
        stub_->ListPools(&context, request, &reply);

        std::vector<RemotePoolInfo> remotePools;

        for (auto &pool : reply.pools()) {
            remotePools.push_back(RemotePoolInfo{pool.name()});
        }
        return remotePools;
    }

  private:
    // Keep the channel, so outputs can get the channel in their new calls
    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<remotezfs::RemotePool::Stub> stub_;
};

class DatasetInterfaceClient {
  public:
    DatasetInterfaceClient(std::shared_ptr<grpc::Channel> channel)
        : channel(channel), stub_(remotezfs::RemoteDataset::NewStub(channel)) {}

    std::vector<RemoteDatasetInfo>
    ListDatasets(zfsutils::interface::IPool &targetPool) {
        std::cout << "Calling listDatasets on " << targetPool.name()
                  << std::endl;
        std::vector<RemoteDatasetInfo> datasets;

        remotezfs::ListDatasetsRequest request;
        request.set_base(targetPool.name());

        remotezfs::ListDatasetsReply reply;

        grpc::ClientContext context;
        stub_->ListDatasets(&context, request, &reply);

        for (auto &dataset : reply.datasets()) {
            std::cout << "Dataset: " << dataset.name() << std::endl;
        }

        return datasets;
    }

  private:
    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<remotezfs::RemoteDataset::Stub> stub_;
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
