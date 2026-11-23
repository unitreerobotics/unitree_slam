import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from inspection_observation import screen, touched_cells


def scene():
    return dict(schema=1, units="m", frame_id="synthetic_map", cell_size_m=1,
                camera=dict(height_m=0.5, vertical_fov_deg=60), footprint_radius_m=0.2,
                range_m=[1, 5], known_cells=[[x, y] for x in range(-1, 7) for y in range(-1, 3)],
                navigation_cells=[], visibility_cells=[],
                targets=[dict(id="target", xyz=[4.5, 0.5, 0.5])],
                candidates=[dict(id="candidate", node_id=1, xyz_ground=[0.5, 0.5, 0])])


class ObservationTests(unittest.TestCase):
    def test_clear_candidate_has_target_facing_yaw(self):
        result = screen(scene())
        self.assertTrue(result["passed"])
        self.assertAlmostEqual(result["targets"][0]["selected"]["yaw_rad"], 0)

    def test_low_obstacle_outside_footprint_does_not_block_sight(self):
        data = scene()
        data["navigation_cells"] = [[2, 0]]
        self.assertTrue(screen(data)["passed"])

    def test_navigation_obstacle_rejects_even_provided_candidate(self):
        data = scene()
        data["navigation_cells"] = [[0, 0]]
        result = screen(data)
        self.assertFalse(result["passed"])
        self.assertIn("footprint_obstacle", result["targets"][0]["candidates"][0]["reasons"])

    def test_wall_blocks_sight(self):
        data = scene()
        data["visibility_cells"] = [[2, 0]]
        self.assertFalse(screen(data)["passed"])

    def test_footprint_tangent_to_lower_cell_boundary_is_blocked(self):
        data = scene()
        data["footprint_radius_m"] = 0.5
        data["navigation_cells"] = [[-1, 0]]
        self.assertFalse(screen(data)["passed"])

    def test_unknown_ray_cell_is_rejected(self):
        data = scene()
        data["known_cells"].remove([2, 0])
        self.assertFalse(screen(data)["passed"])

    def test_high_target_outside_camera_fov(self):
        data = scene()
        data["targets"][0]["xyz"][2] = 10
        self.assertFalse(screen(data)["passed"])

    def test_local_ground_height_changes_elevation(self):
        data = scene()
        data["targets"][0]["xyz"][2] = 10.5
        data["candidates"][0]["xyz_ground"][2] = 10
        self.assertTrue(screen(data)["passed"])

    def test_short_range_is_not_relaxed(self):
        data = scene()
        data["range_m"] = [1, 2]
        self.assertIsNone(screen(data)["targets"][0]["selected"])

    def test_corner_touching_cells_are_included(self):
        cells = touched_cells((0.5, 0.5), (2.5, 2.5), 1)
        self.assertIn((1, 0), cells)
        self.assertIn((0, 1), cells)

    def test_duplicate_ids_rejected(self):
        data = scene()
        data["candidates"].append(copy.deepcopy(data["candidates"][0]))
        with self.assertRaises(ValueError):
            screen(data)

    def test_nan_calibration_rejected(self):
        data = scene()
        data["camera"]["height_m"] = float("nan")
        with self.assertRaises(ValueError):
            screen(data)

    def test_empty_targets_does_not_pass(self):
        data = scene()
        data["targets"] = []
        self.assertFalse(screen(data)["passed"])


if __name__ == "__main__":
    unittest.main()
