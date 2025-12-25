// Goal is to print info about pools on this system, and on other systems
// (through gRPC)

#include "zfsutils/pool.hpp"

#include <grpcpp/grpcpp.h>

#include "pool.grpc.pb.h"
#include "pool.pb.h"

class PoolInterfaceServiceImpl final : public PoolInterface::Service {
    grpc::Status ListPools(grpc::ServerContext *context,
                           const ListPoolsRequest *request,
                           grpc::ServerWriter<Pool> *writer) {
        std::cout << "ListPools called" << std::endl;

        std::vector<zfsutils::Pool> pools = zfsutils::Pool::getPools();

        Pool reply;
        for (auto &pool : pools) {
            reply.set_name(pool.name());
            reply.set_hostname(pool.getHostname());
            writer->Write(reply);
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
