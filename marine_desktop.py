import pygame
import requests
import tkinter as tk
from tkinter import ttk
import threading
import time

# Настройки подключения
ROV_IP = '192.168.13.129'  # <-- IP лодки
CONTROL_URL = f'http://{ROV_IP}:8000/control'

# Настройки управления
THROTTLE_STEP = 5  # шаг изменения при нажатии кнопки (~10%)
SEND_INTERVAL = 0.1  # Интервал отправки данных

# Инициализация Pygame
pygame.init()
pygame.joystick.init()

joystick = pygame.joystick.Joystick(0)
joystick.init()

class JoystickReader(threading.Thread):
    def __init__(self, gui):
        super().__init__(daemon=True)
        self.gui = gui
        self.running = True

        self.motor_throttle = 0
        self.pump_throttle = 0
        self.pump2_throttle = 0
        self.lights_on = False

        self.last_button_states = [0] * joystick.get_numbuttons()
        self.last_axis_value = 0.0
        self.last_button_x = 0

    def run(self):
        while self.running:
            pygame.event.pump()

            # Чтение оси мотора (например, ось 1 — левый стик вверх/вниз)
            axis_value = joystick.get_axis(1)  # -1 вверх, +1 вниз
            if abs(axis_value - self.last_axis_value) > 0.1:
                delta = -axis_value * 25  # инвертируем, если нужно
                self.motor_throttle += int(delta)
                self.motor_throttle = max(-255, min(255, self.motor_throttle))
                self.last_axis_value = axis_value
                time.sleep(0.1)  # защита от дребезга

            # Управление помпой 1 (LB увеличить)
            if joystick.get_button(4) and not self.last_button_states[4]:
                self.pump_throttle = min(255, self.pump_throttle + 25)
                time.sleep(0.1)

            # Управление помпой 2 (RB увеличить)
            if joystick.get_button(5) and not self.last_button_states[5]:
                self.pump2_throttle = min(255, self.pump2_throttle + 25)
                time.sleep(0.1)

            # Управление светом (X кнопка — 2)
            button_x = joystick.get_button(2)
            if button_x and not self.last_button_x:
                self.lights_on = not self.lights_on
                self.send_lights()
                time.sleep(0.1)
            self.last_button_x = button_x

            # Обновляем кнопки
            for i in range(joystick.get_numbuttons()):
                self.last_button_states[i] = joystick.get_button(i)

            # Отправка команд
            self.send_control()
            time.sleep(0.1)

    def send_control(self):
        motor_value = int(self.motor_throttle)
        pump_value = int(self.pump_throttle)
        pump2_value = int(self.pump2_throttle)

        if -10 < motor_value < 10:
            motor_value = 0

        payload = {
            "M": motor_value,
            "P1": pump_value,
            "P2": pump2_value
        }

        try:
            requests.post(CONTROL_URL, json=payload, timeout=0.2)
            self.gui.log_message(f"Sent: {payload}")
        except Exception as e:
            self.gui.log_message(f"Error: {e}")

    def send_lights(self):
        payload = {"L": int(self.lights_on)}
        try:
            requests.post(CONTROL_URL, json=payload, timeout=0.2)
            self.gui.log_message(f"Lights {'ON' if self.lights_on else 'OFF'}")
        except Exception as e:
            self.gui.log_message(f"Error sending lights: {e}")

    def stop(self):
        self.running = False


class MarineDiveGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Marine Dive - Управление лодкой")

        self.motor_label = ttk.Label(root, text="Мотор: 0", width=20)
        self.motor_label.pack(pady=5)

        self.pump_label = ttk.Label(root, text="Помпа 1: 0", width=20)
        self.pump_label.pack(pady=5)

        self.pump2_label = ttk.Label(root, text="Помпа 2: 0", width=20)
        self.pump2_label.pack(pady=5)

        self.lights_label = ttk.Label(root, text="Фонари: OFF", width=20)
        self.lights_label.pack(pady=5)

        self.log = tk.Text(root, height=10, width=50)
        self.log.pack(pady=10)

        self.reader = JoystickReader(self)
        self.reader.start()

        self.update_loop()

        self.root.protocol("WM_DELETE_WINDOW", self.on_close)

    def update_loop(self):
        self.motor_label.config(text=f"Мотор: {self.reader.motor_throttle}")
        self.pump_label.config(text=f"Помпа 1: {self.reader.pump_throttle}")
        self.pump2_label.config(text=f"Помпа 2: {self.reader.pump2_throttle}")
        self.lights_label.config(text=f"Фонари: {'ON' if self.reader.lights_on else 'OFF'}")
        self.root.after(100, self.update_loop)

    def log_message(self, message):
        self.log.insert(tk.END, f"{message}\n")
        self.log.see(tk.END)

    def on_close(self):
        self.reader.stop()
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = MarineDiveGUI(root)
    root.mainloop()
