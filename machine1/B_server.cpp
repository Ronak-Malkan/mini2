#include <iostream>
#include <fstream>
#include <chrono>
#include <grpcpp/grpcpp.h>
#include "overlay.grpc.pb.h"
#include <nlohmann/json.hpp>
#include "vectorized_dataset.h"   // Include the vectorized dataset header
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
VectorizedDataSet dataset;  // Global instance for local vectorized data

class OverlayServiceImpl final : public OverlayComm::Service {
public:
    // The PushData function now acts as a query handler.
    // It parses the payload as the injury threshold (an integer).
    // Then it performs a local search and (if configured) forwards the query to downstream server D.
    Status PushData(ServerContext* context, const OverlayRequest* request, OverlayAck* reply) override {
        // Begin timing the search
        auto t_start = std::chrono::steady_clock::now();

        int threshold = std::stoi(request->payload());
        auto indices = dataset.searchByInjuryCountParallel(threshold);
        int local_result = indices.size();
        std::cout << "B: Local search found " << local_result << " matching records." << std::endl;

        int downstream_result = 0;
        // If there is a next hop configured (for B, expect "D")
        if (!next_hops.empty()) {
            std::string target = next_hops[0];  // For B, it should be "D"
            std::string address = target;
            auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
            std::unique_ptr<OverlayComm::Stub> stub = OverlayComm::NewStub(channel);

            OverlayRequest fwd_request;
            fwd_request.set_origin("B");
            fwd_request.set_payload(request->payload());  // Forward the same threshold

            OverlayAck ack;
            grpc::ClientContext ctx;
            Status status = stub->PushData(&ctx, fwd_request, &ack);
            if (status.ok()) {
                downstream_result = std::stoi(ack.status());
                std::cout << "B: Received " << downstream_result << " from downstream D." << std::endl;
            } else {
                std::cerr << "B: Failed to get result from D." << std::endl;
            }
        }

        int total = local_result + downstream_result;
        auto t_end = std::chrono::steady_clock::now();
        std::chrono::duration<double> search_time = t_end - t_start;
        std::cout << "B: Total aggregated result = " << total << " (search time: " << search_time.count() << " seconds)" << std::endl;

        reply->set_status(std::to_string(total));
        return Status::OK;
    }
};

void loadConfig() {
    std::ifstream in("../config/overlay_config.json");
    json config;
    in >> config;
    next_hops = config["B"].get<std::vector<std::string>>();
}

// Function to load the dataset partition for Server B.
void loadDataset() {
    // Set the desired number of threads for loading (for example, 4 for Device1)
    omp_set_num_threads(4);

    string dataFile = "../data/dataset.csv";
    size_t total = VectorizedDataSet::countLines(dataFile);
    if(total == 0) {
        std::cerr << "B: Failed to count lines in " << dataFile << std::endl;
        return;
    }
    // total lines in data (excluding header)
    size_t quarter = total / 4;
    size_t start = 0;     // For Server B: first quarter
    size_t count = quarter;

    std::cout << "B: Total records = " << total << ", loading first quarter (" << count << " records)." << std::endl;
    auto t1 = std::chrono::steady_clock::now();
    if(dataset.loadFromFileRange(dataFile, start, count))
    {
        auto t2 = std::chrono::steady_clock::now();
        std::chrono::duration<double> dt = t2 - t1;
        std::cout << "B: Dataset loaded (" << dataset.number_of_persons_injured.size() 
                  << " records) in " << dt.count() << " seconds." << std::endl;
    } else {
        std::cerr << "B: Error loading dataset." << std::endl;
    }
}

void RunServer() {
    std::string server_address("0.0.0.0:50052");
    OverlayServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server B listening on " << server_address << std::endl;
    server->Wait();
}

int main() {
    loadConfig();
    loadDataset();  // Load the data partition before starting the service.
    RunServer();
    return 0;
}
