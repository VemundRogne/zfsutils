// Goal is to print info about pools on this system, and on other systems
// (through gRPC)

#include "zfsutils/pool.hpp"

#include <grpcpp/grpcpp.h>

#include "remotezfs.grpc.pb.h"
#include "remotezfs.pb.h"

class RemotePoolService final : public remotezfs::RemotePool::Service {
    grpc::Status ListPools(grpc::ServerContext *context,
                           const remotezfs::ListPoolsRequest *request,
                           remotezfs::ListPoolsReply *reply) {
        std::cout << "ListPools called" << std::endl;

        std::vector<zfsutils::Pool> localPools = zfsutils::Pool::getPools();

        for (auto &localPool : localPools) {
            auto *pool = reply->add_pools();
            pool->set_name(localPool.name());
        }

        return grpc::Status::OK;
    }
};

class RemoteDatasetService final : public remotezfs::RemoteDataset::Service {
    grpc::Status ListDatasets(grpc::ServerContext *context,
                              const remotezfs::ListDatasetsRequest *request,
                              remotezfs::ListDatasetsReply *reply) {
        auto targetTopLevelDataset = zfsutils::Dataset::open(request->base());

        for (auto &dataset : targetTopLevelDataset->getDatasets()) {
            reply->add_datasets()->set_name(dataset.name());
        }

        return grpc::Status::OK;
    }
};

int main() {
    std::string server_address{"0.0.0.0:50000"};

    RemotePoolService remotePoolService;
    RemoteDatasetService remoteDatasetService;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&remotePoolService);
    builder.RegisterService(&remoteDatasetService);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    server->Wait();

    return 0;
}
