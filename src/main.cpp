#include <filesystem> // Add this to fix the path issue
#include <limits>
#include <mpi.h>
import Node;
import Definitions;

#define OPT true

int main(int argc, char **argv) {
  float tau = 0.4;
  DVar delta = 10; // std::numeric_limits<DVar>::max();
  bool graph = false;
  Path input_path = argv[1];
  Path output_path = argv[2];

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--graph") {
      graph = true;
    }
  }
  MPI_Init(NULL, NULL);

  MPI_Comm com;
  Node *node = new Node(delta, input_path, MPI_COMM_WORLD);

  if (graph) {
    node->get_graph_comm(&com);
    delete node;
    node = new Node(delta, input_path, com);
  }
  if (OPT) {
    node->run_opt(tau);
  } else {
    node->run();
  }
  node->save(output_path);
  delete node;
  MPI_Finalize();
  return 0;
}
