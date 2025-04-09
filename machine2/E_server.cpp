#include <iostream>
#include <fstream>
#include <grpcpp/grpcpp.h>
#include "overlay.grpc.pb.h"
#include "shared_memory.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using overlay::OverlayComm;
using overlay::OverlayRequest;
using overlay::OverlayAck;

class OverlayServiceImpl final : public OverlayComm::Service {
public:
    Status PushData(ServerContext* context, const OverlayRequest* request, OverlayAck* reply) override {
        std::string data = request->payload();
        std::cout << "E received from " << request->origin() << ": " << data << std::endl;

        // Sink logic — just store it
        SharedMemory::store("E", data);

        reply->set_status("E stored");
        return Status::OK;
    }
};

void RunServer() {
    std::string server_address("0.0.0.0:50052");
    OverlayServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Node E listening on " << server_address << std::endl;
    server->Wait();
}

int main() {
    RunServer();
    return 0;
}
