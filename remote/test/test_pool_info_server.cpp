// Goal is to print info about pools on this system, and on other systems
// (through gRPC)

#include "zfsutils/pool.hpp"

#include <grpcpp/grpcpp.h>

#include "pool.grpc.pb.h"
#include "pool.pb.h"

class PoolInterfaceServiceImpl final
    : public remotezfs::v1::PoolInterface::Service {
    grpc::Status ListPools(grpc::ServerContext *context,
                           const remotezfs::v1::ListPoolsRequest *request,
                           remotezfs::v1::ListPoolsReply *reply) {
        std::cout << "ListPools called" << std::endl;

        std::vector<zfsutils::Pool> localPools = zfsutils::Pool::getPools();

        for (auto &localPool : localPools) {
            auto *pool = reply->add_pools();
            pool->set_name(localPool.name());
        }

        return grpc::Status::OK;
    }

    grpc::Status ListDatasets(grpc::ServerContext *context,
                              const remotezfs::v1::ListDatasetsRequest *request,
                              remotezfs::v1::ListDatasetsReply *reply) {
        auto targetTopLevelDataset =
            zfsutils::Dataset::open(request->target_pool().name());

        for (auto &dataset : targetTopLevelDataset->getDatasets()) {
            reply->add_datasets()->set_name(dataset.name());
        }

        return grpc::Status::OK;
    }
};

int main() {
    std::string server_address{"0.0.0.0:50000"};

    PoolInterfaceServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    server->Wait();

    return 0;
}
