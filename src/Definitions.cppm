module;
#include <filesystem>
#include <list>

export module Definitions;

export using DVar = unsigned long long;
export using Vertex = int;
export using Path = std::filesystem::path; // Add this explicit export
export constexpr int MAX = INT_MAX;
export using Bucket = std::list<int>;
export constexpr int MAX_QUE_SIZE = 128;
