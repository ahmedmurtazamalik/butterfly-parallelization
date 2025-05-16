import networkx as nx
import matplotlib.pyplot as plt

# Read nodes
U_nodes = set()
I_nodes = set()
with open("field.dat", "r") as f:
    for line in f:
        node_id, node_type = line.strip().split()
        if node_type == 'u':
            U_nodes.add(f"u{node_id}")
        else:
            I_nodes.add(f"i{node_id}")

# Read edges
edges = []
with open("edges.dat", "r") as f:
    for line in f:
        u, i, weight = line.strip().split()
        edges.append((u, i, int(weight)))

# Create bipartite graph
B = nx.Graph()
B.add_nodes_from(U_nodes, bipartite=0)
B.add_nodes_from(I_nodes, bipartite=1)
B.add_weighted_edges_from(edges)

# Position nodes for bipartite layout
pos = {}
pos.update((node, (0, i)) for i, node in enumerate(U_nodes))
pos.update((node, (1, i)) for i, node in enumerate(I_nodes))

# Draw the graph
plt.figure(figsize=(8, 6))
nx.draw(B, pos, with_labels=True, node_color='lightblue', edge_color='gray', node_size=1500, font_size=12)
labels = nx.get_edge_attributes(B, 'weight')
nx.draw_networkx_edge_labels(B, pos, edge_labels=labels)
plt.title("Bipartite Graph Visualization")
plt.axis("off")
plt.tight_layout()
plt.show()
