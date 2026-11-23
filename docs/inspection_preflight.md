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
