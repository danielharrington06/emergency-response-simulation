from pathlib import Path
from pyrosm import OSM

PROJECT_ROOT = Path(__file__).resolve().parents[1]

OSM_FILE = PROJECT_ROOT / "data" / "osm" / "hertfordshire.osm.pbf"

def main():
    print(f"Loading: {OSM_FILE}")

    osm = OSM(str(OSM_FILE))

    nodes, edges = osm.get_network(nodes = True, network_type="driving")

    print(f"Nodes: {len(nodes):,}")
    print(f"Edges: {len(edges):,}")

    print("\nNode columns:")
    print(nodes.columns.tolist())

    print("\nEdge columns:")
    print(edges.columns.tolist())

if __name__ == "__main__":
    main()