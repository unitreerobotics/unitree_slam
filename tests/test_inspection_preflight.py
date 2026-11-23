import hashlib
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from inspection_preflight import validate


class PreflightTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        assets = {}
        for name in ("map", "config"):
            path = self.root / (name + ".txt")
            path.write_text("synthetic " + name, encoding="utf-8")
            assets[name] = dict(file=path.name, sha256=hashlib.sha256(path.read_bytes()).hexdigest())
        self.data = dict(schema=1, units="m", frame_id="synthetic_map", directed=True,
                         return_required=True, **assets,
                         nodes=[dict(id=i, xyz=[i, 0, 0], yaw_rad=0) for i in (1, 2)],
                         edges=[dict(**{"from": 1, "to": 2}, enabled=True),
                                dict(**{"from": 2, "to": 1}, enabled=True)],
                         visit_order=[1, 2], targets=[dict(id="photo", node_id=2)])

    def errors(self):
        return validate(self.data, self.root)["errors"]

    def test_valid_round_trip(self):
        self.assertTrue(validate(self.data, self.root)["passed"])

    def test_directed_return_is_checked(self):
        self.data["edges"].pop()
        self.assertIn("unreachable_leg:2->1", self.errors())

    def test_explicit_undirected_edge_allows_return(self):
        self.data["directed"] = False
        self.data["edges"].pop()
        self.assertEqual(self.errors(), [])

    def test_disabled_edge_is_not_traversed(self):
        self.data["edges"][1]["enabled"] = False
        self.assertIn("unreachable_leg:2->1", self.errors())

    def test_dangling_edge(self):
        self.data["edges"][0]["to"] = 99
        self.assertIn("dangling_edge", self.errors())

    def test_duplicate_node(self):
        self.data["nodes"].append(self.data["nodes"][0])
        self.assertIn("duplicate_node_id", self.errors())

    def test_nan_pose(self):
        self.data["nodes"][0]["xyz"][2] = float("nan")
        self.assertIn("invalid_node_pose", self.errors())

    def test_modified_map(self):
        (self.root / "map.txt").write_text("changed")
        self.assertIn("map_digest_mismatch", self.errors())

    def test_outside_bundle_path(self):
        self.data["map"]["file"] = "../external.txt"
        self.assertIn("map_outside_bundle", self.errors())

    def test_missing_frame(self):
        del self.data["frame_id"]
        self.assertIn("schema_units_or_frame", self.errors())

    def test_target_not_visited(self):
        self.data["targets"][0]["node_id"] = 99
        self.assertIn("target_not_in_visit_order", self.errors())

    def test_direction_not_inferred(self):
        del self.data["directed"]
        self.assertIn("explicit_graph_direction_and_return_policy_required", self.errors())


if __name__ == "__main__":
    unittest.main()
