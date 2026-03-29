# ⚡ Quick Fix - "Connection Refused" Error

## The Problem
ESP32 shows: `❌ Upload failed: connection refused`

Server works fine - you tested it manually.

## The Solution (Choose One)

### FIX #1: Check Server IP (Most Common - 80% of cases)

**On your Mac:**
```bash
ifconfig | grep "inet " | grep -v 127.0.0.1 | head -1
```

You'll see something like: `192.168.1.150` or `192.168.1.177`

**Edit:** `include/iot_config.h`

Change this line:
```cpp
#define SERVER_HOST "http://192.168.1.177:8080"
```

To match YOUR IP:
```cpp
#define SERVER_HOST "http://192.168.1.150:8080"  // Your actual IP!
```

**Then:**
```bash
cd /Users/macos/Documents/IOT
pio run -e esp32s3-n16r8-cam -t upload
```

---

### FIX #2: Disable Firewall (macOS)

If you just changed the IP but still getting "connection refused":

```bash
# Disable macOS firewall
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --setglobalstate off

# Or just for Flask port:
sudo ufw allow 8080/tcp
```

Then restart Flask:
```bash
# Kill Flask
lsof -i :8080  # See process ID
kill -9 <PID>

# Or just kill all python:
killall python3

# Restart
python3 audio_server.py
```

---

### FIX #3: Check WiFi Network

Ensure ESP32 and server on **same WiFi**:

**In iot_config.h:**
```cpp
#define WIFI_SSID "MILANO95"    // Make sure this is YOUR network
#define WIFI_PASS "68686868"    // Make sure password correct
```

---

### FIX #4: Check If Flask is Running

```bash
# Is Flask running?
lsof -i :8080

# Should show python3 process
# If nothing shows, start it:
python3 audio_server.py
```

---

## Verify Fix Worked

After making changes:

1. **Recompile ESP32:**
   ```bash
   cd /Users/macos/Documents/IOT
   pio run -e esp32s3-n16r8-cam -t upload
   ```

2. **Monitor serial output:**
   ```bash
   pio device monitor
   ```

3. **Wait ~30 seconds** for image capture

4. **Look for SUCCESS:**
   ```
   ✅ Image captured: 65536 bytes
   ⏳ Sending image...
   ✅ Upload OK, code: 200
   ```

5. **Verify file saved:**
   ```bash
   ls -lah /Users/macos/Documents/IOT/images/ | tail -3
   ```

Should show new file with current timestamp!

---

## If STILL Not Working

Follow the full guide: [ESP32_UPLOAD_DIAGNOSIS.md](ESP32_UPLOAD_DIAGNOSIS.md)

That file has all the advanced diagnostics.
