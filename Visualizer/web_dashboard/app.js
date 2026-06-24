// UI Elements
const btnConnect = document.getElementById('btn-connect');
const btnSimulate = document.getElementById('btn-simulate');
const statusPort = document.getElementById('status-port');
const systemStatusDot = document.getElementById('system-status-dot');
const telemetryAngle = document.getElementById('telemetry-angle');
const telemetryDistance = document.getElementById('telemetry-distance');
const telemetryStatus = document.getElementById('telemetry-status');
const dialHand = document.getElementById('dial-hand');
const detectionLog = document.getElementById('detection-log');
const threatAlertBanner = document.getElementById('threat-alert-banner');
const canvas = document.getElementById('radarCanvas');
const ctx = canvas.getContext('2d');
// State
let port = null;
let reader = null;
let isSimulating = false;
let isConnected = false;
let inputBuffer = '';
let scanHistory = {}; // Maps angle -> { distance, timestamp }
let currentAngle = 90;
let currentDistance = 0;
let sweepAngle = 90;
let simDirection = 1;
let simIntervalId = null;
// Simulated Obstacles
let simObstacles = [
    { angle: 30, distance: 75 },
    { angle: 90, distance: 15 }, // Threat
    { angle: 145, distance: 120 }
];
// Audio Context for Sonar Sweeps
let audioCtx = null;
let lastPingTime = 0;
// Setup Canvas for High-DPI screens
function setupCanvas() {
    const dpr = window.devicePixelRatio || 1;
    const rect = canvas.parentNode.getBoundingClientRect();
    
    // We want the canvas width to match the wrapper width
    canvas.width = rect.width * dpr;
    canvas.height = rect.height * dpr;
    canvas.style.width = `${rect.width}px`;
    canvas.style.height = `${rect.height}px`;
    
    ctx.scale(dpr, dpr);
}
// Initialize Audio Context on click
function initAudio() {
    if (!audioCtx) {
        audioCtx = new (window.AudioContext || window.webkitAudioContext)();
    }
}
// Play a retro-futuristic sonar sweep sound
function playSonarSound(pitchMod = 1.0, isWarning = false) {
    if (!audioCtx) return;
    if (audioCtx.state === 'suspended') {
        audioCtx.resume();
    }
    const now = audioCtx.currentTime;
    
    // Limit sonar sound triggers to prevent auditory fatigue
    if (now - lastPingTime < 0.18) return;
    lastPingTime = now;
    const osc = audioCtx.createOscillator();
    const gainNode = audioCtx.createGain();
    
    osc.connect(gainNode);
    gainNode.connect(audioCtx.destination);
    
    if (isWarning) {
        // High pitched double alert beep
        osc.type = 'sawtooth';
        osc.frequency.setValueAtTime(880, now);
        osc.frequency.exponentialRampToValueAtTime(440, now + 0.15);
        gainNode.gain.setValueAtTime(0.12, now);
        gainNode.gain.exponentialRampToValueAtTime(0.01, now + 0.15);
        osc.start(now);
        osc.stop(now + 0.15);
    } else {
        // Classic sonar ping
        osc.type = 'sine';
        const startFreq = 800 + (pitchMod * 400); // modulate frequency by sweep angle
        osc.frequency.setValueAtTime(startFreq, now);
        osc.frequency.exponentialRampToValueAtTime(startFreq / 2, now + 0.35);
        
        gainNode.gain.setValueAtTime(0.08, now);
        gainNode.gain.exponentialRampToValueAtTime(0.001, now + 0.4);
        
        osc.start(now);
        osc.stop(now + 0.4);
    }
}
// Drawing Constants relative to responsive sizes
let cx, cy, maxRadius;
const maxDistanceCm = 200;
const threatPerimeterCm = 20;
function updateRadarDrawingParams() {
    const w = canvas.width / (window.devicePixelRatio || 1);
    const h = canvas.height / (window.devicePixelRatio || 1);
    cx = w / 2;
    cy = h - 35;
    maxRadius = Math.min(w / 2 - 40, h - 70);
}
// Main Render Loop
function drawRadar() {
    updateRadarDrawingParams();
    
    // Clear with a transparent fill to create a trailing motion blur effect
    ctx.fillStyle = 'rgba(7, 11, 8, 0.14)';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    drawGrid();
    drawTelemetryPoints();
    drawSweepLine();
    // Smoothly animate sweep angle towards current target angle
    const diff = currentAngle - sweepAngle;
    sweepAngle += diff * 0.45;
    requestAnimationFrame(drawRadar);
}
function drawGrid() {
    // Concentric Range Rings
    const ringCount = 4;
    ctx.lineWidth = 1;
    
    for (let i = 1; i <= ringCount; i++) {
        const radius = (maxRadius / ringCount) * i;
        const distCm = (maxDistanceCm / ringCount) * i;
        
        ctx.strokeStyle = 'rgba(0, 255, 85, 0.08)';
        ctx.beginPath();
        ctx.arc(cx, cy, radius, Math.PI, 2 * Math.PI);
        ctx.stroke();
        // Distance Labels
        ctx.font = '9px Orbitron';
        ctx.fillStyle = '#57735f';
        ctx.textAlign = 'left';
        ctx.fillText(`${distCm}cm`, cx + radius - 35, cy + 12);
    }
    // Radial Angled Grid Lines (30, 60, 90, 120, 150 degrees)
    const angles = [30, 60, 90, 120, 150];
    angles.forEach(deg => {
        const rad = (180 - deg) * Math.PI / 180; // invert because 0 is right in trig, but 0 is left in radar
        const ex = cx + maxRadius * Math.cos(rad);
        const ey = cy - maxRadius * Math.sin(rad);
        ctx.strokeStyle = 'rgba(0, 255, 85, 0.08)';
        ctx.beginPath();
        ctx.moveTo(cx, cy);
        ctx.lineTo(ex, ey);
        ctx.stroke();
        // Angle Labels
        ctx.font = '10px Orbitron';
        ctx.fillStyle = '#93b39c';
        ctx.textAlign = 'center';
        
        const lx = cx + (maxRadius + 18) * Math.cos(rad);
        const ly = cy - (maxRadius + 18) * Math.sin(rad);
        ctx.fillText(`${deg}°`, lx, ly);
    });
    // Base horizontal line
    ctx.strokeStyle = 'rgba(0, 255, 85, 0.15)';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(cx - maxRadius - 20, cy);
    ctx.lineTo(cx + maxRadius + 20, cy);
    ctx.stroke();
}
function drawTelemetryPoints() {
    const now = Date.now();
    
    Object.keys(scanHistory).forEach(angleStr => {
        const angle = parseInt(angleStr);
        const { distance, timestamp } = scanHistory[angleStr];
        const ageSec = (now - timestamp) / 1000;
        
        if (ageSec > 4.0) { // Keep data for 4 seconds
            delete scanHistory[angleStr];
            return;
        }
        if (distance === 0 || distance > maxDistanceCm) return;
        const opacity = Math.max(0, 1 - ageSec / 4.0);
        const pixelDist = (distance / maxDistanceCm) * maxRadius;
        const rad = (180 - angle) * Math.PI / 180;
        
        const px = cx + pixelDist * Math.cos(rad);
        const py = cy - pixelDist * Math.sin(rad);
        // Select Threat Red or Standard Green
        const isThreat = distance < threatPerimeterCm;
        const color = isThreat ? `rgba(255, 26, 60, ${opacity})` : `rgba(0, 255, 85, ${opacity})`;
        
        // Draw obstacle glow ring
        ctx.fillStyle = isThreat ? `rgba(255, 26, 60, ${opacity * 0.18})` : `rgba(0, 255, 85, ${opacity * 0.15})`;
        ctx.beginPath();
        ctx.arc(px, py, isThreat ? 9 : 6, 0, 2 * Math.PI);
        ctx.fill();
        // Draw obstacle center dot
        ctx.fillStyle = color;
        ctx.beginPath();
        ctx.arc(px, py, 3, 0, 2 * Math.PI);
        ctx.fill();
    });
}
function drawSweepLine() {
    const rad = (180 - sweepAngle) * Math.PI / 180;
    const sx = cx + maxRadius * Math.cos(rad);
    const sy = cy - maxRadius * Math.sin(rad);
    // Glowing sweeping arm
    ctx.strokeStyle = '#00ff55';
    ctx.lineWidth = 2;
    ctx.shadowBlur = 8;
    ctx.shadowColor = 'rgba(0, 255, 85, 0.6)';
    ctx.beginPath();
    ctx.moveTo(cx, cy);
    ctx.lineTo(sx, sy);
    ctx.stroke();
    
    // Reset shadow
    ctx.shadowBlur = 0;
    // Draw fading gradient tail wedge (sweep trailing arc)
    const tailCount = 20;
    ctx.lineWidth = 1.5;
    for (let i = 0; i < tailCount; i++) {
        const offsetAngle = sweepAngle - (simDirection * i * 0.7);
        const orad = (180 - offsetAngle) * Math.PI / 180;
        const ox = cx + maxRadius * Math.cos(orad);
        const oy = cy - maxRadius * Math.sin(orad);
        
        const opacity = Math.max(0, 0.4 * (1 - i / tailCount));
        ctx.strokeStyle = `rgba(0, 255, 85, ${opacity})`;
        ctx.beginPath();
        ctx.moveTo(cx, cy);
        ctx.lineTo(ox, oy);
        ctx.stroke();
    }
}
// Process Telemetry Input
function processTelemetryData(angle, distance) {
    currentAngle = angle;
    currentDistance = distance;
    
    // Update Sidebar Fields
    telemetryAngle.innerText = `${angle}°`;
    dialHand.style.transform = `rotate(${angle}deg)`;
    if (distance > 0 && distance <= maxDistanceCm) {
        telemetryDistance.innerText = `${distance} cm`;
        
        // Log telemetry reading
        scanHistory[angle] = { distance, timestamp: Date.now() };
        // Proximity threat logic
        if (distance < threatPerimeterCm) {
            telemetryStatus.innerText = 'THREAT';
            telemetryStatus.className = 'threat-badge threat';
            threatAlertBanner.classList.remove('hidden');
            
            // Add threat log
            addThreatLogEntry(angle, distance);
            
            // Play warning alarm beep
            playSonarSound(null, true);
        } else {
            telemetryStatus.innerText = 'SECURE';
            telemetryStatus.className = 'threat-badge clear';
            threatAlertBanner.classList.add('hidden');
            
            // Play standard sweep ping modulated by angle
            playSonarSound(angle / 180.0, false);
        }
    } else {
        telemetryDistance.innerText = 'OUT OF RANGE';
        telemetryStatus.innerText = 'SECURE';
        telemetryStatus.className = 'threat-badge clear';
        threatAlertBanner.classList.add('hidden');
        
        // Log clear spot
        scanHistory[angle] = { distance: 0, timestamp: Date.now() };
        
        // Play sweep ping
        playSonarSound(angle / 180.0, false);
    }
}
// Log threat entry to sidebar list
let lastLogTime = 0;
function addThreatLogEntry(angle, distance) {
    const now = Date.now();
    // Throttle logging to prevent overflowing the sidebar
    if (now - lastLogTime < 500) return;
    lastLogTime = now;
    // Remove placeholder
    const placeholder = detectionLog.querySelector('.log-placeholder');
    if (placeholder) placeholder.remove();
    const timestamp = new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
    const entry = document.createElement('div');
    entry.className = 'log-entry';
    entry.innerHTML = `
        <span class="time">[${timestamp}]</span>
        <span class="desc">OBSTACLE @ ${angle}° (${Math.round(distance)}cm)</span>
    `;
    detectionLog.insertBefore(entry, detectionLog.firstChild);
    // Keep log limited to last 15 items
    if (detectionLog.children.length > 15) {
        detectionLog.lastChild.remove();
    }
}
// Interactive Simulation Engine
function startSimulation() {
    isSimulating = true;
    btnSimulate.innerText = '⏹️ Stop Simulation';
    btnSimulate.classList.add('active');
    systemStatusDot.className = 'status-indicator-dot simulating';
    statusPort.innerText = 'SIMULATING';
    statusPort.className = 'val status-sim';
    let simAngle = 0;
    
    simIntervalId = setInterval(() => {
        simAngle += simDirection * 2;
        if (simAngle >= 180) {
            simAngle = 180;
            simDirection = -1;
        } else if (simAngle <= 0) {
            simAngle = 0;
            simDirection = 1;
        }
        // Check if cursor/sweep points to a simulated obstacle
        let simDist = 0;
        simObstacles.forEach(obs => {
            if (Math.abs(simAngle - obs.angle) < 6) {
                // Add minor noise
                simDist = Math.round(obs.distance + (Math.random() * 2 - 1));
            }
        });
        processTelemetryData(simAngle, simDist);
    }, 40);
}
function stopSimulation() {
    isSimulating = false;
    btnSimulate.innerText = '⚙️ Run Simulation';
    btnSimulate.classList.remove('active');
    clearInterval(simIntervalId);
    
    if (!isConnected) {
        systemStatusDot.className = 'status-indicator-dot';
        statusPort.innerText = 'DISCONNECTED';
        statusPort.className = 'val status-closed';
    }
}
// Connect to Serial Port (Web Serial API)
async function connectSerial() {
    initAudio();
    if (isSimulating) stopSimulation();
    try {
        port = await navigator.serial.requestPort();
        await port.open({ baudRate: 9600 });
        
        isConnected = true;
        btnConnect.innerText = '🔌 Disconnect';
        statusPort.innerText = 'CONNECTED';
        statusPort.className = 'val status-open';
        systemStatusDot.className = 'status-indicator-dot online';
        readSerialStream();
    } catch (e) {
        console.error('Serial port connection failed:', e);
        alert('Failed to connect to Serial Port: ' + e.message);
    }
}
async function disconnectSerial() {
    isConnected = false;
    btnConnect.innerText = '🔌 Connect Radar';
    statusPort.innerText = 'DISCONNECTED';
    statusPort.className = 'val status-closed';
    systemStatusDot.className = 'status-indicator-dot';
    if (reader) {
        await reader.cancel();
        reader.releaseLock();
        reader = null;
    }
    if (port) {
        await port.close();
        port = null;
    }
}
// Read telemetry stream from Serial
async function readSerialStream() {
    const textDecoder = new TextDecoderStream();
    const readableStreamClosed = port.readable.pipeTo(textDecoder.writable);
    reader = textDecoder.readable.getReader();
    try {
        while (isConnected) {
            const { value, done } = await reader.read();
            if (done) break;
            
            // Append incoming text segment to buffer
            inputBuffer += value;
            
            // Process individual rows terminated by \n or \r
            let newlineIdx;
            while ((newlineIdx = inputBuffer.indexOf('\n')) >= 0) {
                const line = inputBuffer.substring(0, newlineIdx).trim();
                inputBuffer = inputBuffer.substring(newlineIdx + 1);
                if (line.includes(',')) {
                    const parts = line.split(',');
                    if (parts.length === 2) {
                        const angle = parseInt(parts[0]);
                        const distance = parseFloat(parts[1]);
                        
                        if (!isNaN(angle) && !isNaN(distance)) {
                            // Update simulation sweep direction based on telemetry trend
                            if (angle > currentAngle) simDirection = 1;
                            else if (angle < currentAngle) simDirection = -1;
                            processTelemetryData(angle, distance);
                        }
                    }
                }
            }
        }
    } catch (e) {
        console.error('Error reading serial stream:', e);
    } finally {
        reader.releaseLock();
    }
}
// Add click listener on canvas to generate interactive obstacles during simulation
canvas.addEventListener('mousedown', (e) => {
    initAudio();
    if (!isSimulating) return;
    const rect = canvas.getBoundingClientRect();
    const mx = e.clientX - rect.left;
    const my = e.clientY - rect.top;
    
    // Relative coordinates to center
    const dx = mx - cx;
    const dy = cy - my; // invert Y
    
    const clickRadius = Math.sqrt(dx*dx + dy*dy);
    
    if (clickRadius <= maxRadius && dy >= 0) {
        // Compute angle (180 - calculated angle in degrees)
        let clickAngle = Math.round(Math.atan2(dy, dx) * 180 / Math.PI);
        clickAngle = 180 - clickAngle; // map to our grid coordinate frame (0 left, 180 right)
        
        const clickDistCm = Math.round((clickRadius / maxRadius) * maxDistanceCm);
        
        // Add to simulation obstacles array
        simObstacles.push({ angle: clickAngle, distance: clickDistCm });
        
        // Temporarily log threat or detection
        addThreatLogEntry(clickAngle, clickDistCm);
    }
});
// Event Listeners
btnConnect.addEventListener('click', () => {
    if (isConnected) {
        disconnectSerial();
    } else {
        connectSerial();
    }
});
btnSimulate.addEventListener('click', () => {
    initAudio();
    if (isSimulating) {
        stopSimulation();
    } else {
        startSimulation();
    }
});
// Resize handler
window.addEventListener('resize', () => {
    setupCanvas();
});
// Boot Setup
window.addEventListener('DOMContentLoaded', () => {
    setupCanvas();
    drawRadar();
});

