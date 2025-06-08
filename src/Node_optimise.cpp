#include "Lookup.hpp"
#include "Node.hpp"
#include <cstddef>

bool compareMessages(const Message& m1, const Message& m2) {
    return m1.v < m2.v;
}

DVar find_given_vertex(std::vector<Message>* arr, Vertex target)
{
    int start = 0;
    int end = arr->size() - 1;

    while (start <= end)
    {
        int mid = start + (end - start) / 2;
        Vertex mid_value = (*arr)[mid].v;

        if (mid_value == target)
        {
            return (*arr)[mid].distance;
        }
        else if (mid_value < target)
        {
            start = mid + 1;
        }
        else
        {
            end = mid - 1;
        }
    }
    return -1;
}


void Node::run_opt(float tau)
{
    //max = all_reduce(&max,MPI_MAX);
    int k = 0;
    int work_to_do;
    std::vector<bool> was_deleted(N,false);
    int settled_vertices = 0;
    int global_settled = 0;
    int bucket_th = -1;
    do
    {
        while (k < buckets.size() && (!buckets[k] || buckets[k]->empty()))
            k++;

        if (k >= buckets.size())
            k = MAX;

        k = all_reduce(&k,MPI_MIN);

        if (k == MAX)
            break;

        global_settled = all_reduce(&settled_vertices, MPI_SUM);
        if (global_settled > tau * N)
        {
            bucket_th = k;
            int k_new = k+1;

            while (k_new < buckets.size() && buckets[k] && buckets[k_new])
            {
                Bucket& bucket = *buckets[k_new];
                while (!bucket.empty())
                {
                    int u = bucket.front();
                    bucket.pop_front();
                    buckets[k]->push_back(u);
                }
                k_new++;
            }
        }
        std::vector<int> deleted;
        do
        {
            if (k < buckets.size() && buckets[k])
            {
                Bucket& bucket = *buckets[k];
                while (!bucket.empty())
                {
                    int u = bucket.front();
                    bucket.pop_front();
                    settled_vertices++;
                    if (!was_deleted[u])
                    {
                        was_deleted[u] = true;
                        if (bucket_th < 0)
                            deleted.push_back(u);
                    }
                    for (const auto& [v, d] : adjacency_list[u]) {
                        if (d <= Delta || bucket_th > 0)
                            relax(u,v,d + tenative[u-lower]);
                    }
               }
            }
            synchronize();
            work_to_do = (k < buckets.size() && buckets[k] && !buckets[k]->empty());
        }
        while (all_reduce(&work_to_do,MPI_LOR));


        int k_new;

        //Estimate pull and push volumes
        int push = 0;
        int pull = 0;

        //Push
        for (auto u:deleted)
        {
            push += long_count[u];
        }
        //Pull
        k_new = k + 1;
        while (k_new < buckets.size())
        {
            if (buckets[k_new])
            {
                Bucket& bucket = *buckets[k_new];
                for (Vertex v:bucket)
                {
                    pull += adjacency_list[v].size()*(tenative[v-lower]-(k+1)*Delta)/max;
                }
            }
            k_new++;
        }
        int glob_pull = 2 * all_reduce(&pull, MPI_SUM);
        int glob_push = all_reduce(&push, MPI_SUM);

        if (glob_push < glob_pull)
        {
            //Heavy reduction
            for (auto u:deleted)
            {
                for (const auto& [v, d] : adjacency_list[u]) {
                    if (d > Delta)
                    relax(u,v,d + tenative[u-lower]);
                }
            }
            synchronize();
        }
        else
        {
            synchronize();
            k_new = k+1;
            std::unordered_map<Vertex, std::vector<std::pair<Vertex,DVar>>> what_one_needs;
            while (k_new < buckets.size())
            {
                if (buckets[k_new])
                {
                    Bucket& bucket = *buckets[k_new];
                    for (Vertex v:bucket)
                    {
                        for (const auto& [u, d] : adjacency_list[v]) {
                            if (d < tenative[v-lower]-k*Delta)
                            {
                                send_request(u);
                                what_one_needs[v].push_back({u,d});
                            }
                        }
                    }
                }
                k_new++;
            }

            auto answers = accept_requests(k);
            for (auto [v,vector]:what_one_needs)
            {
                for (auto [u,d]:vector)
                {
                    relax(u, v, answers[u]+d);
                }
            }

        }
    k++;
    }
    while (true);
}
