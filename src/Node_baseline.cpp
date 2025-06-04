#include "Node.hpp"

void Node::run()
{
    int k = 0;
    int global_k;
    int work_to_do;
    do
    {
        while (k < buckets.size() && (!buckets[k] || buckets[k]->empty())) k++;
        // std::cout<<k<<std::endl;
        if (k == MAX)
            break;

        k = all_reduce(&k,MPI_MIN);
        do
        {
            if (k < buckets.size() && buckets[k])
            {
                Bucket& bucket = *buckets[k];
                while (!bucket.empty())
                {
                    int u = bucket.front();
                    bucket.pop_front();
                    for (int i = 0; i < distances.size();i++)
                    {
                        if (vertex_1[i] == u)
                        {
                            int v = vertex_2[i];
                            int d = distances[i];
                            relax(u,v,d + tenative[u-lower]);
                        }
                    }

               }
            }
            synchronize();
            work_to_do = (k < buckets.size() && buckets[k] && !buckets[k]->empty());
        }
        while (all_reduce(&work_to_do,MPI_LOR));

    k++;
    }
    while (true);
}
