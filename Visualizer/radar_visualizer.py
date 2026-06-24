#!/usr/bin/env python3
"""
Ultrasonic Radar System Visualizer
Displays real-time object mapping from STM32 serial telemetry.
Includes an automatic simulation fallback if no hardware is connected.
"""
import sys
import math
import random
import time
import pygame
import serial
from serial.tools import list_ports
# Configuration Constants
WINDOW_WIDTH = 950
WINDOW_HEIGHT = 650
FPS = 60
# Colors (Hex HSL-aligned dark-mode style)
COLOR_BG = (10, 15, 12)          # Deep dark forest green
COLOR_RADAR_GREEN = (0, 220, 70)  # Bright neon radar green
COLOR_GRID = (0, 60, 20)          # Dim grid green
COLOR_SWEEP = (0, 255, 80)        # Sweep line green
COLOR_THREAT = (255, 30, 50)      # Neon alert red
COLOR_TEXT = (200, 240, 210)      # Pale mint green
COLOR_TEXT_DIM = (100, 140, 110)  # Muted mint green
COLOR_CARD = (20, 30, 24)         # Card background
# Radar settings
MAX_DISTANCE_CM = 200             # Max distance to plot (2 meters)
RADAR_RADIUS = 480                # Radius in pixels
DANGER_THRESHOLD_CM = 20          # 20cm buzzer threshold
class RadarVisualizer:
    def __init__(self):
        pygame.init()
        pygame.display.set_caption("Ultrasonic Radar System - Telemetry Monitor")
        self.screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
        self.clock = pygame.time.Clock()
        self.font_title = pygame.font.SysFont("Courier New", 22, bold=True)
        self.font_data = pygame.font.SysFont("Courier New", 16, bold=True)
        self.font_small = pygame.font.SysFont("Courier New", 12)
        # Radar Sweep Center (bottom middle of the radar area)
        self.center_x = 550
        self.center_y = 570
        # Serial Connection Setup
        self.ser = None
        self.is_simulated = False
        self.port_name = "SIMULATED"
        self.init_serial()
        # Telemetry State
        self.current_angle = 90
        self.current_distance = 0
        self.scan_history = {}    # Maps angle (int) -> (distance_cm, timestamp)
        self.sweep_angle = 0.0    # Floating sweep representation for visual smoothness
        
        # Simulation Mock Obstacles
        self.sim_obstacles = [
            {"angle": 45, "dist": 80, "noise": 0.0},
            {"angle": 120, "dist": 140, "noise": 0.0},
            {"angle": 85, "dist": 15, "noise": 0.0}
        ]
        self.sim_direction = 1
        self.sim_last_tick = time.time()
    def init_serial(self):
        # Scan for ports
        ports = list(list_ports.comports())
        target_port = None
        
        # Try to find a Nucleo or USB-Serial port
        for p in ports:
            if "usbmodem" in p.device.lower() or "usbserial" in p.device.lower() or "com" in p.device.lower():
                target_port = p.device
                break
                
        if target_port:
            try:
                self.ser = serial.Serial(target_port, 9600, timeout=0.1)
                self.port_name = target_port
                print(f"Connected to hardware on port: {self.port_name}")
            except Exception as e:
                print(f"Failed to open port {target_port}: {e}. Falling back to simulation.")
                self.is_simulated = True
        else:
            print("No active microcontroller board found. Launching in SIMULATION mode.")
            self.is_simulated = True
    def read_telemetry(self):
        if self.is_simulated:
            # Generate simulated radar telemetry in "angle,distance" format
            now = time.time()
            if now - self.sim_last_tick >= 0.04: # 40ms updates (same as STM32)
                self.sim_last_tick = now
                self.current_angle += self.sim_direction * 2
                
                if self.current_angle >= 180:
                    self.current_angle = 180
                    self.sim_direction = -1
                elif self.current_angle <= 0:
                    self.current_angle = 0
                    self.sim_direction = 1
                
                # Check if we are pointing towards one of the mock obstacles
                detected_dist = 0
                for obs in self.sim_obstacles:
                    if abs(self.current_angle - obs["angle"]) < 8:
                        # Introduce small distance noise
                        noise = random.uniform(-1.5, 1.5)
                        detected_dist = obs["dist"] + noise
                        break
                
                # Add general random clutter/empty reads occasionally
                if detected_dist == 0 and random.random() < 0.02:
                    detected_dist = random.uniform(50, 190)
                self.current_distance = detected_dist
                self.scan_history[self.current_angle] = (self.current_distance, now)
        else:
            # Read from hardware serial
            if self.ser and self.ser.in_waiting > 0:
                try:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if ',' in line:
                        parts = line.split(',')
                        angle = int(parts[0])
                        distance = int(parts[1])
                        
                        self.current_angle = angle
                        self.current_distance = distance
                        self.scan_history[self.current_angle] = (self.current_distance, time.time())
                except Exception as e:
                    pass
        # Smoothly slide the sweep line towards the current telemetry angle
        diff = self.current_angle - self.sweep_angle
        self.sweep_angle += diff * 0.4
    def draw_grid(self):
        # Draw concentric rings
        ring_counts = 4
        for i in range(1, ring_counts + 1):
            radius = int((RADAR_RADIUS / ring_counts) * i)
            dist_label = int((MAX_DISTANCE_CM / ring_counts) * i)
            
            # Draw semi-circular grid arc
            pygame.draw.arc(self.screen, COLOR_GRID, 
                            (self.center_x - radius, self.center_y - radius, radius * 2, radius * 2), 
                            0, math.pi, 2)
            
            # Distance labels
            lbl = self.font_small.render(f"{dist_label}cm", True, COLOR_TEXT_DIM)
            self.screen.blit(lbl, (self.center_x + radius - 35, self.center_y + 5))
        # Draw angular grid lines (30, 60, 90, 120, 150 degrees)
        angles = [30, 60, 90, 120, 150]
        for a in angles:
            rad = math.radians(a)
            end_x = self.center_x + RADAR_RADIUS * math.cos(rad)
            end_y = self.center_y - RADAR_RADIUS * math.sin(rad)
            pygame.draw.line(self.screen, COLOR_GRID, (self.center_x, self.center_y), (end_x, end_y), 1)
            
            # Draw angle markers
            lbl = self.font_small.render(f"{a}°", True, COLOR_TEXT_DIM)
            lbl_x = self.center_x + (RADAR_RADIUS + 15) * math.cos(rad) - 10
            lbl_y = self.center_y - (RADAR_RADIUS + 15) * math.sin(rad) - 8
            self.screen.blit(lbl, (lbl_x, lbl_y))
        # Base line
        pygame.draw.line(self.screen, COLOR_GRID, 
                         (self.center_x - RADAR_RADIUS - 20, self.center_y), 
                         (self.center_x + RADAR_RADIUS + 20, self.center_y), 3)
    def draw_sweep_and_points(self):
        now = time.time()
        
        # 1. Draw historical scan results with fading trails
        for angle, (dist, timestamp) in list(self.scan_history.items()):
            age = now - timestamp
            if age > 4.0: # Remove objects after 4 seconds of decay
                del self.scan_history[angle]
                continue
                
            if dist == 0 or dist > MAX_DISTANCE_CM:
                continue # Out of range or no obstacle
                
            # Compute alpha / brightness fade
            alpha_ratio = max(0.0, 1.0 - (age / 4.0))
            
            # Map distance to screen pixels
            pixel_dist = (dist / MAX_DISTANCE_CM) * RADAR_RADIUS
            rad = math.radians(angle)
            point_x = int(self.center_x + pixel_dist * math.cos(rad))
            point_y = int(self.center_y - pixel_dist * math.sin(rad))
            
            # Color shifts to red if within threat zone, otherwise fades green
            if dist < DANGER_THRESHOLD_CM:
                dot_color = (int(255 * alpha_ratio), int(30 * alpha_ratio), int(50 * alpha_ratio))
                glow_radius = 8
            else:
                dot_color = (int(COLOR_RADAR_GREEN[0] * alpha_ratio), 
                             int(COLOR_RADAR_GREEN[1] * alpha_ratio), 
                             int(COLOR_RADAR_GREEN[2] * alpha_ratio))
                glow_radius = 5
            # Draw target glow
            s = pygame.Surface((glow_radius * 2, glow_radius * 2), pygame.SRCALPHA)
            pygame.draw.circle(s, (*dot_color, int(100 * alpha_ratio)), (glow_radius, glow_radius), glow_radius)
            self.screen.blit(s, (point_x - glow_radius, point_y - glow_radius))
            
            # Draw core target dot
            pygame.draw.circle(self.screen, dot_color, (point_x, point_y), 3)
        # 2. Draw sweeping radar arm
        sweep_rad = math.radians(self.sweep_angle)
        sweep_end_x = self.center_x + RADAR_RADIUS * math.cos(sweep_rad)
        sweep_end_y = self.center_y - RADAR_RADIUS * math.sin(sweep_rad)
        
        # Draw sweeping radar line
        pygame.draw.line(self.screen, COLOR_SWEEP, (self.center_x, self.center_y), (sweep_end_x, sweep_end_y), 2)
        
        # Draw a translucent fade wedge behind the sweeping line
        fade_steps = 25
        for i in range(fade_steps):
            fade_angle = self.sweep_angle - (self.sim_direction * i * 0.8)
            frad = math.radians(fade_angle)
            fx = self.center_x + RADAR_RADIUS * math.cos(frad)
            fy = self.center_y - RADAR_RADIUS * math.sin(frad)
            
            alpha = int(120 * (1.0 - (i / fade_steps)))
            fade_color = (*COLOR_GRID, alpha)
            
            # Draw thin fading radial rays
            s = pygame.Surface((WINDOW_WIDTH, WINDOW_HEIGHT), pygame.SRCALPHA)
            pygame.draw.line(s, fade_color, (self.center_x, self.center_y), (fx, fy), 2)
            self.screen.blit(s, (0, 0))
    def draw_dashboard(self):
        # Draw Sidebar Area
        pygame.draw.rect(self.screen, COLOR_CARD, (15, 20, 260, WINDOW_HEIGHT - 40), border_radius=8)
        pygame.draw.rect(self.screen, COLOR_GRID, (15, 20, 260, WINDOW_HEIGHT - 40), width=2, border_radius=8)
        # Render Header Info
        header_lbl = self.font_title.render("RADAR MONITOR", True, COLOR_SWEEP)
        self.screen.blit(header_lbl, (30, 35))
        
        # Divider Line
        pygame.draw.line(self.screen, COLOR_GRID, (30, 75), (260, 75), 1)
        # Port status card
        lbl_status_title = self.font_small.render("CONNECTION PORT:", True, COLOR_TEXT_DIM)
        self.screen.blit(lbl_status_title, (30, 95))
        
        status_color = COLOR_SWEEP if not self.is_simulated else COLOR_TEXT_DIM
        lbl_status_val = self.font_data.render(self.port_name, True, status_color)
        self.screen.blit(lbl_status_val, (30, 115))
        lbl_mode_title = self.font_small.render("SYSTEM MODE:", True, COLOR_TEXT_DIM)
        self.screen.blit(lbl_mode_title, (30, 145))
        
        mode_str = "HARDWARE TELET" if not self.is_simulated else "SIMULATED SCAN"
        lbl_mode_val = self.font_data.render(mode_str, True, COLOR_TEXT)
        self.screen.blit(lbl_mode_val, (30, 165))
        # Divider Line
        pygame.draw.line(self.screen, COLOR_GRID, (30, 205), (260, 205), 1)
        # Sensor Telemetry Readings
        lbl_ang_title = self.font_small.render("SWEEP ANGLE:", True, COLOR_TEXT_DIM)
        self.screen.blit(lbl_ang_title, (30, 225))
        lbl_ang_val = self.font_title.render(f"{self.current_angle}°", True, COLOR_SWEEP)
        self.screen.blit(lbl_ang_val, (30, 245))
        lbl_dist_title = self.font_small.render("OBJECT DISTANCE:", True, COLOR_TEXT_DIM)
        self.screen.blit(lbl_dist_title, (30, 295))
        
        dist_str = f"{self.current_distance:.1f} cm" if self.current_distance > 0 else "OUT OF RANGE"
        dist_color = COLOR_THREAT if (0 < self.current_distance < DANGER_THRESHOLD_CM) else COLOR_TEXT
        lbl_dist_val = self.font_title.render(dist_str, True, dist_color)
        self.screen.blit(lbl_dist_val, (30, 315))
        # Proximity Alarm Indicator
        alarm_active = 0 < self.current_distance < DANGER_THRESHOLD_CM
        if alarm_active:
            # Flashing threat banner
            flash_time = pygame.time.get_ticks()
            if (flash_time // 250) % 2 == 0:
                pygame.draw.rect(self.screen, COLOR_THREAT, (30, 375, 230, 45), border_radius=5)
                lbl_alarm = self.font_data.render("THREAT DETECTED!", True, (255, 255, 255))
                self.screen.blit(lbl_alarm, (65, 388))
            else:
                pygame.draw.rect(self.screen, COLOR_GRID, (30, 375, 230, 45), width=2, border_radius=5)
                lbl_alarm = self.font_data.render("WARN: CLOSE RNG", True, COLOR_THREAT)
                self.screen.blit(lbl_alarm, (70, 388))
        else:
            pygame.draw.rect(self.screen, COLOR_GRID, (30, 375, 230, 45), width=1, border_radius=5)
            lbl_alarm = self.font_data.render("ZONE SECURE", True, COLOR_TEXT_DIM)
            self.screen.blit(lbl_alarm, (85, 388))
        # Instructions / Interactive options
        lbl_instr_title = self.font_small.render("CONTROLS:", True, COLOR_TEXT_DIM)
        self.screen.blit(lbl_instr_title, (30, 450))
        
        lbl_instr_1 = self.font_small.render("ESC: Quit Monitor", True, COLOR_TEXT)
        self.screen.blit(lbl_instr_1, (30, 475))
        
        lbl_instr_2 = self.font_small.render("R: Reset Scan Data", True, COLOR_TEXT)
        self.screen.blit(lbl_instr_2, (30, 495))
        
        if self.is_simulated:
            lbl_instr_3 = self.font_small.render("Click Radar: Add Object", True, COLOR_TEXT)
            self.screen.blit(lbl_instr_3, (30, 515))
    def reset_data(self):
        self.scan_history.clear()
        if self.is_simulated:
            self.sim_obstacles.clear()
    def run(self):
        running = True
        while running:
            # 1. Event Handling
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False
                elif event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_ESCAPE:
                        running = False
                    elif event.key == pygame.K_r:
                        self.reset_data()
                elif event.type == pygame.MOUSEBUTTONDOWN and self.is_simulated:
                    # Allow user to click on the screen to add simulated obstacles
                    mx, my = event.pos
                    # Calculate distance and angle relative to center
                    dx = mx - self.center_x
                    dy = self.center_y - my
                    clicked_radius = math.sqrt(dx*dx + dy*dy)
                    
                    if clicked_radius <= RADAR_RADIUS and dy >= 0:
                        clicked_angle = int(math.degrees(math.atan2(dy, dx)))
                        if clicked_angle < 0:
                            clicked_angle += 360
                        clicked_dist_cm = (clicked_radius / RADAR_RADIUS) * MAX_DISTANCE_CM
                        
                        self.sim_obstacles.append({
                            "angle": clicked_angle,
                            "dist": clicked_dist_cm,
                            "noise": 0.0
                        })
                        print(f"Added simulation obstacle at: Angle={clicked_angle}°, Distance={clicked_dist_cm:.1f}cm")
            # 2. Telemetry polling
            self.read_telemetry()
            # 3. Drawing
            self.screen.fill(COLOR_BG)
            self.draw_grid()
            self.draw_sweep_and_points()
            self.draw_dashboard()
            pygame.display.flip()
            self.clock.tick(FPS)
        # Cleanup serial on exit
        if self.ser and self.ser.is_open:
            self.ser.close()
        pygame.quit()
        sys.exit()
if __name__ == "__main__":
    app = RadarVisualizer()
    app.run()

