#include "Lookup.hpp"
#include "Node.hpp"
#include <cstddef>

void Node::run_opt(float tau)
{
    max = all_reduce(&max,MPI_LONG_LONG_INT,MPI_MAX);
    int k = 0;
    int work_to_do;
    std::vector<bool> was_deleted(N,false);
    int settled_vertices = 0;
    int global_settled = 0;
    int bucket_th = -1;
    int total_pull = 0;
    int total_push = 0;
    do
    {
        while (k < buckets.size() && (!buckets[k] || buckets[k]->empty()))
            k++;

        if (k >= buckets.size())
            k = MAX;

        k = all_reduce(&k,MPI_INT,MPI_MIN);

        if (k == MAX)
            break;

        global_settled = all_reduce(&settled_vertices, MPI_INT,MPI_SUM);
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
        deleted.clear();
        do
        {
            if (k < buckets.size() && buckets[k])
            {
                Bucket& bucket = *buckets[k];
                while (!bucket.empty())
                {
                    int u = bucket.front();
                    bucket.pop_front();
                    if (!was_deleted[u])
                    {
                        was_deleted[u] = true;
                        settled_vertices++;
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
        while (all_reduce(&work_to_do,MPI_INT,MPI_LOR));


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

        std::vector<Vertex> forward;
        for (Vertex i = lower; i <=upper; i++)
        {
            if (!was_deleted[i])
            {
                forward.push_back(i);
                pull++;
            }
        }

        int glob_pull = 2 * all_reduce(&pull,MPI_INT, MPI_SUM);
        int glob_push = all_reduce(&push,MPI_INT, MPI_SUM);
        if (glob_push < glob_pull)
        {
            total_push++;
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
            total_pull++;
            synchronize();
            std::unordered_map<Vertex, std::vector<std::pair<Vertex,DVar>>> what_one_needs;
            for (Vertex v:forward)
            {
                for (const auto& [u, d] : adjacency_list[v]) {
                    if (d > Delta && d < tenative[v-lower]-k*Delta)
                    {
                        send_request(u);
                        what_one_needs[v].push_back({u,d});
                    }
                }
            }

            auto dist = accept_requests(k);
            for (auto [v,vector]:what_one_needs)
            {
                for (auto [u,d]:vector)
                {
                    assert(v>=lower && v<=upper);
                    auto it = dist.find(u);
                    if (it != dist.end())
                        relax(u, v, dist[u]+d);
                }
            }

        }
    k++;
    }
    while (true);
}
