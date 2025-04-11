#include <iostream>
#include <fstream>
#include <chrono>
#include <grpcpp/grpcpp.h>
#include "overlay.grpc.pb.h"
#include <nlohmann/json.hpp>
#include "vectorized_dataset.h"
#include <omp.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using overlay::OverlayComm;
using overlay::OverlayRequest;
using overlay::OverlayAck;

using json = nlohmann::json;

std::vector<std::string> next_hops;
VectorizedDataSet dataset;  // Local dataset for C

class OverlayServiceImpl final : public OverlayComm::Service {
public:
    Status PushData(ServerContext* context, const OverlayRequest* request, OverlayAck* reply) override {
        auto t_start = std::chrono::steady_clock::now();

        int threshold = std::stoi(request->payload());
        auto indices = dataset.searchByInjuryCountParallel(threshold);
        int local_result = indices.size();
        std::cout << "C: Local search found " << local_result << " matching records." << std::endl;

        int downstream_result = 0;
        if (!next_hops.empty()) {
            std::string target = next_hops[0]; // For C, should be "E"
            std::string address = target;
            auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
            std::unique_ptr<OverlayComm::Stub> stub = OverlayComm::NewStub(channel);

            OverlayRequest fwd_request;
            fwd_request.set_origin("C");
            fwd_request.set_payload(request->payload());

            OverlayAck ack;
            grpc::ClientContext ctx;
            Status status = stub->PushData(&ctx, fwd_request, &ack);
            if (status.ok()) {
                downstream_result = std::stoi(ack.status());
                std::cout << "C: Received " << downstream_result << " from downstream E." << std::endl;
            } else {
                std::cerr << "C: Failed to get result from E." << std::endl;
            }
        }
        int total = local_result + downstream_result;
        auto t_end = std::chrono::steady_clock::now();
        std::chrono::duration<double> search_time = t_end - t_start;
        std::cout << "C: Total aggregated result = " << total << " (search time: " 
                  << search_time.count() << " seconds)" << std::endl;

        reply->set_status(std::to_string(total));
        return Status::OK;
    }
};

void loadConfig() {
    std::ifstream in("../config/overlay_config.json");
    json config;
    in >> config;
    next_hops = config["C"].get<std::vector<std::string>>();
}

void loadDataset() {
    omp_set_num_threads(3);
    string dataFile = "../data/dataset.csv";
    size_t total = VectorizedDataSet::countLines(dataFile);
    if(total == 0) {
        std::cerr << "C: Failed to count lines." << std::endl;
        return;
    }
    size_t quarter = total / 4;
    size_t start = quarter * 1;  // Second quarter for C
    size_t count = quarter;
    std::cout << "C: Total records = " << total << ", loading second quarter (" << count << " records)." << std::endl;
    auto t1 = std::chrono::steady_clock::now();
    if(dataset.loadFromFileRange(dataFile, start, count))
    {
        auto t2 = std::chrono::steady_clock::now();
        std::chrono::duration<double> dt = t2 - t1;
        std::cout << "C: Dataset loaded (" << dataset.number_of_persons_injured.size() 
                  << " records) in " << dt.count() << " seconds." << std::endl;
    } else {
        std::cerr << "C: Error loading dataset." << std::endl;
    }
}

void RunServer() {
    std::string server_address("0.0.0.0:50053");
    OverlayServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server C listening on " << server_address << std::endl;
    server->Wait();
}

int main() {
    loadConfig();
    loadDataset();
    RunServer();
    return 0;
}
