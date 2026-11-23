# Offline inspection mission bundle preflight

This tool uses an intermediate mission format to check file digests, target associations and graph reachability before execution.

## Run
From the repository root, with Python 3.8+ and the standard library:
```bash
python3 tools/inspection_preflight.py examples/inspection/preflight/mission.json
python3 -B -m unittest discover -s tests -p test_inspection_preflight.py -v
```
Exit codes: 0 = all specified checks pass; 1 = validation fails; 2 = input/read error. The report includes errors and unverified.

## Format
Use schema=1 and units=m; frame_id names the mission frame. directed and return_required must be explicit booleans.
- map/config: file is relative to the directory containing mission.json; sha256 is the expected lowercase SHA-256. Files are hashed in 64 KiB chunks. Absolute paths, parent-directory escapes and symlinks resolving outside the bundle are rejected. Referenced files are read as data.
- nodes: unique uint16 id, xyz (three finite numbers) and finite yaw_rad.
- edges: from/to reference existing nodes; enabled is an explicit boolean. directed determines graph direction; omitting it fails validation.
- visit_order: a nonempty node sequence. Each consecutive leg must be graph-reachable; return_required adds a check from the last node to the first. Intermediate graph nodes and retracing are allowed.
- targets: a nonempty list with unique nonempty string IDs; each node_id must occur in the visit order.

## Usage notes
The intermediate format explicitly represents graph direction and edge state through directed and enabled. Adapting Unitree Node/Edge messages requires translating the corresponding fields. The example uses a text map identifier to make digest changes easy to inspect.
The report covers file consistency and reachability in the declared graph. Coordinate transforms, geometric clearance and camera visibility belong to their respective modules. Geometric crossing audits and visit-order optimization are possible extensions.

---

# 中文

# 巡检任务包离线预检示例

本工具使用中间任务格式，在任务执行前检查文件摘要、目标关联及路网可达性。

## 运行
Python 3.8+，仅标准库，仓库根目录：
```bash
python3 tools/inspection_preflight.py examples/inspection/preflight/mission.json
python3 -B -m unittest discover -s tests -p test_inspection_preflight.py -v
```
退出码 0=指定检查全部通过，1=校验不通过，2=输入／读取错误。报告含 errors 和 unverified。

## 格式
schema=1，units=m，frame_id 指定任务坐标系名称。directed 和 return_required 必须显式为布尔值。
- map/config: file 为相对 mission.json 所在目录的文件名；sha256 为预期小写 SHA-256。摘要按 64KiB 流式读取；绝对路径、父目录越界及解析后位于包外的符号链接均拒绝。文件按数据读取。
- nodes: uint16 id、xyz（三个有限数值）、yaw_rad（有限数值），ID 不可重复。
- edges: from/to 必须引用已有节点，enabled 必须显式为布尔值；direction 统一由 directed 控制。未设置方向时校验失败。
- visit_order: 非空节点序列。每个连续任务段都必须图可达，开启返程时还检查末点到首点；允许中间经过未列入访问序列的图节点及正常复走。
- targets: 非空目标列表，字符串 id 唯一，node_id 必须在访问序列中。

## 使用说明
中间格式通过 directed 和 enabled 明确图方向与边状态。适配官方 Node/Edge 消息时，需要将对应字段转换为这一格式。样例用文本标识代替地图文件，方便核对摘要变化。
报告覆盖文件一致性和声明路网中的可达性；坐标变换、几何净空和相机视线应由对应模块检查。几何交叉审计和访问顺序优化可作为后续扩展。
