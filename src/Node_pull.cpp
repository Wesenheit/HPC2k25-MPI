#include "Lookup.hpp"
#include "Node.hpp"
#include <algorithm>

void Node::clear_mess_que_pull()
{
    MPI_Waitall(pull_que.req_arr.size(), pull_que.req_arr.data(), MPI_STATUS_IGNORE);
    for (auto element:pull_que.mess_arr)
    {
        delete element;
    }
    pull_que.mess_arr.clear();
    pull_que.req_arr.clear();
}

void Node::send_request(Vertex u)
{
    int dest;
    if (u>=lower &&u <=upper)
    {
        dest = rank;
    }
    else
    {
        dest = table.get_node_for_value(u);
    }
    Vertex *data = new Vertex;
    *data = u;
    if (pull_que.dest.find(dest) == pull_que.dest.end())
    {
        pull_que.dest[dest] = 0;
    }
    pull_que.dest[dest]++;
    pull_que.req_arr.push_back(MPI_REQUEST_NULL);
    pull_que.mess_arr.push_back(data);
    MPI_Isend(data, 1, MPI_INT, dest, 1, world,&pull_que.req_arr.back());

    if (pull_que.req_arr.size() > MAX_QUE_SIZE)
    {
        clear_mess_que_pull();
    }
}

std::unordered_map<Vertex,DVar> Node::accept_requests_normal(int k)
{
    //Step 1 - measure total amount of messages
    std::vector<int> count_to_send(size_world,0);
    std::vector<int> mess_to_recive(size_world,0);

    for (int i = 0; i < size_world;i++)
    {
        if (pull_que.dest.find(i) != pull_que.dest.end())
            count_to_send[i] = pull_que.dest[i];
    }

    //Step 2 - wait for all messages
    clear_mess_que_pull();

    MPI_Alltoall(count_to_send.data(),1,MPI_INT,
        mess_to_recive.data(),1,MPI_INT,world);

    //Step 3 - recive all messages
    std::vector<int> requests_to_return(size_world,0);
    std::vector<int> answers(size_world,0);
    int index = 0;
    std::vector<std::pair<int,Vertex>> requested_vertices;

    while (index < size_world)
    {
        if (mess_to_recive[index] == 0)
        {
            index++;
        }
        else
        {

            Vertex u;
            MPI_Recv(&u,1,MPI_INT,index,1,world,MPI_STATUS_IGNORE);
            assert(u >= lower && u <=upper);
            bool found = false;
            if (k < buckets.size() && buckets[k])
            {
                found = std::find(buckets[k]->begin(), buckets[k]->end(),
                    u) != buckets[k]->end();
            }

            if (found && tenative[u-lower] > k*Delta)
            {
                requests_to_return[index]++;
                requested_vertices.push_back({index,u});
            }
            mess_to_recive[index]--;

        }
    }

    //Step 4 Send numbers of answeres to recieve
    MPI_Alltoall(requests_to_return.data(),1,MPI_INT,
        answers.data(),1,MPI_INT,world);

    //MessStruct returns;
    for (auto const [dest,u]:requested_vertices)
    {
        Message mess;
        mess.v = u;
        mess.distance = tenative[u-lower];
        MPI_Send(&mess, 1, MPI_mess, dest, 2, world);
    }

    //Step 5 recive all answers
    index = 0;
    std::unordered_map<Vertex,DVar> out;
    while (index < size_world)
    {
        if (answers[index] == 0)
        {
            index++;
        }
        else
        {
            Message buff;
            MPI_Recv(&buff,1,MPI_mess,index,2,world,MPI_STATUS_IGNORE);
            out[buff.v] = buff.distance;
            answers[index]--;
        }
    }
    pull_que.dest.clear();
    return out;
}

std::unordered_map<Vertex,DVar> Node::accept_requests_graph(int k)
{
    int degree = table.node.size();
    //Step 1 - measure total amount of messages
    std::vector<int> count_to_send(degree,0);
    std::vector<int> mess_to_recive(degree,0);
    for (int i = 0; i < degree;i++)
    {
        if (pull_que.dest.find(table.node[i]) != pull_que.dest.end())
            count_to_send[i] = pull_que.dest[i];
    }

    //Step 2 - wait for all messages
    clear_mess_que_pull();

    MPI_Neighbor_alltoall(count_to_send.data(),1,MPI_INT,
        mess_to_recive.data(),1,MPI_INT,world);

    //Step 3 - recive all messages
    std::vector<int> requests_to_return(degree,0);
    std::vector<int> answers(degree,0);
    int index = 0;
    std::vector<std::pair<int,Vertex>> requested_vertices;

    while (index < degree)
    {
        if (mess_to_recive[index] == 0)
        {
            index++;
        }
        else
        {

            Vertex u;
            MPI_Recv(&u,1,MPI_INT,table.node[index],1,world,MPI_STATUS_IGNORE);
            assert(u >= lower && u <=upper);
            bool found = false;
            if (k < buckets.size() && buckets[k])
            {
                found = std::find(buckets[k]->begin(), buckets[k]->end(),
                    u) != buckets[k]->end();
            }

            if (found && tenative[u-lower] > k*Delta)
            {
                requests_to_return[index]++;
                requested_vertices.push_back({table.node[index],u});
            }
            mess_to_recive[index]--;

        }
    }

    //Step 4 Send numbers of answeres to recieve
    MPI_Neighbor_alltoall(requests_to_return.data(),1,MPI_INT,
        answers.data(),1,MPI_INT,world);

    //MessStruct returns;
    for (auto const [dest,u]:requested_vertices)
    {
        Message mess;
        mess.v = u;
        mess.distance = tenative[u-lower];
        MPI_Send(&mess, 1, MPI_mess, dest, 2, world);
    }

    //Step 5 recive all answers
    index = 0;
    std::unordered_map<Vertex,DVar> out;
    while (index < degree)
    {
        if (answers[index] == 0)
        {
            index++;
        }
        else
        {
            Message buff;
            MPI_Recv(&buff,1,MPI_mess,table.node[index],2,world,MPI_STATUS_IGNORE);
            out[buff.v] = buff.distance;
            answers[index]--;
        }
    }
    pull_que.dest.clear();
    return out;
}
