#include <iostream>
#include <fstream>
#include <grpcpp/grpcpp.h>
#include "data.grpc.pb.h"
#include "overlay.grpc.pb.h"
#include "shared_memory.hpp"
#include <nlohmann/json.hpp>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using dataportal::DataPortal;
using dataportal::DataRequest;
using dataportal::Ack;

using overlay::OverlayComm;
using overlay::OverlayRequest;
using overlay::OverlayAck;

using json = nlohmann::json;

std::vector<std::string> next_hops;

class DataServiceImpl final : public DataPortal::Service {
public:
    Status SendData(ServerContext* context, const DataRequest* request, Ack* reply) override {
        std::string incoming_data = request->payload();
        std::cout << "A received from client: " << incoming_data << std::endl;

        // Forward to next hop(s)
        for (const auto& target : next_hops) {
            std::string address = target + ":50052";  // assuming all others use this port
            auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
            std::unique_ptr<OverlayComm::Stub> stub = OverlayComm::NewStub(channel);

            OverlayRequest fwd_request;
            fwd_request.set_origin("A");
            fwd_request.set_payload(incoming_data);

            OverlayAck ack;
            grpc::ClientContext ctx;

            Status status = stub->PushData(&ctx, fwd_request, &ack);
            if (status.ok()) {
                std::cout << "A → " << target << " OK\n";
            } else {
                std::cerr << "A → " << target << " FAILED\n";
            }
        }

        reply->set_message("Data forwarded by A");
        return Status::OK;
    }
};

void loadConfig() {
    std::ifstream in("../config/overlay_config.json");
    json config;
    in >> config;
    next_hops = config["A"].get<std::vector<std::string>>();
}

void RunServer() {
    std::string server_address("0.0.0.0:50051");
    DataServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Node A listening on " << server_address << std::endl;
    server->Wait();
}

int main() {
    loadConfig();
    RunServer();
    return 0;
}
