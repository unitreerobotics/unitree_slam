from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from inspection_replay import Replay, replay


def event(seq, kind, stamp=None, **extra):
    return dict(seq=seq, t_s=seq if stamp is None else stamp, event=kind,
                mission_id="synthetic", waypoint_id="one", **extra)


class ReplayTests(unittest.TestCase):
    def setUp(self):
        self.machine = Replay("synthetic", "one", 10)

    def feed(self, *kinds):
        for seq, kind in enumerate(kinds):
            self.machine.feed(event(seq, kind))
        return self.machine.state

    def test_clearance_resume_arrival(self):
        self.assertEqual(self.feed("near_stop", "clearance", "resume_ack", "arrived"), "COMPLETED")

    def test_recovery_verify_resume_arrival(self):
        self.assertEqual(self.feed("near_stop", "persistent_blockage", "recovery_done", "resume_ack", "arrived"), "COMPLETED")

    def test_arrival_without_resume_ack_faults(self):
        self.assertEqual(self.feed("near_stop", "clearance", "arrived"), "FAULTED")

    def test_identical_duplicate_ignored(self):
        sample = event(0, "near_stop")
        self.machine.feed(sample)
        result = self.machine.feed(sample)
        self.assertEqual(result["reason"], "duplicate_ignored")
        self.assertEqual(self.machine.state, "HOLDING")

    def test_conflicting_duplicate_faults(self):
        self.machine.feed(event(0, "near_stop"))
        self.machine.feed(event(0, "clearance"))
        self.assertEqual(self.machine.state, "FAULTED")

    def test_time_regression_faults(self):
        self.machine.feed(event(0, "near_stop", 2))
        self.machine.feed(event(1, "clearance", 1))
        self.assertEqual(self.machine.state, "FAULTED")

    def test_deadline_boundary_faults(self):
        self.machine.feed(event(0, "near_stop", 0))
        self.machine.feed(event(1, "clearance", 10))
        self.assertEqual(self.machine.history[-1]["reason"], "deadline_exceeded")

    def test_localization_loss_is_terminal(self):
        self.feed("localization_lost", "arrived")
        self.assertEqual(self.machine.state, "FAULTED")

    def test_cancel_is_terminal(self):
        self.assertEqual(self.feed("cancel", "arrived"), "CANCELLED")

    def test_wrong_waypoint_faults(self):
        sample = event(0, "arrived")
        sample["waypoint_id"] = "other"
        self.machine.feed(sample)
        self.assertEqual(self.machine.state, "FAULTED")

    def test_truncated_log_is_incomplete(self):
        result = replay(dict(schema=1, mission_id="synthetic", waypoint_id="one", timeout_s=10,
                             events=[event(0, "near_stop")]))
        self.assertFalse(result["completed"])
        self.assertTrue(result["incomplete"])

    def test_nonfinite_timestamp_rejected(self):
        with self.assertRaises(ValueError):
            self.machine.feed(event(0, "arrived", float("nan")))


if __name__ == "__main__":
    unittest.main()
