#include "Node.hpp"

void Node::run()
{
    int k = 0;
    int work_to_do;
    std::vector<bool> was_deleted(N,false);
    do
    {
        while (k < buckets.size() && (!buckets[k] || buckets[k]->empty()))
            k++;

        if (k >= buckets.size())
            k = MAX;

        k = all_reduce(&k,MPI_MIN);

        if (k == MAX)
            break;
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
                    if (!was_deleted[u])
                    {
                        was_deleted[u] = true;
                        deleted.push_back(u);
                    }
                    for (const auto& [v, d] : adjacency_list[u]) {
                        if (d <= Delta)
                            relax(u,v,d + tenative[u-lower]);
                    }

               }
            }
            synchronize();
            work_to_do = (k < buckets.size() && buckets[k] && !buckets[k]->empty());
        }
        while (all_reduce(&work_to_do,MPI_LOR));

        //Heavy reduction
        for (auto u:deleted)
        {
            for (const auto& [v, d] : adjacency_list[u]) {
                if (d > Delta)
                    relax(u,v,d + tenative[u-lower]);
            }
        }
        synchronize();
    k++;
    }
    while (true);
}
