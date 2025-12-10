"""
ESP32-S3 (MicroPython) UDP listener to drive an RGB LED based on NPC0 task status.

Copy this file as main.py to the ESP32-S3 flash, adjust pins/SSID/password below.
"""
import machine
import network
import socket
import time

try:
    import neopixel
except ImportError:
    neopixel = None

FRAMERATE = 30
SLEEP_TIME = 1.0 / FRAMERATE

last_client_addr = None
last_heartbeat_sent = 0.0


# Wi-Fi config (fill with your hotspot/router)
WIFI_SSID = "YOUR_WIFI_SSID"
WIFI_PWD = "YOUR_WIFI_PASSWORD"

# LED config (NeoPixel single LED)
LED_PIN = 48      # adjust to your board's RGB LED pin
LED_COUNT = 1

# UDP server
UDP_PORT = 8765


def connect_wifi():
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    if not wlan.isconnected():
        wlan.connect(WIFI_SSID, WIFI_PWD)
        for _ in range(60):
            if wlan.isconnected():
                break
            time.sleep(0.5)
    print("WiFi:", wlan.ifconfig() if wlan.isconnected() else "Failed")
    return wlan


def set_color(np, color):
    if not np:
        return
    np[0] = color
    np.write()


def main():
    wlan = connect_wifi()
    np = None
    if neopixel:
        np = neopixel.NeoPixel(machine.Pin(LED_PIN), LED_COUNT)
        set_color(np, (0, 0, 0))

    global last_client_addr, last_heartbeat_sent

    colors = {
        "idle": (0, 0, 0),
        "gather": (0, 120, 0),   # green
        "craft": (255, 140, 0),  # orange
        "build": (0, 80, 200),   # blue
    }

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("0.0.0.0", UDP_PORT))
    sock.settimeout(0)
    print("Listening UDP on port", UDP_PORT)
    last = "idle"
    while True:
        now = time.time()
        try:
            data, addr = sock.recvfrom(64)
            msg = data.decode().strip()
            if msg in colors:
                last = msg
                set_color(np, colors[msg])
                print("From", addr, "->", msg)
            last_client_addr = addr
        except Exception:
            # timeout or other errors; keep last color
            pass
        if last_client_addr is not None and (now - last_heartbeat_sent) >= 1.0:
            try:
                sock.sendto(b"alive", last_client_addr)
            except Exception:
                pass
            last_heartbeat_sent = now
        time.sleep(SLEEP_TIME)


if __name__ == "__main__":
    main()
