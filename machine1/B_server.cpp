#include <iostream>
#include <fstream>
#include <grpcpp/grpcpp.h>
#include "overlay.grpc.pb.h"
#include "shared_memory.hpp"
#include <nlohmann/json.hpp>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using overlay::OverlayComm;
using overlay::OverlayRequest;
using overlay::OverlayAck;

using json = nlohmann::json;

std::vector<std::string> next_hops;

class OverlayServiceImpl final : public OverlayComm::Service {
public:
    Status PushData(ServerContext* context, const OverlayRequest* request, OverlayAck* reply) override {
        std::string data = request->payload();
        std::cout << "B received from " << request->origin() << ": " << data << std::endl;

        for (const auto& target : next_hops) {
            std::string address = target + ":50052";
            auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
            std::unique_ptr<OverlayComm::Stub> stub = OverlayComm::NewStub(channel);

            OverlayRequest fwd;
            fwd.set_origin("B");
            fwd.set_payload(data);

            OverlayAck ack;
            grpc::ClientContext ctx;

            Status status = stub->PushData(&ctx, fwd, &ack);
            if (status.ok()) {
                std::cout << "B → " << target << " OK\n";
            } else {
                std::cerr << "B → " << target << " FAILED\n";
            }
        }

        reply->set_status("B forwarded");
        return Status::OK;
    }
};

void loadConfig() {
    std::ifstream in("../config/overlay_config.json");
    json config;
    in >> config;
    next_hops = config["B"].get<std::vector<std::string>>();
}

void RunServer() {
    std::string server_address("0.0.0.0:50052");
    OverlayServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Node B listening on " << server_address << std::endl;
    server->Wait();
}

int main() {
    loadConfig();
    RunServer();
    return 0;
}
