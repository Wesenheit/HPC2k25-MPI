#ifndef PROCESS
#define PROCESS

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include <utility>
#include <filesystem>
#include <mpi.h>

#include "Lookup.hpp"

namespace fs = std::filesystem;
using Bucket = std::list<int>;
static int MAX = 1000000;

class Node
{
    int N;
    int lower;
    int upper;
    int Delta;

    MPI_Comm world;
    int rank;
    int size_world;

    int source_size;
    int target_size;

    std::vector<int> distances;
    std::vector<int> vertex_1;
    std::vector<int> vertex_2;

    std::vector<Bucket*> buckets;
    std::vector<int> tenative;

    Lookup table;

    public:
    Node(int Delta,fs::path in, MPI_Comm* com)
    {
        this->Delta = Delta;
        world = *com;
        MPI_Comm_rank(*com,&rank);
        MPI_Comm_size(*com,&size_world);
    
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
        }
        

        tenative = std::vector<int>(upper-lower,INT_MAX);
        if (lower == 0)
        {
            tenative[0] = 0;
            Bucket* zero;
            zero = new Bucket;
            zero->push_back(0);
            buckets.push_back(zero);
        }

    }
    void relax(int u, int v,int d);
    int all_reduce(int* value,MPI_Op op)
    {
        int global;
        MPI_Allreduce(value,
            &global,
            1,
            MPI_INT,
            op,
            world);
        return global;
    }
    void synchronize();
    void construct_lookup_table();
    void run();
};


void Node::construct_lookup_table()
{
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
        //MPI_Barrier(world);
    }
    std::cout<<rank<<" " << table.leng() << std::endl;
}

void Node::relax(int u, int v, int d)
{
    if (v > lower && v < upper)
    {
        if (d < tenative[v-lower])
        {
            tenative[v-lower] = d;
            int j = d / Delta;
            if (j >= buckets.size())
            {
                buckets.resize(j + 1, nullptr);
            }
            if (!buckets[j])
            {
                buckets[j] = new Bucket;
            }
            if (buckets[j]->empty() || 
                buckets[j]->back() != v)
            {
                buckets[j]->push_back(v);
            }
        }
    }
    else
    {
        return;
    }
}

void Node::synchronize()
{
    
    MPI_Barrier(world);
}


void Node::run()
{
    int k = 0;
    int global_k;
    int work_to_do;
    do
    {
        while (k < buckets.size() && (!buckets[k] || buckets[k]->empty())) k++;

        if (k == MAX) break;

        k = all_reduce(&k,MPI_MIN);
        if (rank == 0)
        {
            //std::cout << "k: " << k << std::endl;
        }
        do
        {
            if (k < buckets.size() && buckets[k])
            {
                Bucket& bucket = *buckets[k];
                while (!bucket.empty())
                {
                    int u = bucket.front();
                    bucket.pop_front();
                    for (int i = 0; i < distances.size();i++)
                    {
                        if (vertex_1[i] == u)
                        {
                            int v = vertex_2[i];
                            int d = distances[i];
                            relax(u,v,d + tenative[u-lower]);
                        }
                    }

               }
            }
            synchronize();
            work_to_do = (k < buckets.size() && buckets[k] && !buckets[k]->empty());
        }
        while (all_reduce(&work_to_do,MPI_LOR));

    k++;
    }
    while (true);
        
    

}


#endif
