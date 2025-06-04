#include <mpi.h>
#include "Process.hpp"
#include <filesystem>

namespace fs = std::filesystem;
int main(int argc, char** argv)
{
    MPI_Init(NULL,NULL);
    fs::path input_path = argv[1];
    fs::path output_path = argv[2];


    int delta = 1;
    MPI_Comm com = MPI_COMM_WORLD;
    auto node = Node(delta,input_path,&com);
    node.construct_lookup_table();

    node.run();

    node.save(output_path);
    MPI_Finalize();
    return 0;
}
