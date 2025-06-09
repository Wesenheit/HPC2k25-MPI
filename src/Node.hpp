#ifndef PROCESS
#define PROCESS

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
    std::vector<int> dest_arr;
    std::vector<MPI_Request> req_arr;
    std::vector<Message*> mess_arr;
} MessStruct;


typedef struct {
    std::vector<int> dest_arr;
    std::vector<MPI_Request> req_arr;
    std::vector<Vertex*> mess_arr;
} PullStruct;

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

    PullStruct pull_que;

    Lookup table; //lookup table, alows to
    //obtain the source of the target
    bool is_graph;
    MessStruct que;
    MPI_Datatype MPI_mess;

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
        void synchronize()
        {
            if (is_graph)
            {
                synchronize_graph();
            }
            else
            {
                synchronize_normal();
            }
        }
        void synchronize_normal();
        void synchronize_graph();
        void construct_lookup_table();
        void run();

        void send_request(Vertex u);
        std::unordered_map<Vertex,DVar> accept_requests(int k)
        {
            if (is_graph)
            {
                return accept_requests_graph(k);
            }
            else
            {
                return accept_requests_normal(k);
            }
        }
        std::unordered_map<Vertex,DVar> accept_requests_normal(int k);
        std::unordered_map<Vertex,DVar> accept_requests_graph(int k);


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
