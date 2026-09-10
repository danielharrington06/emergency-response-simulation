from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
OSM_FILE = PROJECT_ROOT / "data" / "osm" / "county.osm.pbf" # replace county with the filename