#include <iostream>
#include <fstream>
#include <chrono>
#include <grpcpp/grpcpp.h>
#include "data.grpc.pb.h"
#include "overlay.grpc.pb.h"
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

std::vector<std::string> next_hops;  // For A, expect "B" and "C"

class DataServiceImpl final : public DataPortal::Service {
public:
    // When a client sends a query, forward it to servers B and C.
    Status SendData(ServerContext* context, const DataRequest* request, Ack* reply) override {
        auto t_start = std::chrono::steady_clock::now();

        int threshold = std::stoi(request->payload());
        int aggregated_result = 0;

        // For each next hop in A's config (B and C)
        for (const auto& target : next_hops) {
            std::string address = target;
            auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
            std::unique_ptr<OverlayComm::Stub> stub = OverlayComm::NewStub(channel);

            OverlayRequest fwd_request;
            fwd_request.set_origin("A");
            fwd_request.set_payload(request->payload());

            OverlayAck ack;
            grpc::ClientContext ctx;
            Status status = stub->PushData(&ctx, fwd_request, &ack);
            if (status.ok()) {
                int result = std::stoi(ack.status());
                std::cout << "A: Received " << result << " from " << target << std::endl;
                aggregated_result += result;
            } else {
                std::cerr << "A: Failed to communicate with " << target << std::endl;
            }
        }

        auto t_end = std::chrono::steady_clock::now();
        std::chrono::duration<double> total_search_time = t_end - t_start;
        std::cout << "A: Aggregated result = " << aggregated_result 
                  << " (total query search time: " << total_search_time.count() << " seconds)" << std::endl;

        reply->set_message("Total matching records: " + std::to_string(aggregated_result));
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
    std::cout << "Server A listening on " << server_address << std::endl;
    server->Wait();
}

int main() {
    loadConfig();
    RunServer();
    return 0;
}
