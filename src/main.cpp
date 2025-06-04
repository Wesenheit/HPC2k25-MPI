#include <mpi.h>
#include "Node.hpp"
#include <filesystem>

#define USE_GRAPH true

namespace fs = std::filesystem;
int main(int argc, char** argv)
{
    MPI_Init(NULL,NULL);
    fs::path input_path = argv[1];
    fs::path output_path = argv[2];


    int delta = 40;
    MPI_Comm com;
    Node *node = new Node(delta,input_path,MPI_COMM_WORLD);

    if (USE_GRAPH)
    {
        node->get_graph_comm(&com);
        delete node;
        node = new Node(delta,input_path,com);
    }

    node->run();

    node->save(output_path);
    delete node;
    MPI_Finalize();
    return 0;
}
