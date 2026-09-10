from pathlib import Path
from pyrosm import OSM

PROJECT_ROOT = Path(__file__).resolve().parents[1]

OSM_FILE = PROJECT_ROOT / "data" / "osm" / "hertfordshire.osm.pbf"
OUTPUT_DIR = PROJECT_ROOT / "data" / "processed"

ROAD_TYPES = {
    "motorway",
    "motorway_link",
    "trunk",
    "trunk_link",
    "primary",
    "primary_link",
    "secondary",
    "secondary_link",
    "tertiary",
    "tertiary_link",
    "unclassified",
    "residential",
    "living_street",
    "busway",
}

def main():
    print(f"Loading: {OSM_FILE}")

    osm = OSM(str(OSM_FILE))

    nodes, edges = osm.get_network(
        nodes = True, 
        network_type="driving"
    )

    print(f"Loaded {len(nodes):,} nodes and {len(edges):,} edges")

    processed_edges = edges[
        edges["highway"].isin(ROAD_TYPES)
    ].copy()

    print("\nHighway types:")
    print(edges["highway"].value_counts().head(30))

    print("\nSample edges:")
    print(
        edges[
            ["u", "v", "length", "highway", "maxspeed", "oneway", "name"]
        ].head(10).to_string()
    )

    processed_nodes = nodes[
        ["id", "lat", "lon"]
    ].copy()

    processed_edges = edges[
        ["u", "v", "length", "highway", "maxspeed", "oneway", "name"]
    ].copy()

    processed_edges = processed_edges.rename(columns={
        "u": "source",
        "v": "target",
        "length": "length_miles",
    })

    processed_edges["length_miles"] /= 1609.344 # change from metres to miles

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    nodes_file = OUTPUT_DIR / "nodes.csv"
    edges_file = OUTPUT_DIR / "edges.csv"

    processed_nodes.to_csv(nodes_file, index=False)
    processed_edges.to_csv(edges_file, index=False)

    print(f"Saved {len(processed_nodes):,} nodes to {nodes_file}")
    print(f"Saved {len(processed_edges):,} edges to {edges_file}")

if __name__ == "__main__":
    main()