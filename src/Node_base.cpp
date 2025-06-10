#include "Lookup.hpp"
#include "Node.hpp"
#include <cassert>
#include <limits>


void Node::load_data(fs::path in, int rank)
{
    std::string name = std::to_string(rank) + ".in";
    in /= name;

    std::ifstream inputFile(in);

    if (!inputFile.is_open())  throw("File error");
    inputFile >> N >> lower >> upper;

    Vertex u,v;
    DVar d;
    max = 0;
    long_count = std::vector<int>(N,0);
    while (inputFile >> u >> v >> d) {
        adjacency_list[u].push_back({v,d});
        adjacency_list[v].push_back({u,d});
        if (d>Delta)
        {
            long_count[v]++;
            long_count[u]++;
        }
        if (d > max)
        {
            max = d;
        }
    }

    int block_lengths[2] = {1, 1};
    MPI_Aint displacements[2];
    MPI_Datatype types[2] = {MPI_INT, MPI_LONG_LONG};

    // Compute displacements:
    displacements[0] = offsetof(Message, v);
    displacements[1] = offsetof(Message, distance);

    // Create and commit the datatype:
    MPI_Type_create_struct(2, block_lengths, displacements, types, &MPI_mess);
    MPI_Type_commit(&MPI_mess);
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

    tenative = std::vector<DVar>(upper-lower+1,std::numeric_limits<DVar>::max());

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
    Vertex data[2] = {lower, upper};
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

            for (const auto& pair:adjacency_list)
            {
                Vertex u = pair.first;
                for (const auto& [v, w] : pair.second) {
                    if (((u >= data[0] && u <= data[1]) ||
                    (v >= data[0] && v <= data[1])))
                    {
                        table.add(i, data[0], data[1]);
                    }
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
            for (const auto& pair:adjacency_list)
            {
                Vertex u = pair.first;
                for (const auto& [v, w] : pair.second) {
                    if (((u >= data[0] && u <= data[1]) ||
                    (v >= data[0] && v <= data[1])))
                    {
                        new_table.add(i, data[0], data[1]);
                    }
                }
            }
        }
        table = new_table;
    }
    MPI_Barrier(world);
}
