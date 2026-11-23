"""Validate an explicit offline inspection graph/bundle; never contact a robot."""
import argparse
import hashlib
import json
import math
from pathlib import Path


def validate(data, root):
    if not isinstance(data, dict):
        raise ValueError("input must be an object")
    errors = []
    if type(data.get("schema")) is not int or data["schema"] != 1 or data.get("units") != "m" or not isinstance(data.get("frame_id"), str) or not data["frame_id"]:
        errors.append("schema_units_or_frame")
    if type(data.get("directed")) is not bool or type(data.get("return_required")) is not bool:
        errors.append("explicit_graph_direction_and_return_policy_required")
    root = Path(root).resolve()
    for kind in ("map", "config"):
        asset = data[kind]
        path = Path(asset["file"])
        resolved = (root / path).resolve()
        if path.is_absolute() or root not in resolved.parents:
            errors.append(kind + "_outside_bundle")
            continue
        digest = asset["sha256"]
        if not isinstance(digest, str) or len(digest) != 64 or any(c not in "0123456789abcdef" for c in digest):
            errors.append(kind + "_invalid_digest")
            continue
        try:
            hasher = hashlib.sha256()
            with resolved.open("rb") as stream:
                for chunk in iter(lambda: stream.read(65536), b""):
                    hasher.update(chunk)
            if hasher.hexdigest() != digest:
                errors.append(kind + "_digest_mismatch")
        except OSError:
            errors.append(kind + "_unreadable")
    nodes = {}
    for node in data["nodes"]:
        ident = node["id"]
        if type(ident) is not int or not 0 <= ident <= 65535:
            errors.append("invalid_node_id")
            continue
        if ident in nodes:
            errors.append("duplicate_node_id")
        values = node["xyz"] + [node["yaw_rad"]]
        if len(node["xyz"]) != 3 or any(isinstance(v, bool) or not isinstance(v, (int, float)) or not math.isfinite(v) for v in values):
            errors.append("invalid_node_pose")
        nodes[ident] = node
    graph = {n: set() for n in nodes}
    for edge in data["edges"]:
        a, b = edge["from"], edge["to"]
        if type(a) is not int or type(b) is not int or a not in nodes or b not in nodes:
            errors.append("dangling_edge")
            continue
        if type(edge.get("enabled")) is not bool:
            errors.append("explicit_edge_enabled_required")
            continue
        if edge["enabled"]:
            graph[a].add(b)
            if data.get("directed") is False:
                graph[b].add(a)
    route = data["visit_order"]
    if not route or any(type(n) is not int or n not in nodes for n in route):
        errors.append("invalid_visit_order")
    else:
        legs = list(zip(route, route[1:]))
        if data.get("return_required"):
            legs.append((route[-1], route[0]))
        for a, b in legs:
            seen, pending = {a}, [a]
            while pending:
                for neighbour in graph[pending.pop()] - seen:
                    seen.add(neighbour)
                    pending.append(neighbour)
            if b not in seen:
                errors.append("unreachable_leg:%s->%s" % (a, b))
    target_ids = set()
    for target in data["targets"]:
        ident = target["id"]
        if not isinstance(ident, str) or not ident or ident in target_ids:
            errors.append("invalid_or_duplicate_target_id")
        target_ids.add(ident)
        if type(target["node_id"]) is not int or target["node_id"] not in route:
            errors.append("target_not_in_visit_order")
    if not data["targets"]:
        errors.append("no_inspection_targets")
    return dict(schema=1, passed=not errors, errors=errors,
                scope="bundle_digests_and_declared_graph_only",
                unverified=["physical_clearance", "localization", "camera_visibility", "firmware_compatibility"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mission")
    args = parser.parse_args()
    path = Path(args.mission)
    try:
        result = validate(json.loads(path.read_text(encoding="utf-8")), path.parent)
    except (ValueError, KeyError, TypeError, OSError) as error:
        print(json.dumps({"passed": False, "error": str(error)}))
        return 2
    print(json.dumps(result, indent=2))
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
