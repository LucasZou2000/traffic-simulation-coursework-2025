"""
Pygame visualizer for Simulation.log (2K window 2560x1600).

Usage:
  python visualizer_2k.py [path/to/Simulation.log]

Controls:
  Space: play/pause
  Right/Left: step forward/backward one frame
  Up/Down: change playback speed (frames per second)
  Esc/Q: quit
"""
import sys
import re
import pygame
from copy import deepcopy


class BuildingState:
    def __init__(self, bid, name, x, y, completed=False):
        self.id = bid
        self.name = name
        self.x = x
        self.y = y
        self.completed = completed


def parse_log(path):
    with open(path, "r") as f:
        lines = [ln.rstrip("\n") for ln in f]

    resource_points = []
    buildings = {}
    frames = []
    state = "start"
    building_done = {}
    tick_re = re.compile(r"\[Tick\s+(\d+)\]\s+NPCs:\s+(.*)")
    pos_re = re.compile(r"\((-?\d+),(-?\d+)\)")
    needs_re = re.compile(r"I(\d+):(\d+)/(\d+)")

    for line in lines:
        if not line:
            continue
        if line.startswith("ResourcePoints:"):
            state = "rp"
            continue
        if state == "rp":
            if line.startswith("Buildings:"):
                state = "buildings"
                continue
            m = re.match(r"RP\s+(\d+)\s+item\s+(\d+)\s+at\s+\((-?\d+),(-?\d+)\)", line)
            if m:
                rid, item, x, y = map(int, m.groups())
                resource_points.append({"id": rid, "item": item, "x": x, "y": y})
            continue
        if state == "buildings":
            if line.startswith("[Tick"):
                state = "body"
            else:
                m = re.match(r"B\s+(\d+)\s+(.+)\s+at\s+\((-?\d+),(-?\d+)\)", line)
                if m:
                    bid = int(m.group(1))
                    name = m.group(2)
                    x = int(m.group(3))
                    y = int(m.group(4))
                    done = (bid == 256)
                    buildings[bid] = BuildingState(bid, name, x, y, done)
                    building_done[bid] = done
                continue
        if state == "body":
            if "built building" in line:
                m = re.search(r"built building (\d+)", line)
                if m:
                    bid = int(m.group(1))
                    building_done[bid] = True
                continue
            m_tick = tick_re.match(line)
            if m_tick:
                tick = int(m_tick.group(1))
                rest = m_tick.group(2)
                positions = [(int(a), int(b)) for a, b in pos_re.findall(rest)]
                needs_match = needs_re.findall(line)
                needs = {}
                inv = {}
                for item_id, need_v, inv_v in needs_match:
                    needs[int(item_id)] = int(need_v)
                    inv[int(item_id)] = int(inv_v)
                tasks = []
                m_tasks = re.search(r"Tasks:\s*(.*)$", line)
                if m_tasks:
                    task_part = m_tasks.group(1).strip()
                    if task_part and task_part != "None":
                        tasks = task_part.split()
                frames.append({
                    "tick": tick,
                    "npcs": positions,
                    "needs": needs,
                    "inv": inv,
                    "tasks": tasks,
                    "building_done": deepcopy(building_done)
                })
    return resource_points, list(buildings.values()), frames


def world_bounds(resource_points, buildings):
    xs = [rp["x"] for rp in resource_points] + [b.x for b in buildings]
    ys = [rp["y"] for rp in resource_points] + [b.y for b in buildings]
    return (min(xs + [0]), max(xs + [1]), min(ys + [0]), max(ys + [1]))


def color_cycle():
    palette = [
        (230, 57, 70), (29, 161, 242), (67, 160, 71), (255, 193, 7),
        (156, 39, 176), (0, 150, 136), (255, 87, 34), (121, 85, 72),
        (63, 81, 181), (0, 188, 212),
    ]
    idx = 0
    while True:
        yield palette[idx % len(palette)]
        idx += 1


def draw_triangle(surface, color, center, size):
    cx, cy = center
    pts = [(cx, cy - size), (cx - size, cy + size), (cx + size, cy + size)]
    pygame.draw.polygon(surface, color, pts)


def draw_triangle_outline(surface, color, center, size, width=2):
    cx, cy = center
    pts = [(cx, cy - size), (cx - size, cy + size), (cx + size, cy + size)]
    pygame.draw.polygon(surface, color, pts, width)


def draw_outline_square(surface, color, rect, width=2):
    pygame.draw.rect(surface, color, rect, width)


def main():
    log_path = sys.argv[1] if len(sys.argv) > 1 else "Simulation.log"
    resource_points, buildings, frames = parse_log(log_path)
    if not frames:
        print("No frames parsed from log.")
        return

    pygame.init()
    font = pygame.font.SysFont(None, 32)
    big_font = pygame.font.SysFont(None, 44)

    win_w, win_h = 2560, 1600
    margin = 40
    min_x, max_x, min_y, max_y = world_bounds(resource_points, buildings)
    world_w = float(max_x - min_x + 1)
    world_h = float(max_y - min_y + 1)
    scale_x = (win_w - 2 * margin) / world_w
    scale_y = (win_h - 2 * margin) / world_h
    scale = min(scale_x, scale_y)
    offset_x = margin + (win_w - 2 * margin - world_w * scale) / 2.0
    offset_y = margin + (win_h - 2 * margin - world_h * scale) / 2.0

    def to_screen(x, y):
        sx = offset_x + (x - min_x) * scale
        sy = offset_y + (y - min_y) * scale
        return int(sx), int(sy)

    res_colors = [(220, 20, 60), (30, 144, 255), (46, 204, 113), (255, 152, 0)]
    bld_colors = [(142, 36, 170), (0, 172, 193), (255, 202, 40),
                  (233, 30, 99), (0, 121, 107), (205, 102, 29)]
    npc_palette = [
        (66, 133, 244), (52, 168, 83), (251, 188, 5),
        (234, 67, 53), (171, 71, 188), (0, 188, 212),
        (255, 112, 67), (124, 179, 66), (57, 73, 171)
    ]
    npc_colors = color_cycle()
    npc_color_pool = iter(npc_palette)
    npc_color_map = []

    screen = pygame.display.set_mode((win_w, win_h))
    clock = pygame.time.Clock()
    fps = 30
    speed = 15  # ticks per frame
    paused = False
    idx = 0

    while True:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                return
            if event.type == pygame.KEYDOWN:
                if event.key in (pygame.K_ESCAPE, pygame.K_q):
                    pygame.quit()
                    return
                if event.key == pygame.K_SPACE:
                    paused = not paused
                if event.key == pygame.K_RIGHT:
                    idx = min(len(frames) - 1, idx + 1)
                if event.key == pygame.K_LEFT:
                    idx = max(0, idx - 1)
                if event.key == pygame.K_UP:
                    fps = min(120, fps + 5)
                if event.key == pygame.K_DOWN:
                    fps = max(5, fps - 5)

        if not paused:
            idx = (idx + speed) % len(frames)

        frame = frames[idx]
        screen.fill((245, 245, 245))

        for rp in resource_points:
            color = res_colors[(rp["item"] - 1) % len(res_colors)]
            pos = to_screen(rp["x"], rp["y"])
            draw_triangle(screen, color, pos, 16)
            draw_triangle_outline(screen, (0, 0, 0), pos, 16, 2)

        for i, b in enumerate(buildings):
            done = frame["building_done"].get(b.id, b.completed)
            size = 32
            color = bld_colors[i % len(bld_colors)]
            rect = pygame.Rect(0, 0, size, size)
            rect.center = to_screen(b.x, b.y)
            if done:
                pygame.draw.rect(screen, color, rect)
                pygame.draw.rect(screen, (0, 0, 0), rect, 2)
            else:
                draw_outline_square(screen, color, rect, 2)
            if b.id == 256:
                pygame.draw.rect(screen, (0, 0, 0), rect, 2)
                label = font.render("Storage", True, (0, 0, 0))
                screen.blit(label, label.get_rect(center=rect.center))

        if len(npc_color_map) < len(frame["npcs"]):
            for _ in range(len(frame["npcs"]) - len(npc_color_map)):
                try:
                    npc_color_map.append(next(npc_color_pool))
                except StopIteration:
                    npc_color_map.append(next(npc_colors))
        for idx_npc, pos in enumerate(frame["npcs"]):
            center = to_screen(pos[0], pos[1])
            pygame.draw.circle(screen, npc_color_map[idx_npc], center, 16)
            pygame.draw.circle(screen, (0, 0, 0), center, 16, 2)

        tick_text = big_font.render(f"Tick {frame['tick']}", True, (0, 0, 0))
        screen.blit(tick_text, (10, 10))

        bx = win_w - 240
        by = 12
        for b in sorted(buildings, key=lambda x: x.id):
            done = frame["building_done"].get(b.id, b.completed)
            line = f"{b.name} ({1 if done else 0}/1)"
            surf = font.render(line, True, (0, 0, 0))
            screen.blit(surf, (bx, by))
            by += 28

        needs = frame["needs"]
        inv = frame["inv"]
        y_items = win_h - 320
        x_items = 16
        lines = []
        for item_id in sorted(set(list(needs.keys()) + list(inv.keys()))):
            line = f"I{item_id} = {inv.get(item_id,0)} (need {needs.get(item_id,0)})"
            lines.append(line)
        if not lines:
            lines.append("No item stats")
        for i, line in enumerate(lines[:10]):
            surf = font.render(line, True, (0, 0, 0))
            screen.blit(surf, (x_items, y_items + i * 28))

        tasks = frame.get("tasks", [])
        tx = win_w - 300
        ty = win_h - 320
        if tasks:
            title = font.render("Tasks", True, (0, 0, 0))
            screen.blit(title, (tx, ty))
            ty += 28
            for i, tstr in enumerate(tasks[:10]):
                surf = font.render(tstr, True, (0, 0, 0))
                screen.blit(surf, (tx, ty + i * 28))

        pygame.display.flip()
        clock.tick(fps)


if __name__ == "__main__":
    main()
