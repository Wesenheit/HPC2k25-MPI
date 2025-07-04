module;

#include <algorithm>
#include <iterator>
#include <vector>

export module Lookup;
import Definitions;

export class Lookup {
public:
  std::vector<int> node;
  std::vector<Vertex> lower;
  std::vector<Vertex> upper;
  std::vector<int> how_many;
  int size;

  Lookup() {
    node.resize(0);
    lower.resize(0);
    upper.resize(0);
    how_many.resize(0);
    size = 0;
  }
  void add(int n, Vertex l, Vertex u) {
    auto it = std::find(node.begin(), node.end(), n);
    if (it == node.end()) {
      node.push_back(n);
      lower.push_back(l);
      upper.push_back(u);
      how_many.push_back(1);
      size++;
    } else {
      auto index = std::distance(node.begin(), it);
      how_many[index]++;
    }
  }
  int leng() { return size; }
  Vertex get_index(int node) {
    for (int i = 0; i < this->node.size(); i++) {
      if (this->node[i] == node) {
        return i;
      }
    }
    return -1;
  }
  Vertex get_lower(int node) { return lower[get_index(node)]; }
  Vertex get_upper(int node) { return upper[get_index(node)]; }

  int get_node_for_value(Vertex value) {
    for (int i = 0; i < size; i++) {
      if (value >= lower[i] && value <= upper[i]) {
        return node[i];
      }
    }
    throw("node not found int the lookup table");
  }
  Lookup &operator=(const Lookup &other) {
    if (this != &other) {
      node = other.node;
      lower = other.lower;
      upper = other.upper;
      size = other.size;
    }
    return *this;
  }
};
