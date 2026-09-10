import numpy as np
import pandas as pd
from pyrosm import OSM

from config import PROJECT_ROOT, OSM_FILE

NODES_FILE = PROJECT_ROOT / "data" / "processed" / "nodes.csv"
OUTPUT_FILE = PROJECT_ROOT / "data" / "processed" / "facilities.csv"

def get_facility_type(row):
    amenity = row.get("amenity")
    emergency = row.get("emergency")
    if amenity in ["hospital", "fire_station", "police"]:
        if amenity == "police":
            return "police_station"
        return amenity
    if emergency == "ambulance_station":
        return emergency

    return None

def find_nearest_node(
    latitude,
    longitude,
    node_latitudes,
    node_longitudes,
    node_ids
):
    earth_radius_m = 6_371_000

    latitude_radians = np.radians(latitude)
    node_latitudes_radians = np.radians(node_latitudes)

    latitude_difference = (
        node_latitudes_radians - latitude_radians
    )

    longitude_difference = np.radians(
        node_longitudes - longitude
    )

    a = (
        np.sin(latitude_difference / 2) ** 2
        + np.cos(latitude_radians)
        * np.cos(node_latitudes_radians)
        * np.sin(longitude_difference / 2) ** 2
    )

    distances = (
        2
        * earth_radius_m
        * np.arcsin(np.sqrt(a))
    )

    nearest_index = np.argmin(distances)

    return (
        node_ids[nearest_index],
        distances[nearest_index]
    )

def main():
    print(f"Loading OSM data: {OSM_FILE}")

    osm = OSM(str(OSM_FILE))

    facilities = osm.get_data_by_custom_criteria(
        custom_filter={
            "amenity": [
                "hospital",
                "fire_station",
                "police",
            ],
            "emergency": [
                "ambulance_station",
            ],
        },
        tags_as_columns=[
            "name",
            "amenity",
            "emergency",
            "operator",
        ],
        keep_nodes=True,
        keep_ways=True,
        keep_relations=True,
    )

    print(
        f"Found {len(facilities):,} potential facilities"
    )

    nodes = pd.read_csv(NODES_FILE)

    node_ids = nodes["id"].to_numpy()
    node_latitudes = nodes["lat"].to_numpy()
    node_longitudes = nodes["lon"].to_numpy()

    processed_facilities = []

    for _, facility in facilities.iterrows():

        facility_type = get_facility_type(facility)

        if facility_type is None:
            continue

        geometry = facility.geometry

        if geometry is None or geometry.is_empty:
            continue

        point = geometry.representative_point()

        longitude = point.x
        latitude = point.y

        nearest_node_id, road_node_distance = find_nearest_node(
            latitude,
            longitude,
            node_latitudes,
            node_longitudes,
            node_ids,
        )

        name = facility.get("name")

        if pd.isna(name):
            name = ""

        processed_facilities.append({
            "osm_id": facility["id"],
            "type": facility_type,
            "name": name,
            "latitude": latitude,
            "longitude": longitude,
            "road_node_id": nearest_node_id,
            "road_node_distance_m": road_node_distance,
        })

    output = pd.DataFrame(processed_facilities)

    output = output.drop_duplicates(
        subset=["osm_id", "type"]
    )

    output.to_csv(
        OUTPUT_FILE,
        index=False
    )

    print(
        f"Saved {len(output):,} facilities to {OUTPUT_FILE}"
    )

    print("\nFacilities by type:")
    print(output["type"].value_counts())

    print("\nRoad-node distance statistics:")

    print(
        output["road_node_distance_m"].describe()
    )


if __name__ == "__main__":
    main()