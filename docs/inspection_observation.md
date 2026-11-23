# Offline inspection observation candidate screening

This example checks navigation clearance and target visibility separately to select observation candidates for inspection targets.

## Run
From the repository root, with Python 3.8+ and the standard library:
```bash
python3 tools/inspection_observation.py examples/inspection/observation.json
python3 -B -m unittest discover -s tests -p test_inspection_observation.py -v
```
Exit codes: 0 = every target has a candidate; 1 = a target has no candidate, or there are no targets; 2 = input/read error. JSON records rejection reasons, target-facing yaw and elevation.

## Input format
The example scene is synthetic. Use schema=1 and units=m; all inputs share the frame specified by frame_id.
- XY grid origin is (0,0); cell_size_m is the cell width. Cell (i,j) covers [i*s,(i+1)*s] × [j*s,(j+1)*s]; boundary contact is treated conservatively.
- known_cells lists known cells; others are unknown. Caller-supplied navigation_cells and visibility_cells must be subsets of known_cells.
- targets: id and xyz, the 3D target coordinates.
- candidates: id, caller-supplied uint16 node_id, and xyz_ground (candidate XY and local ground Z).
- camera.height_m is camera height above local ground. vertical_fov_deg is the full symmetric vertical field of view. The model assumes a horizontal optical axis and camera XY coincident with candidate XY. Configure parameters using the camera's calibration.
- footprint_radius_m is the circular footprint radius; range_m is the inclusive horizontal observation-distance interval.

Every candidate is checked against the navigation mask. Sight checks intersect the segment with closed cells, including corner contacts, and exclude the target's own cell. Unknown space rejects the candidate. Elevation must lie inside the configured vertical field of view. Qualifying candidates are ordered by horizontal distance, then ID.

## Usage notes
Low obstacles can occupy only navigation_cells; walls can occupy visibility_cells. The caller prepares masks, local ground, candidates and camera parameters. The tool uses an XY height-band visibility criterion and a horizontal optical-axis camera model.
The selected node_id is passed through for node association and subsequent route checks. Point-cloud grid construction, Dijkstra routing and node publication are possible extensions.
The example uses a 1 m grid for manual inspection; configure map resolution and device dimensions for the application.

---

# 中文

# 离线巡检观测候选筛选示例

本示例将导航净空与目标视线分开检查，为巡检目标选择合适的观测候选位姿。

## 运行
在仓库根目录执行（Python 3.8+，仅标准库）：
```bash
python3 tools/inspection_observation.py examples/inspection/observation.json
python3 -B -m unittest discover -s tests -p test_inspection_observation.py -v
```
退出码：0 所有目标均有候选；1 至少一个目标没有候选或无目标；2 输入／读取错误。JSON 记录每个候选的拒绝原因、目标朝向 yaw 和仰角。

## 输入格式
样例为合成场景。schema=1；units=m；所有输入使用 frame_id 指定的同一地图坐标系。
- XY 栅格原点固定为 (0,0)，cell_size_m 为边长；整数格 (i,j) 覆盖 [i*s,(i+1)*s] × [j*s,(j+1)*s]，边界接触按保守占用处理。
- known_cells 列出已知格；未列出的格视为未知。navigation_cells 和 visibility_cells 必须是已知格的子集，由调用方提供。
- targets: id 和 xyz（三维目标坐标）。
- candidates: id、node_id（调用方提供的 uint16 编号）、xyz_ground（候选 XY 及当地地面 Z）。Z 表示当地地面高度。
- camera.height_m 为相机相对当地地面的高度，vertical_fov_deg 为显式给定的完整对称垂直视场；假设光轴水平、相机 XY 与候选 XY 重合。接入具体相机时应按其标定参数配置。
- footprint_radius_m 为圆形足迹半径，range_m 为闭区间水平观测距离。

每个候选均检查足迹与导航 mask。视线使用闭格线段相交，包含仅角点接触的格；视线检查排除目标自身所在格。穿过未知格即拒绝。仰角须位于配置垂直视场内。合格候选按水平距离最近、ID 字典序择一。

## 使用说明
低障碍可只进入 navigation_cells，墙体可进入 visibility_cells。调用方负责准备 mask、局部地面、候选和相机参数；工具采用 XY 高度带视线判据与水平光轴相机模型。
node_id 随选定候选输出，供节点关联及后续路由检查使用。点云栅格构建、Dijkstra 路由及节点发布可作为后续扩展。
样例采用 1m 栅格，便于手工核对；实际应用按地图分辨率和设备尺寸配置。
