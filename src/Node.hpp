#ifndef PROCESS
#define PROCESS

#include <algorithm>
#include <climits>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include <filesystem>
#include <cassert>
#include <mpi.h>
#include <unordered_map>
#include "Lookup.hpp"

#define MAX_QUE_SIZE 64

namespace fs = std::filesystem;
using Bucket = std::list<int>;
static int MAX = INT_MAX;
using DVar = unsigned long long;
using Vertex = int;

typedef struct {
    Vertex v;
    DVar distance;
} Message;


typedef struct {
    std::vector<int> dest;
    std::vector<Message> mess_arr;
} MessStruct;

class Node
{
    int N; //number of nodes
    int lower; //lower bound for nodes we manage
    int upper; //upper bound for nodes we manage
    int Delta; //just delta

    MPI_Comm world; //communicator
    int rank; //rank
    int size_world; //number of processes

    std::unordered_map<Vertex, std::vector<std::pair<Vertex, DVar>>> adjacency_list;
    std::vector<int>long_count;
    DVar max;

    std::vector<Bucket*> buckets; //vector of buckets
    std::vector<DVar> tenative; //tenative distance
    std::vector<int> deleted;

    Lookup table; //lookup table, alows to
    //obtain the source of the target
    bool is_graph;
    MessStruct que;
    MPI_Datatype MPI_mess;

    MPI_Win window;
    MPI_Win count_window;
    int local_synchro;
    int global_synchro;
    Message* shared_buffer;
    int* message_counts;
    std::vector<Message> local_messages;

    public:
        Node(int Delta,fs::path in, MPI_Comm com);
        void relax(Vertex u, Vertex v,DVar d,int bucket_th = -1);
        template<typename T>
        T all_reduce(T* value,MPI_Datatype type, MPI_Op op);
        void load_data(fs::path in, int rank);
        void get_graph_comm(MPI_Comm *com);
        void save(fs::path out)
        {
            std::string name = std::to_string(rank) + ".out";
            out /= name;
            std::ofstream outputFile(out);
            if (!outputFile.is_open()) throw("File error");
            for (int i = lower; i <= upper; i++)
            {
                outputFile << tenative[i-lower] << std::endl;
            }
        }
        void initialize_rma_window();
        void finalize_rma_window();
        void synchronize_rma();
        void synchronize()
        {
            synchronize_rma();
            global_synchro = 1;
            while(global_synchro)
            {
                synchronize_rma();
            }
        }
        void construct_lookup_table();
        void run();

        void run_opt(float tau);
        void run_pruning();
        ~Node()
        {
            for (auto element:buckets)
            {
                delete element;
            }
            MPI_Type_free(&MPI_mess);
        }
};

template <typename T>
T Node::all_reduce(T* value,MPI_Datatype type,MPI_Op op)
{
    T global;
    MPI_Allreduce(value,
        &global,
        1,
        type,
        op,
        world);
    return global;
}

#endif
