#include "Node.hpp"
void Node::get_graph_comm(MPI_Comm *com)
{
    MPI_Info info;
    MPI_Info_create(&info);
    MPI_Info_set(info, "no_local", "true");
    MPI_Info_set(info, "reorder", "true");

    MPI_Dist_graph_create_adjacent(world,table.node.size(),table.node.data(),MPI_UNWEIGHTED,
        table.node.size(),table.node.data(),MPI_UNWEIGHTED,info,1,com);
}

int Node::all_reduce(int* value,MPI_Op op)
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
void Node::relax(int u, int v, int d)
{
    if (v >= lower && v <= upper)
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
        int destination = table.get_node_for_value(v);
        Message * mes = new Message;
        *mes = {v,d};
        que.push_back({destination,mes});
    }
}

void Node::synchronize_normal()
{
    //Step 1 - measure total amount of messages
    std::vector<int> count_to_send(size_world,0);
    std::vector<int> mess_to_recive(size_world,0);

    for (auto element:que)
    {
        count_to_send[element.first]++;
    }
    MPI_Alltoall(count_to_send.data(),1,MPI_INT,
        mess_to_recive.data(),1,MPI_INT,world);

    //Step 2 - send all messages
    std::vector<MPI_Request> request_arr(que.size());
    for (int i = 0; i < que.size();i++)
    {
        auto element = que[i];
        MPI_Isend(element.second->data(),2,MPI_INT,element.first,0,
            world,&request_arr[i]);
    }
    MPI_Waitall(request_arr.size(),request_arr.data(), MPI_STATUS_IGNORE);

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
            int buff[2];
            MPI_Recv(buff,2,MPI_INT,index,0,world,MPI_STATUS_IGNORE);
            assert(buff[0] >= lower && buff[0] <=upper);
            relax(0,buff[0],buff[1]);
            mess_to_recive[index]--;
        }
    }

    //Step 4 - cleanup
    for (auto element:que)
    {
        delete element.second;
    }
    que.clear();
}



void Node::synchronize_graph()
{
    //Step 1 - measure total amount of messages
    std::vector<int> count_to_send(table.node.size(),0);
    std::vector<int> mess_to_recive(table.node.size(),0);

    for (auto element:que)
    {
        count_to_send[table.get_index(element.first)]++;
    }
    MPI_Neighbor_alltoall(count_to_send.data(),1,MPI_INT,
        mess_to_recive.data(),1,MPI_INT,world);

    //Step 2 - send all messages
    std::vector<MPI_Request> request_arr(que.size());
    for (int i = 0; i < que.size();i++)
    {
        auto element = que[i];
        MPI_Isend(element.second->data(),2,MPI_INT,element.first,0,
            world,&request_arr[i]);
    }
    MPI_Waitall(request_arr.size(),request_arr.data(), MPI_STATUS_IGNORE);

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
            int buff[2];
            MPI_Recv(buff,2,MPI_INT,table.node[index],0,world,MPI_STATUS_IGNORE);
            assert(buff[0] >= lower && buff[0] <=upper);
            relax(0,buff[0],buff[1]);
            mess_to_recive[index]--;
        }
    }

    //Step 4 - cleanup
    for (auto element:que)
    {
        delete element.second;
    }
    que.clear();
}
