module; // Fixed typo
#include <algorithm>
#include <cassert>
#include <climits>
#include <fstream>
#include <iostream>
#include <list>
#include <mpi.h> //module;
#include <string>
#include <unordered_map>
#include <vector>

export module Node:interface;

import Lookup;
import Definitions;

typedef struct {
  Vertex v;
  DVar distance;
} Message;

typedef struct {
  std::vector<int> dest;
  std::vector<Message> mess_arr;
  std::vector<MPI_Request> req_arr;
} MessStruct;

export class Node {
  int N;      // number of nodes
  int lower;  // lower bound for nodes we manage
  int upper;  // upper bound for nodes we manage
  DVar Delta; // just delta

  MPI_Comm world; // communicator
  int rank;       // rank
  int size_world; // number of processes

  std::unordered_map<Vertex, std::vector<std::pair<Vertex, DVar>>>
      adjacency_list;
  std::vector<int> long_count;
  DVar max;

  std::vector<Bucket *> buckets; // vector of buckets
  std::vector<DVar> tenative;    // tenative distance
  std::vector<DVar> buffer;
  std::vector<int> deleted;

  Lookup table; // lookup table, alows to
  // obtain the source of the target
  bool is_graph;
  MessStruct que;
  MPI_Datatype MPI_mess;

  MPI_Win tenative_win;

public:
  Node(int Delta, Path in, MPI_Comm com);
  void relax(Vertex u, Vertex v, DVar d, int bucket_th = -1);
  template <typename T> T all_reduce(T *value, MPI_Datatype type, MPI_Op op);
  void load_data(Path in, int rank);
  void get_graph_comm(MPI_Comm *com);
  void save(Path out) {
    std::string name = std::to_string(rank) + ".out";
    out /= name;
    std::ofstream outputFile(out);
    if (!outputFile.is_open())
      throw("File error");
    for (int i = lower; i <= upper; i++) {
      outputFile << tenative[i - lower] << std::endl;
    }
  }
  void initialize_rma_window();
  void finalize_rma_window();
  void synchronize_rma();
  void synchronize(int bucket_th = -1);

  void add_to_bucket(Vertex v, DVar d, int bucket_th) {
    int j = d / Delta;
    if (bucket_th > 0 && j > bucket_th)
      j = bucket_th;
    if (j >= buckets.size()) {
      buckets.resize(j + 1, nullptr);
    }
    if (!buckets[j]) {
      buckets[j] = new Bucket;
    }
    if (buckets[j]->empty() || buckets[j]->back() != v) {
      buckets[j]->push_back(v);
    }
  };

  void construct_lookup_table();
  void run();

  void run_opt(float tau);
  void run_pruning();
  ~Node() {
    for (auto element : buckets) {
      delete element;
    }
    MPI_Type_free(&MPI_mess);
  }
};

template <typename T>
T Node::all_reduce(T *value, MPI_Datatype type, MPI_Op op) {
  T global;
  MPI_Allreduce(value, &global, 1, type, op, world);
  return global;
}
