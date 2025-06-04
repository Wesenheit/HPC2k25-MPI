#include <mpi.h>
#include "Node.hpp"
#include <filesystem>

namespace fs = std::filesystem;
int main(int argc, char** argv)
{

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



    int delta = 40;
    MPI_Comm com;
    Node *node = new Node(delta,input_path,MPI_COMM_WORLD);

    if (graph)
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
