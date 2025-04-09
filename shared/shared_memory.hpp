#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>

class SharedMemory {
public:
    static void store(const std::string& node, const std::string& data);
    static std::vector<std::string> get_data(const std::string& node);
private:
    static std::unordered_map<std::string, std::vector<std::string>> storage;
    static std::mutex mtx;
};
