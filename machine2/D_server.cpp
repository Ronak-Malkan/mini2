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

VectorizedDataSet dataset;  // Local dataset for D

class OverlayServiceImpl final : public OverlayComm::Service {
public:
    Status PushData(ServerContext* context, const OverlayRequest* request, OverlayAck* reply) override {
        auto t_start = std::chrono::steady_clock::now();

        int threshold = std::stoi(request->payload());
        auto indices = dataset.searchByInjuryCountParallel(threshold);
        int result = indices.size();
        auto t_end = std::chrono::steady_clock::now();
        std::chrono::duration<double> search_time = t_end - t_start;
        std::cout << "D: Found " << result << " matching records (search time: " 
                  << search_time.count() << " seconds)." << std::endl;

        reply->set_status(std::to_string(result));
        return Status::OK;
    }
};

void loadConfig() {
    // D is a leaf so no downstream hosts to forward to.
}

void loadDataset() {
    omp_set_num_threads(3);
    string dataFile = "../data/dataset.csv";
    size_t total = VectorizedDataSet::countLines(dataFile);
    if(total == 0) {
        std::cerr << "D: Failed to count lines." << std::endl;
        return;
    }
    size_t quarter = total / 4;
    size_t start = quarter * 2;   // Third quarter for D
    size_t count = quarter;

    std::cout << "D: Total records = " << total << ", loading third quarter (" << count << " records)." << std::endl;
    auto t1 = std::chrono::steady_clock::now();
    if(dataset.loadFromFileRange(dataFile, start, count))
    {
        auto t2 = std::chrono::steady_clock::now();
        std::chrono::duration<double> dt = t2 - t1;
        std::cout << "D: Dataset loaded (" << dataset.number_of_persons_injured.size() 
                  << " records) in " << dt.count() << " seconds." << std::endl;
    } else {
        std::cerr << "D: Error loading dataset." << std::endl;
    }
}

void RunServer() {
    std::string server_address("0.0.0.0:50054");
    OverlayServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server D listening on " << server_address << std::endl;
    server->Wait();
}

int main() {
    loadDataset();
    RunServer();
    return 0;
}
