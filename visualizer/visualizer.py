"""
Pygame visualizer for Simulation.log.

Usage:
  python visualizer.py [path/to/Simulation.log]

Controls:
  Space: play/pause
  Right/Left: step forward/backward one frame
  Up/Down: change playback speed (frames per second)
  Esc/Q: quit

What it shows:
  - Tick (top-left)
  - Building status top-right: Name (0/1 or 1/1)
  - Global items bottom-left: I# = inventory (need)
  - Resource points: solid triangles, 4 distinct colors
  - Buildings: squares, 6 distinct colors; filled if completed, outline if not;
    Storage is double size, black outline with label.
  - NPCs: colored circles, one color per agent (cycled palette)
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

    resource_points = []  # list of dict: {id,item,x,y}
    buildings = {}        # id -> BuildingState
    frames = []           # list of dict per tick

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
                    done = (bid == 256)  # Storage is completed at start
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
                frames.append({
                    "tick": tick,
                    "npcs": positions,
                    "needs": needs,
                    "inv": inv,
                    "building_done": deepcopy(building_done)
                })
    return resource_points, list(buildings.values()), frames


def world_bounds(resource_points, buildings):
    xs = [rp["x"] for rp in resource_points] + [b.x for b in buildings]
    ys = [rp["y"] for rp in resource_points] + [b.y for b in buildings]
    return (min(xs + [0]), max(xs + [1]), min(ys + [0]), max(ys + [1]))


def color_cycle():
    palette = [
        (230, 57, 70),   # red
        (29, 161, 242),  # blue
        (67, 160, 71),   # green
        (255, 193, 7),   # amber
        (156, 39, 176),  # purple
        (0, 150, 136),   # teal
        (255, 87, 34),   # deep orange
        (121, 85, 72),   # brown
        (63, 81, 181),   # indigo
        (0, 188, 212),   # cyan
    ]
    idx = 0
    while True:
        yield palette[idx % len(palette)]
        idx += 1


def draw_triangle(surface, color, center, size):
    cx, cy = center
    pts = [
        (cx, cy - size),
        (cx - size, cy + size),
        (cx + size, cy + size),
    ]
    pygame.draw.polygon(surface, color, pts)


def draw_outline_square(surface, color, rect, width=2):
    pygame.draw.rect(surface, color, rect, width)


def main():
    log_path = sys.argv[1] if len(sys.argv) > 1 else "Simulation.log"
    resource_points, buildings, frames = parse_log(log_path)
    if not frames:
        print("No frames parsed from log.")
        return

    pygame.init()
    font = pygame.font.SysFont(None, 16)
    big_font = pygame.font.SysFont(None, 22)

    win_w, win_h = 1000, 1000
    margin = 40
    min_x, max_x, min_y, max_y = world_bounds(resource_points, buildings)
    scale_x = (win_w - 2 * margin) / float(max_x - min_x + 1)
    scale_y = (win_h - 2 * margin) / float(max_y - min_y + 1)
    scale = min(scale_x, scale_y)

    def to_screen(x, y):
        sx = margin + (x - min_x) * scale
        sy = margin + (y - min_y) * scale
        return int(sx), int(sy)

    # Colors
    res_colors = [(220, 20, 60), (30, 144, 255), (46, 204, 113), (255, 152, 0)]
    bld_colors = [(142, 36, 170), (0, 172, 193), (255, 202, 40),
                  (233, 30, 99), (0, 121, 107), (205, 102, 29)]
    npc_colors = color_cycle()
    npc_color_map = []

    screen = pygame.display.set_mode((win_w, win_h))
    clock = pygame.time.Clock()
    fps = 30
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
            idx = (idx + 1) % len(frames)

        frame = frames[idx]
        screen.fill((245, 245, 245))

        # Resource points
        for rp in resource_points:
            color = res_colors[(rp["item"] - 1) % len(res_colors)]
            draw_triangle(screen, color, to_screen(rp["x"], rp["y"]), 8)

        # Buildings
        for i, b in enumerate(buildings):
            done = frame["building_done"].get(b.id, b.completed)
            size = 18
            color = bld_colors[i % len(bld_colors)]
            rect = pygame.Rect(0, 0, size * (2 if b.id == 256 else 1), size * (2 if b.id == 256 else 1))
            rect.center = to_screen(b.x, b.y)
            if done:
                pygame.draw.rect(screen, color, rect)
            else:
                draw_outline_square(screen, color, rect, 2)
            if b.id == 256:
                pygame.draw.rect(screen, (0, 0, 0), rect, 2)
                label = font.render("Storage", True, (0, 0, 0))
                screen.blit(label, label.get_rect(center=rect.center))

        # NPCs
        if len(npc_color_map) < len(frame["npcs"]):
            for _ in range(len(frame["npcs"]) - len(npc_color_map)):
                npc_color_map.append(next(npc_colors))
        for idx_npc, pos in enumerate(frame["npcs"]):
            pygame.draw.circle(screen, npc_color_map[idx_npc], to_screen(pos[0], pos[1]), 8)

        # Text: tick
        tick_text = big_font.render(f"Tick {frame['tick']}", True, (0, 0, 0))
        screen.blit(tick_text, (10, 10))

        # Text: buildings status top-right
        bx = win_w - 200
        by = 10
        for b in sorted(buildings, key=lambda x: x.id):
            done = frame["building_done"].get(b.id, b.completed)
            line = f"{b.name} ({1 if done else 0}/1)"
            surf = font.render(line, True, (0, 0, 0))
            screen.blit(surf, (bx, by))
            by += 16

        # Text: items bottom-left
        needs = frame["needs"]
        inv = frame["inv"]
        y_items = win_h - 80
        x_items = 10
        lines = []
        for item_id in sorted(set(list(needs.keys()) + list(inv.keys()))):
            line = f"I{item_id} = {inv.get(item_id,0)} (need {needs.get(item_id,0)})"
            lines.append(line)
        if not lines:
            lines.append("No item stats")
        for i, line in enumerate(lines[:6]):
            surf = font.render(line, True, (0, 0, 0))
            screen.blit(surf, (x_items, y_items + i * 16))

        pygame.display.flip()
        clock.tick(fps)


if __name__ == "__main__":
    main()
