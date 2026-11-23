"""Strict, offline candidate screening on explicitly supplied XY masks."""
import argparse
import json
import math


def number(value):
    if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
        raise ValueError("expected a finite number")
    return float(value)


def touched_cells(a, b, size):
    """Closed-cell segment intersection, including corner-touching cells."""
    cells = set()
    for x in range(math.floor(min(a[0], b[0]) / size) - 1, math.floor(max(a[0], b[0]) / size) + 1):
        for y in range(math.floor(min(a[1], b[1]) / size) - 1, math.floor(max(a[1], b[1]) / size) + 1):
            lo, hi = 0.0, 1.0
            for start, end, cell in zip(a, b, (x, y)):
                delta = end - start
                if delta == 0:
                    if not cell * size <= start <= (cell + 1) * size:
                        lo, hi = 1.0, 0.0
                        break
                else:
                    t0 = (cell * size - start) / delta
                    t1 = ((cell + 1) * size - start) / delta
                    lo, hi = max(lo, min(t0, t1)), min(hi, max(t0, t1))
            if lo <= hi:
                cells.add((x, y))
    return cells


def screen(data):
    if not isinstance(data, dict):
        raise ValueError("input must be an object")
    if type(data.get("schema")) is not int or data["schema"] != 1 or data.get("units") != "m" or not isinstance(data.get("frame_id"), str) or not data["frame_id"]:
        raise ValueError("schema=1, units=m and frame_id are required")
    size = number(data["cell_size_m"])
    camera = data["camera"]
    height, vfov = number(camera["height_m"]), number(camera["vertical_fov_deg"])
    radius = number(data["footprint_radius_m"])
    near, far = map(number, data["range_m"])
    if not (size > 0 and height > 0 and 0 < vfov < 180 and radius > 0 and 0 < near <= far):
        raise ValueError("invalid geometry parameters")

    def mask(name):
        values = data[name]
        if any(len(c) != 2 or any(type(i) is not int for i in c) for c in values):
            raise ValueError("mask cells must be integer XY pairs")
        return {tuple(c) for c in values}

    known, nav, los = mask("known_cells"), mask("navigation_cells"), mask("visibility_cells")
    if not nav <= known or not los <= known:
        raise ValueError("occupied cells must be known")
    targets, candidates = data["targets"], data["candidates"]
    for group in (targets, candidates):
        ids = [item["id"] for item in group]
        if any(not isinstance(i, str) or not i for i in ids) or len(set(ids)) != len(ids):
            raise ValueError("IDs must be unique nonempty strings")
    result = []
    for target in targets:
        tx, ty, tz = map(number, target["xyz"])
        reports = []
        for candidate in candidates:
            x, y, ground = map(number, candidate["xyz_ground"])
            if type(candidate["node_id"]) is not int or not 0 <= candidate["node_id"] <= 65535:
                raise ValueError("node_id must be a uint16 supplied by the caller")
            distance = math.hypot(tx - x, ty - y)
            elevation = math.degrees(math.atan2(tz - ground - height, distance))
            footprint = set()
            for ix in range(math.floor((x - radius) / size) - 1, math.floor((x + radius) / size) + 1):
                for iy in range(math.floor((y - radius) / size) - 1, math.floor((y + radius) / size) + 1):
                    dx = max(ix * size - x, 0, x - (ix + 1) * size)
                    dy = max(iy * size - y, 0, y - (iy + 1) * size)
                    if math.hypot(dx, dy) <= radius:
                        footprint.add((ix, iy))
            ray = touched_cells((x, y), (tx, ty), size)
            # The target's own cell is excluded explicitly; no arbitrary final-distance margin.
            ray.discard((math.floor(tx / size), math.floor(ty / size)))
            failures = []
            if not near <= distance <= far:
                failures.append("range")
            if abs(elevation) > vfov / 2:
                failures.append("camera_elevation")
            if footprint - known or ray - known:
                failures.append("unknown_space")
            if footprint & nav:
                failures.append("footprint_obstacle")
            if ray & los:
                failures.append("sight_obstacle")
            reports.append(dict(candidate_id=candidate["id"], node_id=candidate["node_id"],
                                distance_m=distance, yaw_rad=math.atan2(ty-y, tx-x),
                                elevation_deg=elevation, accepted=not failures, reasons=failures))
        accepted = [r for r in reports if r["accepted"]]
        selected = min(accepted, key=lambda r: (r["distance_m"], r["candidate_id"])) if accepted else None
        result.append(dict(target_id=target["id"], selected=selected, candidates=reports))
    return dict(schema=1, scope="offline_xy_mask_candidate_screening", frame_id=data["frame_id"],
                passed=bool(result) and all(r["selected"] is not None for r in result), targets=result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input")
    args = parser.parse_args()
    try:
        with open(args.input, encoding="utf-8") as stream:
            result = screen(json.load(stream))
    except (ValueError, KeyError, TypeError, OSError) as error:
        print(json.dumps({"passed": False, "error": str(error)}))
        return 2
    print(json.dumps(result, indent=2))
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
