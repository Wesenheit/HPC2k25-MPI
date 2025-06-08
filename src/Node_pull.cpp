#include "Lookup.hpp"
#include "Node.hpp"
#include <ostream>
#include <algorithm>

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
    MPI_Request req;
    Vertex *data = new Vertex;
    *data = u;
    pull_que.dest_arr.push_back(dest);
    pull_que.req_arr.push_back(req);
    pull_que.mess_arr.push_back(data);
    MPI_Isend(data, 1, MPI_INT, dest, 1, world,&pull_que.req_arr.back());
}

std::unordered_map<Vertex,DVar> Node::accept_requests(int k)
{
    //Step 1 - measure total amount of messages
    std::vector<int> count_to_send(size_world,0);
    std::vector<int> mess_to_recive(size_world,0);

    for (auto element:pull_que.dest_arr)
    {
        count_to_send[element]++;
    }

    //Step 2 - wait for all messages
    MPI_Waitall(pull_que.req_arr.size(),pull_que.req_arr.data(), MPI_STATUS_IGNORE);

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

    MessStruct returns;
    for (auto const [dest,u]:requested_vertices)
    {
        Message mess;
        mess.v = u;
        mess.distance = tenative[u-lower];
        MPI_Send(&mess, 1, MPI_mess, dest, 2, world);
    }

    MPI_Barrier(world);

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
    pull_que.req_arr.clear();
    pull_que.dest_arr.clear();
    for (auto element: pull_que.mess_arr)
    {
        delete element;
    }
    pull_que.mess_arr.clear();
    return out;
}
