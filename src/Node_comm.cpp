#include "Node.hpp"
#include <mpi_proto.h>

#define MAX_QUE_SIZE 128

void Node::get_graph_comm(MPI_Comm *com)
{
    MPI_Info info;
    MPI_Info_create(&info);
    MPI_Info_set(info, "no_local", "true");
    MPI_Info_set(info, "reorder", "true");

    MPI_Dist_graph_create_adjacent(world,table.node.size(),table.node.data(),MPI_UNWEIGHTED,
        table.node.size(),table.node.data(),MPI_UNWEIGHTED,info,1,com);
}

void Node::relax(Vertex u, Vertex v, DVar d,int bucket_th)
{
    if (v >= lower && v <= upper)
    {
        if (d < tenative[v-lower])
        {
            tenative[v-lower] = d;
            int j = d / Delta;
            if (bucket_th > 0 && j > bucket_th) j = bucket_th;
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
        int destination = table.get_node_for_value(v);
        Message* mes = new Message;
        *mes = {v,d};
        MPI_Request req;
        que.dest_arr.push_back(destination);
        que.req_arr.push_back(req);
        que.mess_arr.push_back(mes);
        MPI_Isend(mes,1,MPI_mess,destination,0,
            world,&que.req_arr.back());
        if (que.req_arr.size() > MAX_QUE_SIZE)
        {
            MPI_Waitall(que.req_arr.size(), que.req_arr.data(), MPI_STATUS_IGNORE);
            for (auto element:que.mess_arr)
            {
                delete element;
            }
            que.mess_arr.clear();
            que.req_arr.clear();
        }
    }
}

void Node::synchronize_normal()
{
    //Step 1 - measure total amount of messages
    std::vector<int> count_to_send(size_world,0);
    std::vector<int> mess_to_recive(size_world,0);

    for (auto element:que.dest_arr)
    {
        count_to_send[element]++;
    }
    MPI_Alltoall(count_to_send.data(),1,MPI_INT,
        mess_to_recive.data(),1,MPI_INT,world);

    //Step 2 - wait for all messages
    MPI_Waitall(que.req_arr.size(),que.req_arr.data(), MPI_STATUS_IGNORE);

    //Step 3 - recive all messages
    int index = 0;
    while (index < size_world)
    {
        if (mess_to_recive[index] == 0)
        {
            index++;
        }
        else
        {
            Message buff;
            MPI_Recv(&buff,1,MPI_mess,index,0,world,MPI_STATUS_IGNORE);
            assert(buff.v >= lower && buff.v <=upper);
            relax(0,buff.v,buff.distance);
            mess_to_recive[index]--;
        }
    }

    //Step 4 - cleanup
    que.req_arr.clear();
    que.dest_arr.clear();
    for (auto element: que.mess_arr)
    {
        delete element;
    }
    que.mess_arr.clear();
}



void Node::synchronize_graph()
{
    //Step 1 - measure total amount of messages
    std::vector<int> count_to_send(table.node.size(),0);
    std::vector<int> mess_to_recive(table.node.size(),0);

    for (auto element:que.dest_arr)
    {
        count_to_send[table.get_index(element)]++;
    }
    MPI_Neighbor_alltoall(count_to_send.data(),1,MPI_INT,
        mess_to_recive.data(),1,MPI_INT,world);

    //Step 2 - wait for all messages
    MPI_Waitall(que.req_arr.size(),que.req_arr.data(), MPI_STATUS_IGNORE);

    //Step 3 - recive all messages
    int index = 0;
    while (index < mess_to_recive.size())
    {
        if (mess_to_recive[index] == 0)
        {
            index++;
        }
        else
        {
            Message buff;
            MPI_Recv(&buff,1,MPI_mess,table.node[index],0,world,MPI_STATUS_IGNORE);
            assert(buff.v >= lower && buff.v <=upper);
            relax(0,buff.v,buff.distance);
            mess_to_recive[index]--;
        }
    }

    //Step 4 - cleanup
    que.req_arr.clear();
    que.dest_arr.clear();
    for (auto element: que.mess_arr)
    {
        delete element;
    }
    que.mess_arr.clear();
}
