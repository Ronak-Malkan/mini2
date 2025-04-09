#include "shared_memory.hpp"

std::unordered_map<std::string, std::vector<std::string>> SharedMemory::storage;
std::mutex SharedMemory::mtx;

void SharedMemory::store(const std::string& node, const std::string& data) {
    std::lock_guard<std::mutex> lock(mtx);
    storage[node].push_back(data);
}

std::vector<std::string> SharedMemory::get_data(const std::string& node) {
    std::lock_guard<std::mutex> lock(mtx);
    return storage[node];
}
