import argparse
import networkit as nk
import os
import numpy as np



def generate_graph(args):
    # Number of nodes and edges

    # R-MAT probabilities for quadrants (a, b, c, d)
    # Common values: a=0.57, b=0.19, c=0.19, d=0.05
    rmat_gen = nk.generators.RmatGenerator(args.scale, args.edge_factor, 0.57, 0.19, 0.19, 0.05)
    G = rmat_gen.generate()
    cc = nk.components.ConnectedComponents(G)
    cc.run()
    G = nk.components.ConnectedComponents.extractLargestConnectedComponent(G, compactGraph=True)

    weighted_graph = nk.Graph(G.numberOfNodes(), weighted=True, directed=G.isDirected())
    for u, v in G.iterEdges():
        weight = np.random.randint(0,args.w+1)
        weighted_graph.addEdge(u, v, weight)
    N = G.numberOfNodes()
    nodes_per_part = N // args.k
    remainder = N % args.k


    source = 0
    if args.true:
        sssp = nk.distance.Dijkstra(weighted_graph, source)
        sssp.run()
        distances = sssp.getDistances()

    start = 0
    for pid in range(args.k):
        extra = 1 if pid < remainder else 0
        end = start + nodes_per_part + extra - 1

            # Write the partition to a file
        filename = f"{pid}.in"
        file_out = f"{pid}_f.out"
        with open(os.path.join(args.p,filename), "w") as f:
            f.write(f"{N} ")
            f.write(f"{start} {end}\n")
            for u in range(start, end + 1):
                neighbors = G.iterNeighbors(u)
                for v in neighbors:
                    distance = weighted_graph.weight(u,v)
                    f.write(f"{u} {v} {int(distance)}\n")

        if args.true:
            with open(os.path.join(args.p,file_out),"w") as f:
                for u in range(start, end + 1):
                    f.write(f"{int(distances[u])}\n")

        start = end + 1


if __name__=="__main__":
    parser = argparse.ArgumentParser(description="Generate Graph")
    parser.add_argument("-s","--scale",type = int,required=True)
    parser.add_argument("-e","--edge-factor",type = int,required=True)
    parser.add_argument("-k",type = int,required=True)
    parser.add_argument("-p",type=str,default=".")
    parser.add_argument("-w",type=int,default=100)
    parser.add_argument("--true", action='store_true', help="generate true distances")
    args = parser.parse_args()
    generate_graph(args)
