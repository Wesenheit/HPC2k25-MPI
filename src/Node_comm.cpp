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
}

void Node::initialize_rma_window()
{
    int buffer_size = size_world*MAX_QUE_SIZE * MAX_QUE_SIZE * sizeof(Message);
    MPI_Win_allocate(buffer_size, sizeof(Message), MPI_INFO_NULL,
                     world, &shared_buffer, &window);

    MPI_Win_allocate(size_world * sizeof(int), sizeof(int), MPI_INFO_NULL,
                     world, &message_counts, &count_window);
}

void Node::finalize_rma_window()
{
    MPI_Win_free(&window);
    MPI_Win_free(&count_window);
}

void Node::relax(Vertex u, Vertex v, DVar d, int bucket_th)
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
        Message mes = {v,d};
        que.dest.push_back(destination);
        que.mess_arr.push_back(mes);
        if (que.mess_arr.size() > MAX_QUE_SIZE)
        {
            synchronize_rma();
        }
    }
}

void Node::synchronize_rma()
{
    int flag = all_reduce(&global_synchro, MPI_INT, MPI_MIN);
    if (flag)
    {
        global_synchro = 0;
        return;
    }
    std::fill(message_counts, message_counts + size_world, 0);
    MPI_Win_fence(0, window);
    MPI_Win_fence(0, count_window);
    std::vector<int> count_to_send(size_world, 0);
    for (int dest : que.dest)
    {
        count_to_send[dest]++;
    }

    for (int i = 0; i < size_world; i++)
    {
        if (count_to_send[i] > 0)
        {
            MPI_Put(&count_to_send[i], 1, MPI_INT, i,
                   rank, 1, MPI_INT, count_window);
        }
    }

    MPI_Win_fence(0, window);
    MPI_Win_fence(0, count_window);

    int msg_idx = 0;
    std::vector<int> sent(size_world,0);
    for (int dest : que.dest)
    {

        MPI_Aint target_disp = rank * MAX_QUE_SIZE * MAX_QUE_SIZE + sent[dest];
        //MPI_Win_lock(MPI_LOCK_EXCLUSIVE, dest, 0, window);
        MPI_Put(&que.mess_arr[msg_idx], 1, MPI_mess,
                dest, target_disp, 1, MPI_mess, window);
        //MPI_Win_unlock(dest, window);
        msg_idx++;
        sent[dest]++;
        assert(sent[dest]<=count_to_send[dest]);
    }
    MPI_Win_fence(0, window);
    MPI_Win_fence(0, count_window);

    for (int sender = 0; sender < size_world; sender++)
    {
        int msg_count = message_counts[sender];
        assert(msg_count<=MAX_QUE_SIZE*MAX_QUE_SIZE);
        for (int i = 0; i < msg_count; i++)
        {
            Message* mes = &shared_buffer[sender * MAX_QUE_SIZE*MAX_QUE_SIZE + i];
            assert(mes->v >= lower && mes->v <= upper);
            this->relax(0, mes->v, mes->distance);
        }
    }
    que.dest.clear();
    que.mess_arr.clear();
}
