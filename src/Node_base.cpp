#include "Node.hpp"
#include <cassert>


void Node::load_data(fs::path in, int rank)
{
    vertex_1.clear();
    vertex_2.clear();
    distances.clear();

    std::string name = std::to_string(rank) + ".in";
    in /= name;

    std::ifstream inputFile(in);

    if (!inputFile.is_open())  throw("File error");
    inputFile >> N >> lower >> upper;

    int a,b,c;
    while (inputFile >> a >> b >> c) {
        vertex_1.push_back(a);
        vertex_2.push_back(b);
        distances.push_back(c);

        vertex_1.push_back(b);
        vertex_2.push_back(a);
        distances.push_back(c);
    }
}

Node::Node(int Delta,fs::path in, MPI_Comm com)
{
    this->Delta = Delta;
    world = com;

    int topo_type;

    MPI_Topo_test(world, &topo_type);
    is_graph = (topo_type == MPI_DIST_GRAPH);

    MPI_Comm_rank(world,&rank);
    MPI_Comm_size(world,&size_world);

    load_data(in,rank);

    tenative = std::vector<int>(upper-lower+1,MAX);
    if (lower == 0)
    {
        tenative[0] = 0;
        Bucket* zero;
        zero = new Bucket;
        zero->push_back(0);
        buckets.push_back(zero);
    }
    construct_lookup_table();
}

void Node::construct_lookup_table()
{
    //Let's find all nighbours in the system
    //
    int* buffer_int;
    std::vector<int> buffer(2*size_world,0);
    if (rank == 0)
    {
        buffer_int = buffer.data();
    }
    else
    {
        buffer_int = nullptr;
    }
    int data[2] = {lower, upper};
    MPI_Gather(data, 2, MPI_INT, buffer_int, 2, MPI_INT, 0, world);
    if (rank == 0)
    {
        for (int i = 0; i < size_world; i++)
        {
            table.add(i, buffer[2*i], buffer[2*i+1]);
        }
    }
    MPI_Barrier(world);

    for (int i = 0; i < size_world; i++)
    {
        if (rank == 0)
        {
            data[0] = table.get_lower(i);
            data[1] = table.get_upper(i);
            MPI_Bcast(data, 2, MPI_INT, 0, world);
        }
        else
        {
            MPI_Bcast(data, 2, MPI_INT, 0, world);
            for (int j = 0; j < vertex_1.size(); j++)
            {
                if (((vertex_1[j] >= data[0] && vertex_1[j] <= data[1]) ||
                    (vertex_2[j] >= data[0] && vertex_2[j] <= data[1]))
                    && (table.get_index(i) == -1) && i != rank)
                {
                    table.add(i, data[0], data[1]);
                }
            }
        }
    }
    if (rank == 0)
    {
        Lookup new_table;
        for (int i = 0; i < size_world; i++)
        {
            data[0] = table.get_lower(i);
            data[1] = table.get_upper(i);
            for (int j = 0; j < vertex_1.size(); j++)
            {
                if (((vertex_1[j] >= data[0] && vertex_1[j] <= data[1]) ||
                    (vertex_2[j] >= data[0] && vertex_2[j] <= data[1]))
                        && (new_table.get_index(i) == -1) && i != rank)
                {
                    new_table.add(i, data[0], data[1]);
                }
            }
        }
        table = new_table;
    }
    MPI_Barrier(world);
}
