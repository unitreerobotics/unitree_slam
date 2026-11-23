"""Offline single-waypoint event contract; no DDS, HTTP or motion commands."""
import argparse
import json
import math


class Replay:
    WAITING = {"HOLDING", "RESUME_REQUESTED", "RECOVERY_REQUESTED", "VERIFYING"}
    TERMINAL = {"COMPLETED", "FAULTED", "CANCELLED"}
    TRANSITIONS = {
        ("NAVIGATING", "near_stop"): "HOLDING",
        ("HOLDING", "clearance"): "RESUME_REQUESTED",
        ("HOLDING", "persistent_blockage"): "RECOVERY_REQUESTED",
        ("RESUME_REQUESTED", "resume_ack"): "NAVIGATING",
        ("RECOVERY_REQUESTED", "recovery_done"): "VERIFYING",
        ("VERIFYING", "resume_ack"): "NAVIGATING",
        ("NAVIGATING", "arrived"): "COMPLETED",
    }

    def __init__(self, mission_id, waypoint_id, timeout_s):
        if not isinstance(mission_id, str) or not mission_id or not isinstance(waypoint_id, str) or not waypoint_id:
            raise ValueError("explicit mission and waypoint IDs required")
        if isinstance(timeout_s, bool) or not isinstance(timeout_s, (int, float)) or not math.isfinite(timeout_s) or timeout_s <= 0:
            raise ValueError("timeout_s must be finite and positive")
        self.mission = mission_id
        self.waypoint = waypoint_id
        self.timeout = timeout_s
        self.state = "NAVIGATING"
        self.last_seq, self.last_time = -1, -1.0
        self.deadline = None
        self.seen = {}
        self.history = []

    def feed(self, event):
        if not isinstance(event, dict):
            raise ValueError("event must be an object")
        before = self.state
        reason = "transition"
        seq, stamp = event.get("seq"), event.get("t_s")
        valid = (type(seq) is int and seq >= 0 and not isinstance(stamp, bool)
                 and isinstance(stamp, (int, float)) and math.isfinite(stamp) and stamp >= 0)
        if not valid:
            raise ValueError("seq and t_s must be nonnegative finite sequence/time values")
        if seq in self.seen and self.seen[seq] == event:
            reason = "duplicate_ignored"
        elif self.state in self.TERMINAL:
            reason = "terminal_ignored"
        elif (seq <= self.last_seq or stamp < self.last_time
              or event.get("mission_id") != self.mission or event.get("waypoint_id") != self.waypoint):
            self.state, reason = "FAULTED", "event_identity_or_order"
        elif self.deadline is not None and stamp >= self.deadline:
            self.state, reason = "FAULTED", "deadline_exceeded"
        else:
            kind = event.get("event")
            if kind == "cancel":
                self.state = "CANCELLED"
            elif kind in {"localization_lost", "transport_lost"}:
                self.state, reason = "FAULTED", kind
            elif kind == "tick":
                reason = "clock_only"
            elif (self.state, kind) in self.TRANSITIONS:
                self.state = self.TRANSITIONS[(self.state, kind)]
            else:
                self.state, reason = "FAULTED", "unexpected_event"
            if self.state != before:
                self.deadline = stamp + self.timeout if self.state in self.WAITING else None
        self.last_seq = max(seq, self.last_seq)
        self.last_time = max(stamp, self.last_time)
        self.seen.setdefault(seq, dict(event))
        record = dict(seq=seq, t_s=stamp, before=before, after=self.state, reason=reason)
        self.history.append(record)
        return record


def replay(data):
    if not isinstance(data, dict):
        raise ValueError("input must be an object")
    if type(data.get("schema")) is not int or data["schema"] != 1:
        raise ValueError("schema=1 required")
    machine = Replay(data["mission_id"], data["waypoint_id"], data["timeout_s"])
    for event in data["events"]:
        machine.feed(event)
    return dict(schema=1, state=machine.state, completed=machine.state == "COMPLETED",
                incomplete=machine.state not in machine.TERMINAL, history=machine.history,
                scope="synthetic_offline_contract_not_firmware_protocol")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input")
    args = parser.parse_args()
    try:
        with open(args.input, encoding="utf-8") as stream:
            result = replay(json.load(stream))
    except (ValueError, KeyError, TypeError, OSError) as error:
        print(json.dumps({"completed": False, "error": str(error)}))
        return 2
    print(json.dumps(result, indent=2))
    return 0 if result["completed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
