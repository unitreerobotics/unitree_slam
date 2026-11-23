# 障碍响应离线事件回放示例

本示例通过单路点事件回放，检查障碍等待、恢复确认和续行到达的状态转换。输入样例使用合成事件。

## 运行
Python 3.8+，仅标准库：
```bash
python3 tools/inspection_replay.py examples/inspection/recovery.json
python3 -B -m unittest discover -s tests -p test_inspection_replay.py -v
```
退出码 0=回放报告 COMPLETED；1=故障、取消或日志未结束；2=输入／读取错误。状态及转换原因写入 JSON 报告。

## 输入格式
顶层 schema=1、mission_id、waypoint_id、timeout_s、events。
每个事件包含非负整数 seq、相对同一单调时钟的有限非负 t_s、相同 mission_id/waypoint_id 和 event。
接入 qt_notice 等反馈时，将相应消息转换为下列事件。
- NAVIGATING + near_stop → HOLDING。
- HOLDING + clearance → RESUME_REQUESTED；resume_ack 后才回到 NAVIGATING。
- HOLDING + persistent_blockage → RECOVERY_REQUESTED；recovery_done 后进入 VERIFYING；resume_ack 后才回到 NAVIGATING。
- 仅 NAVIGATING 接受 arrived → COMPLETED。COMPLETED 表示该路点的流程完成。
- cancel → CANCELLED；localization_lost/transport_lost → FAULTED。
- 完全相同的序号和事件内容重发被忽略；未结束时冲突序号、倒序、时钟回退、身份不符或不允许的事件使回放 FAULTED。
- 等待态每次进入时以事件时间加 timeout_s 设置截止时间；任何新事件到达／超过截止时间均使其 FAULTED。tick 只推进事件时钟。截止规则优先于同一时刻的其他事件。
- 终态保持不变，后续事件记为忽略；日志截断明确标记 incomplete。

## 使用说明
当前工具处理离线事件和状态转换。接入实时系统时，需要补充消息适配、命令确认和计时器；超时检查由输入事件或 tick 推进。
FAULTED 是应用事件状态，机器人停止应由运行系统确认。后续可扩展多路点调度及实时事件处理。
样例 timeout_s=10 用于展示截止时间行为，实际应用按任务要求配置。
