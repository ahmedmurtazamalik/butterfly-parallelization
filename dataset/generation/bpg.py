import random
import os

# Configuration for the bipartite graph
num_u_nodes = 2500  # Number of nodes in U set
num_i_nodes = 2500  # Number of nodes in I set
num_edges = 100000    # Total number of edges

# Create node data
node_lines = []
for i in range(num_u_nodes):
    node_lines.append(f"{i} u")
for i in range(num_i_nodes):
    node_id = num_u_nodes + i
    node_lines.append(f"{node_id} i")

# Generate valid edges ensuring all references are correct
edge_lines = set()
while len(edge_lines) < num_edges:
    u_id = random.randint(0, num_u_nodes - 1)
    i_id = num_u_nodes + random.randint(0, num_i_nodes - 1)
    weight = random.randint(1, 10)
    edge = f"u{u_id} i{i_id} {weight}"
    edge_lines.add(edge)

# Save to files
nodes_path = "nodes.dat"
edges_path = "edges.dat"

with open(nodes_path, "w") as f:
    f.write("\n".join(node_lines))

with open(edges_path, "w") as f:
    f.write("\n".join(sorted(edge_lines)))

nodes_path, edges_path