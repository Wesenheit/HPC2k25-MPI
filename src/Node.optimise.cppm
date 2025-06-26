module;
#include <list>
#include <mpi.h>
#include <unordered_map>
#include <vector>

export module Node:optimise;

import :interface;

void Node::run_opt(float tau) {
  int k = 0;
  int work_to_do;
  std::vector<bool> was_deleted(N, false);
  initialize_rma_window();
  int settled_vertices = 0;
  int global_settled = 0;
  int bucket_th = -1;
  do {
    while (k < buckets.size() && (!buckets[k] || buckets[k]->empty()))
      k++;

    if (k >= buckets.size())
      k = MAX;

    k = all_reduce(&k, MPI_INT, MPI_MIN);

    if (k == MAX)
      break;

    global_settled = all_reduce(&settled_vertices, MPI_INT, MPI_SUM);
    if (global_settled > tau * N) {
      bucket_th = k;
      int k_new = k + 1;

      while (k_new < buckets.size() && buckets[k] && buckets[k_new]) {
        Bucket &bucket = *buckets[k_new];
        while (!bucket.empty()) {
          int u = bucket.front();
          bucket.pop_front();
          buckets[k]->push_back(u);
        }
        k_new++;
      }
    }
    deleted.clear();
    do {
      if (k < buckets.size() && buckets[k]) {
        Bucket &bucket = *buckets[k];
        while (!bucket.empty()) {
          int u = bucket.front();
          bucket.pop_front();
          if (!was_deleted[u]) {
            was_deleted[u] = true;
            if (bucket_th < 0)
              deleted.push_back(u);
            settled_vertices++;
          }
          for (const auto &[v, d] : adjacency_list[u]) {
            if (d <= Delta || bucket_th > 0)
              relax(u, v, d + tenative[u - lower], bucket_th);
          }
        }
      }
      synchronize(bucket_th);
      work_to_do = (k < buckets.size() && buckets[k] && !buckets[k]->empty());
    } while (all_reduce(&work_to_do, MPI_INT, MPI_LOR));
    // Heavy reduction
    for (auto u : deleted) {
      for (const auto &[v, d] : adjacency_list[u]) {
        if (d > Delta)
          relax(u, v, d + tenative[u - lower], bucket_th);
      }
    }
    synchronize(bucket_th);
    MPI_Barrier(world);
    k++;
  } while (true);
  finalize_rma_window();
}
