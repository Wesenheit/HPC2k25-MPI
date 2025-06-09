#include <mpi.h>
#include "Node.hpp"

#define OPT true

int main(int argc, char** argv)
{
    float tau = 0.4;
    int delta = 40;
    bool graph = false;
    fs::path input_path = argv[1];
    fs::path output_path = argv[2];

    for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--graph") {
                graph = true;
            }
        }
    MPI_Init(NULL,NULL);

    MPI_Comm com;
    Node *node = new Node(delta,input_path,MPI_COMM_WORLD);

    if (graph)
    {
        node->get_graph_comm(&com);
        delete node;
        node = new Node(delta,input_path,com);
    }
    if (OPT)
    {
        node->run_opt(tau);
    }
    else
    {
        node->run();
    }
    node->save(output_path);
    delete node;
    MPI_Finalize();
    return 0;
}
