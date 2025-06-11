#include "Lookup.hpp"
#include "Node.hpp"
#include <cassert>

void Node::get_graph_comm(MPI_Comm *com)
{
    MPI_Info info;
    MPI_Info_create(&info);
    MPI_Info_set(info, "no_local", "true");
    MPI_Info_set(info, "reorder", "true");
    MPI_Dist_graph_create_adjacent(world,table.node.size(),table.node.data(),table.how_many.data(),
        table.node.size(),table.node.data(),table.how_many.data(),info,1,com);
    MPI_Info_free(&info);
}

void Node::initialize_rma_window()
{
    int size = tenative.size();
    buffer = std::vector<DVar>(size,0);
    std::fill(buffer.begin(), buffer.end(), std::numeric_limits<DVar>::max());
    MPI_Win_create(buffer.data(),size * sizeof(DVar), sizeof(DVar),
        MPI_INFO_NULL, world, &tenative_win);
}

void Node::finalize_rma_window()
{
    MPI_Win_free(&tenative_win);
}

void Node::relax(Vertex u, Vertex v, DVar d, int bucket_th)
{
    if (v >= lower && v <= upper)
    {
        if (d < tenative[v-lower])
        {
            tenative[v-lower] = d;
            add_to_bucket(v, d,bucket_th);
        }
    }
    else
    {
        int dest = table.get_node_for_value(v);
        Message mes = {v,d};
        if (std::find(que.dest.begin(),que.dest.end(),dest)==que.dest.end())
            MPI_Win_lock(MPI_LOCK_SHARED, dest, 0, tenative_win);

        que.dest.push_back(dest);
        que.mess_arr.push_back(mes);
        que.req_arr.push_back(MPI_REQUEST_NULL);

        MPI_Aint target_disp = (v - table.get_lower(dest));

        MPI_Raccumulate(&(que.mess_arr.back().distance), 1, MPI_UNSIGNED_LONG_LONG, dest,
                       target_disp, 1, MPI_UNSIGNED_LONG_LONG,
                       MPI_MIN, tenative_win,&que.req_arr.back());

        if (que.mess_arr.size() > MAX_QUE_SIZE)
        {
            synchronize_rma();
        }
    }
}

void Node::synchronize_rma()
{

    std::unordered_map<int, std::vector<MPI_Request>> per_dest_REQ;
    for (int i = 0; i < que.dest.size(); i++)
    {
        per_dest_REQ[que.dest[i]].push_back(que.req_arr[i]);
    }

    for (auto& [dest, req] : per_dest_REQ)
    {
        MPI_Waitall(req.size(), req.data(), MPI_STATUSES_IGNORE);
        MPI_Win_unlock(dest, tenative_win);
    }

    // Clear local queue

    que.dest.clear();
    que.mess_arr.clear();
    que.req_arr.clear();
}

void Node::synchronize(int bucket_th)
{
    synchronize_rma();
    MPI_Barrier(world);
    for (Vertex v = lower; v <= upper; v++)
    {
        if (buffer[v-lower] < tenative[v-lower])
        {
            tenative[v-lower] = buffer[v-lower];
            add_to_bucket(v, tenative[v-lower],bucket_th);
        }
    }
}
