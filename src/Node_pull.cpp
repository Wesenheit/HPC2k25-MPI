#include "Lookup.hpp"
#include "Node.hpp"
#include <algorithm>
#include <set>

auto nothing_Vertex = [](Vertex* val) {};
auto nothing_Message = [](Message* val) {};

void Node::clear_mess_que_pull(std::function<void(Vertex*)> fun)
{
    MPI_Waitall(pull_que.req_arr.size(), pull_que.req_arr.data(), MPI_STATUS_IGNORE);
    for (auto element:pull_que.mess_arr)
    {
        fun(element);
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
        clear_mess_que_pull(nothing_Vertex);
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
    clear_mess_que_pull(nothing_Vertex);
    pull_que.dest.clear();

    MPI_Alltoall(count_to_send.data(),1,MPI_INT,
        mess_to_recive.data(),1,MPI_INT,world);

    //Step 3 - recive all messages

    std::vector<int> requests_to_return(size_world,0);
    std::vector<int> answers(size_world,0);
    int index = 0;
    std::set<std::pair<int,Vertex>> requested_vertices;

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
            if (deleted.size() > 0)
            {
                found = std::find(deleted.begin(), deleted.end(),
                    u) != deleted.end();
            }

            if (found && tenative[u-lower] >= k*Delta)
            {
                if (requested_vertices.find({index,u}) == requested_vertices.end())
                {
                    requests_to_return[index]++;
                    requested_vertices.insert({index,u});
                }
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
        Message *mes = new Message;
        mes->v = u;
        mes->distance = tenative[u-lower];
        que.req_arr.push_back(MPI_REQUEST_NULL);
        que.mess_arr.push_back(mes);
        MPI_Isend(mes,1,MPI_mess,dest,2,
            world,&que.req_arr.back());

        if (que.req_arr.size() > MAX_QUE_SIZE)
        {
            clear_mess_que(nothing_Message);
        }
    }
    clear_mess_que(nothing_Message);

    //Step 5 recive all answers
    index = 0;
    std::unordered_map<Vertex,DVar> out;

    auto save = [&out](Message * val)
    {
        out[val->v] = val->distance;
    };

    while (index < size_world)
    {
        if (answers[index] == 0)
        {
            index++;
        }
        else
        {
            Message * buff = new Message;
            que.req_arr.push_back(MPI_REQUEST_NULL);
            que.mess_arr.push_back(buff);
            MPI_Irecv(buff,1,MPI_mess,index,2,world,&que.req_arr.back());
            answers[index]--;
            if (que.req_arr.size() > MAX_QUE_SIZE)
            {
                clear_mess_que(save);
            }
        }
    }
    clear_mess_que(save);

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
            count_to_send[i] = pull_que.dest[table.node[i]];
    }

    //Step 2 - wait for all messages
    clear_mess_que_pull(nothing_Vertex);
    pull_que.dest.clear();

    MPI_Neighbor_alltoall(count_to_send.data(),1,MPI_INT,
        mess_to_recive.data(),1,MPI_INT,world);

    //Step 3 - recive all messages
    std::vector<int> requests_to_return(degree,0);
    std::vector<int> answers(degree,0);
    int index = 0;
    std::set<std::pair<int,Vertex>> requested_vertices;

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
            if (deleted.size() > 0)
            {
                found = std::find(deleted.begin(), deleted.end(),
                    u) != deleted.end();
            }

            if (found && tenative[u-lower] >= k*Delta)
            {
                if (requested_vertices.find({table.node[index],u}) == requested_vertices.end())
                {
                    requests_to_return[index]++;
                    requested_vertices.insert({table.node[index],u});
                }
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
        Message *mes = new Message;
        mes->v = u;
        mes->distance = tenative[u-lower];
        que.req_arr.push_back(MPI_REQUEST_NULL);
        que.mess_arr.push_back(mes);
        MPI_Isend(mes,1,MPI_mess,dest,2,
            world,&que.req_arr.back());

        if (que.req_arr.size() > MAX_QUE_SIZE)
        {
            clear_mess_que(nothing_Message);
        }
    }
    clear_mess_que(nothing_Message);

    //Step 5 recive all answers
    //Step 5 recive all answers
    index = 0;
    std::unordered_map<Vertex,DVar> out;

    auto save = [&out](Message * val)
    {
        out[val->v] = val->distance;
    };

    while (index < size_world)
    {
        if (answers[index] == 0)
        {
            index++;
        }
        else
        {
            Message * buff = new Message;
            que.req_arr.push_back(MPI_REQUEST_NULL);
            que.mess_arr.push_back(buff);
            MPI_Irecv(buff,1,MPI_mess,index,2,world,&que.req_arr.back());
            answers[index]--;
            if (que.req_arr.size() > MAX_QUE_SIZE)
            {
                clear_mess_que(save);
            }
        }
    }
    clear_mess_que(save);
    return out;
}
