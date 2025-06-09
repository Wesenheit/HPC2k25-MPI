#include "Node.hpp"
auto nothing = [](Message* val) {};

void Node::clear_mess_que(std::function<void(Message*)> fun)
{
    MPI_Waitall(que.req_arr.size(), que.req_arr.data(), MPI_STATUS_IGNORE);
    for (Message* element:que.mess_arr)
    {
        fun(element);
        delete element;
    }
    que.mess_arr.clear();
    que.req_arr.clear();
}
/*
void Node::clear_que_and_relax()
{
    MPI_Waitall(que.req_arr.size(), que.req_arr.data(), MPI_STATUS_IGNORE);
    for (auto buff:que.mess_arr)
    {
        assert(buff->v >= lower && buff->v <=upper);
        relax(0,buff->v,buff->distance);
        delete buff;
    }
    que.mess_arr.clear();
    que.req_arr.clear();
}
*/
void Node::get_graph_comm(MPI_Comm *com)
{
    MPI_Info info;
    MPI_Info_create(&info);
    MPI_Info_set(info, "no_local", "true");
    MPI_Info_set(info, "reorder", "true");
    MPI_Dist_graph_create_adjacent(world,table.node.size(),table.node.data(),table.how_many.data(),
        table.node.size(),table.node.data(),table.how_many.data(),info,1,com);
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
        if (que.dest.find(destination) == que.dest.end())
        {
            que.dest[destination] = 0;
        }
        que.dest[destination]++;
        que.req_arr.push_back(MPI_REQUEST_NULL);
        que.mess_arr.push_back(mes);
        MPI_Isend(mes,1,MPI_mess,destination,0,
            world,&que.req_arr.back());

        if (que.req_arr.size() > MAX_QUE_SIZE)
        {
            clear_mess_que(nothing);
        }
    }
}

void Node::synchronize_normal()
{
    //Step 1 - measure total amount of messages
    std::vector<int> count_to_send(size_world,0);
    std::vector<int> mess_to_recive(size_world,0);
    for (int i = 0; i < size_world;i++)
    {
        if (que.dest.find(i) != que.dest.end())
            count_to_send[i] = que.dest[i];
    }

    //Step 2 - wait for all messages
    clear_mess_que(nothing);
    que.dest.clear();


    MPI_Alltoall(count_to_send.data(),1,MPI_INT,
        mess_to_recive.data(),1,MPI_INT,world);

    //Step 3 - recive all messages
    //
    auto relax_lambda = [this](Message* val){
        assert(val->v >= lower && val->v <=upper);
        this->relax(0,val->v,val->distance);
    };
    int index = 0;
    while (index < size_world)
    {
        if (mess_to_recive[index] == 0)
        {
            index++;
        }
        else
        {
            Message *mes = new Message;
            que.req_arr.push_back(MPI_REQUEST_NULL);
            que.mess_arr.push_back(mes);
            MPI_Irecv(mes,1,MPI_mess,index,0,world,&que.req_arr.back());

            if (que.req_arr.size() > MAX_QUE_SIZE)
            {
                clear_mess_que(relax_lambda);
            }
            mess_to_recive[index]--;
        }
    }
    clear_mess_que(relax_lambda);
}



void Node::synchronize_graph()
{
    int degree = table.node.size();
    //Step 1 - measure total amount of messages
    std::vector<int> count_to_send(degree,0);
    std::vector<int> mess_to_recive(degree,0);

    for (int i = 0; i < degree;i++)
    {
        if (que.dest.find(table.node[i]) != que.dest.end())
            count_to_send[i] = que.dest[table.node[i]];
    }

    //Step 2 - wait for all messages
    clear_mess_que(nothing);
    que.dest.clear();

    MPI_Neighbor_alltoall(count_to_send.data(),1,MPI_INT,
        mess_to_recive.data(),1,MPI_INT,world);

    //Step 3 - recive all messages
    auto relax_lambda = [this](Message* val){
        assert(val->v >= lower && val->v <=upper);
        this->relax(0,val->v,val->distance);
    };
    int index = 0;
    while (index < mess_to_recive.size())
    {
        if (mess_to_recive[index] == 0)
        {
            index++;
        }
        else
        {

            Message *mes = new Message;
            que.req_arr.push_back(MPI_REQUEST_NULL);
            que.mess_arr.push_back(mes);
            MPI_Irecv(mes,1,MPI_mess,table.node[index],0,world,&que.req_arr.back());

            if (que.req_arr.size() > MAX_QUE_SIZE)
            {
                clear_mess_que(relax_lambda);
            };
            mess_to_recive[index]--;
        }
    }
    clear_mess_que(relax_lambda);
}
