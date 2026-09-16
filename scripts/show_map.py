import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path


# ------------------------------------------------------------
# Configuration
# ------------------------------------------------------------

NODES_FILE = Path("data/processed/nodes.csv")
EDGES_FILE = Path("data/processed/edges.csv")
ROUTE_FILE = Path("data/processed/route.txt")

OUTPUT_FILE = Path("route_visualisation.png")


# ------------------------------------------------------------
# Load data
# ------------------------------------------------------------

print("Loading nodes...")
nodes = pd.read_csv(NODES_FILE)

print("Loading edges...")
edges = pd.read_csv(EDGES_FILE)

print(f"Nodes: {len(nodes):,}")
print(f"Edges: {len(edges):,}")


# ------------------------------------------------------------
# Load route
# ------------------------------------------------------------

with open(ROUTE_FILE) as file:
    route = [int(line.strip()) for line in file if line.strip()]

print(f"Route nodes: {len(route):,}")


# ------------------------------------------------------------
# Identify node ID / coordinate columns
# ------------------------------------------------------------

print("\nNode columns:")
print(nodes.columns.tolist())

print("\nEdge columns:")
print(edges.columns.tolist())


# ------------------------------------------------------------
# Create lookup from node ID -> coordinates
# ------------------------------------------------------------

node_lookup = nodes.set_index("id")[["lat", "lon"]]


# ------------------------------------------------------------
# Plot road network
# ------------------------------------------------------------

fig, ax = plt.subplots(figsize=(14, 14))

print("Plotting road network...")

# Draw every edge in the extracted graph.
#
# This is deliberately simple: for each edge, draw a line
# between its source and destination coordinates.
for edge in edges.itertuples(index=False):

    try:
        source = node_lookup.loc[edge.source]
        destination = node_lookup.loc[edge.target]
    except KeyError:
        continue

    ax.plot(
        [source["lon"], destination["lon"]],
        [source["lat"], destination["lat"]],
        linewidth=0.25,
        alpha=0.25
    )


# ------------------------------------------------------------
# Plot route
# ------------------------------------------------------------

route_coordinates = []

for node_id in route:
    try:
        node = node_lookup.loc[node_id]
    except KeyError:
        print(f"Warning: node {node_id} not found")
        continue

    route_coordinates.append(
        (node["lon"], node["lat"])
    )


route_lons = [coordinate[0] for coordinate in route_coordinates]
route_lats = [coordinate[1] for coordinate in route_coordinates]


ax.plot(
    route_lons,
    route_lats,
    linewidth=2.5,
    label="A* route"
)


# ------------------------------------------------------------
# Start and destination
# ------------------------------------------------------------

if route_coordinates:

    ax.scatter(
        route_lons[0],
        route_lats[0],
        s=100,
        zorder=5,
        label="Start"
    )

    ax.scatter(
        route_lons[-1],
        route_lats[-1],
        s=100,
        zorder=5,
        label="Destination"
    )


# ------------------------------------------------------------
# Labels
# ------------------------------------------------------------

ax.set_xlabel("Longitude")
ax.set_ylabel("Latitude")
ax.set_title("Emergency Response Simulation - Route Visualisation")

ax.legend()

ax.set_aspect("equal")

plt.tight_layout()

plt.savefig(
    OUTPUT_FILE,
    dpi=300
)

print(f"\nSaved visualisation to {OUTPUT_FILE}")

plt.show()