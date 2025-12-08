# TaskAllocation Simulation

简述：从 SQLite 数据库加载物品/配方/建筑，生成任务树（Gather/Craft/Build），用调度器做缺口估价 + bundle 竞价分配，模拟器按 20 tick/s 驱动 NPC 采集、制造、建造并输出日志（`Simulation.log`）。  
接口总览请见 [接口说明](docs/INTERFACES.md)；更细的内部成员说明见 [详细说明](docs/DETAIL_REFERENCE.md)。  
项目概览与可视化说明见 [Project Overview](docs/PROJECT_OVERVIEW.md)，常用自定义操作见 [User Tweaks](docs/USER_TWEAKS.md)。

## 运行方式
- 配置依赖：CMake + SQLite3（已通过 `resources/game_data.db` 提供数据）。
- 构建：`cmake -S . -B build && cmake --build build`
- 运行：`./build/TaskFramework`（日志输出到工作目录的 `Simulation.log`）。

## 实现要点
- 数据加载：`DatabaseManager` 读取 Items/Buildings/Crafting/ResourcePoints，填充 `WorldState`。
- 任务树：`TaskTree::buildFromDatabase` 递归展开配方，生成带父子关系的节点（Gather/Craft/Build），支持缺口查询、事件回写、建筑子树“退役”。
- 调度：`Scheduler` 计算缺口（含材料折算），CBBA 风格竞价，按得分分配给空闲 NPC，预扣批次材料。
- 模拟：`Simulator` 每 5 秒重分配，逐 tick 处理移动/采集/制作/建造，事件写回 `TaskTree` 与 `WorldState`，并记录简易调试日志（缺口、就绪、阻塞、分配、事件）。
- 工人：`initDefaultWorkers` 统一创建工人（速度 180，曼哈顿移动，背包共享全局资源）。

## 日志
`Simulation.log` 内容：资源点/建筑初始位置；周期性的缺口/就绪/阻塞列表；任务分配；采集/制作/建造事件；NPC 位置与基础物资缺口摘要。

## 目录提示
- `includes/`：头文件（接口定义）
- `src/`：实现
- `resources/`：数据库与生成脚本
- `docs/DETAIL_REFERENCE.md`：完整类/函数说明（含私有成员）***

## ESP32-S3 可视化联动（Wi-Fi + 逻辑控制）

在不改动核心调度/模拟逻辑的前提下，本项目额外提供了一个 ESP32-S3 联动展示：  
PC 端可视化读取 `Simulation.log` 中的 NPC 任务状态，通过 Wi‑Fi 将“1 号 NPC（NPC0）当前在做什么”发送给 ESP32-S3，ESP 板载 RGB LED 用不同颜色表示不同任务类型。

### 行为逻辑（任务 → 灯光）

PC 端在 `visualizer/visualizer.py` 中解析每个 tick 的 `Tasks:` 字段，例如：

```
... | Tasks: A0:G1 A1:C5 A2:B10003 ...
```

其中：
- `A0` 表示第一个 NPC（这里作为“1 号 NPC”）。
- 后缀的首字母表示任务类型：
  - `G` 开头：Gather（采集/挖矿）
  - `C` 开头：Craft（制造）
  - `B` 开头：Build（建造）

映射到 ESP 端的字符串与灯光颜色如下：
- `idle`：1 号 NPC 当前没有任务 → LED 关闭 `(0, 0, 0)`。
- `gather`：执行采集/挖矿任务 → 绿色 `(0, 120, 0)`。
- `craft`：执行制造任务 → 橙色 `(255, 140, 0)`。
- `build`：执行建造任务 → 蓝色 `(0, 80, 200)`。

PC 端只在状态变化时发送一次消息给 ESP，避免不必要的网络负载。

### 通信方式

- 协议：UDP（轻量、无需握手，足够用来做状态广播）。
- 拓扑：ESP32-S3 连接同一 Wi‑Fi 热点（例如手机热点或路由器），PC 也连接到同一网络；PC 的可视化脚本通过 UDP 将状态发到 ESP 的 IP 和固定端口。
- 默认端口：`8765`。

ESP32-S3 端（MicroPython）：
- 在 `visualizer/esp_main.py` 中：
  - 使用 `network.WLAN(STA_IF)` 连接热点：
    - SSID：`"YOUR_WIFI_SSID"`
    - 密码：`"YOUR_WIFI_PASSWORD"`
  - 连接成功后打印 IP 信息，并用：
    ```python
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("0.0.0.0", UDP_PORT))
    ```
    在 `UDP_PORT = 8765` 上监听 PC 发来的状态字符串（如 `"gather"`、`"craft"` 等）。
  - 通过 `neopixel.NeoPixel(machine.Pin(LED_PIN), 1)` 控制单颗 RGB LED，不同消息切换为不同颜色。

PC 端（可视化脚本）：
- 在 `visualizer/visualizer.py` 中有一段配置（仅此文件参与 ESP 联动）：
  ```python
  ESP_ENABLED = True
  ESP_HOST = "ESP32_IP_HERE"  # 运行时改成你的 ESP32-S3 实际 IP
  ESP_PORT = 8765
  ```
- 每帧从帧数据中读取 `A0:` 项，解析出 NPC0 当前任务类型，当状态变化时：
  ```python
  esp_sock.sendto(status.encode("utf-8"), (ESP_HOST, ESP_PORT))
  ```
  其中 `status` 是 `"idle" / "gather" / "craft" / "build"` 之一。

### ESP32-S3 使用说明（教程摘要）

1. **准备固件与工具**
   - 在 ESP32-S3 上刷入 MicroPython 固件。
   - 在 PC 上安装 `ampy` 或类似工具，用于上传文件：
     ```bash
     pip install adafruit-ampy
     ```

2. **将脚本放入 ESP 根目录**
   - 使用项目中提供的 `visualizer/esp_main.py`，上传到 ESP32-S3 根目录，并命名为 `boot.py`（或 `main.py`，ESP 启动时先运行 `boot.py`，再运行 `main.py`，任选其一即可）：
     ```bash
     ampy --port /dev/tty.usbserial-XXXX put visualizer/esp_main.py boot.py
     ```
   - 建议备份已有同名文件（例如已有 `boot.py` 或 `main.py` 时，可以手动下载为 `.bak`）。

3. **连接 Wi‑Fi 并确认 IP**
   - 重启 ESP32-S3（按板子上的 RESET 键）。
   - 通过串口监视器连接 ESP（例如：`screen /dev/tty.usbserial-XXXX 115200`），启动时会打印类似：
     ```text
     WiFi: ('192.168.0.10', '255.255.255.0', '192.168.0.1', '192.168.0.1')
     Listening UDP on port 8765
     ```
   - 将打印出的 IP（示例为 `192.168.0.10`）填入 `visualizer/visualizer.py` 中的 `ESP_HOST`。

4. **PC 端配置并运行可视化**
- 确保 PC 连接与 ESP32-S3 相同的热点/路由器。
   - 修改 `visualizer/visualizer.py` 中的：
     ```python
     ESP_HOST = "ESP32_IP_HERE"  # 替换为上一步串口看到的 IP
     ```
   - 使用模拟后的 `Simulation.log` 运行可视化：
     ```bash
     python visualizer/visualizer.py Simulation.log
     ```
   - 播放过程中，当 1 号 NPC（NPC0）在 idle/gather/craft/build 间切换时，ESP 串口会打印收到的状态，板载 RGB LED 会对应变色。

5. **可选参数**
   - 若要暂停网络联动，只需在 `visualizer/visualizer.py` 中将：
     ```python
     ESP_ENABLED = True
     ```
     改为：
     ```python
     ESP_ENABLED = False
     ```
   - 若你的开发板 RGB LED 使用的引脚不同，可以在 `visualizer/esp_main.py` 中调整：
     ```python
     LED_PIN = 48
     LED_COUNT = 1
     ```
     为对应的引脚号与 LED 数量。

通过以上配置，本项目在原有“任务调度 + 仿真 + 日志 + 2D 可视化”的基础上，实现了“通过 Wi‑Fi 与 ESP32-S3 互动”的扩展：  
在展示环节，老师可以直观看到 1 号 NPC 的任务状态如何驱动实体 LED 的颜色变化，实现逻辑控制输出与网络通信两个能力点。  
