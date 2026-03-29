# 🎯 ESP32 Image Upload - Diagnosis & Solution

## ✅ Server Status: Working
- Flask server running on http://192.168.1.177:8080
- `/api/upload-image` endpoint responding ✅
- Image folder created and saving files ✅
- Sample image successfully saved

## ❌ Problem: ESP32 Cannot Connect

The server is working correctly, but **ESP32 cannot reach it**.

---

## Root Causes to Check

### 1. **Network Connectivity**
The most likely issue. ESP32 and server must be on **same WiFi network**.

**Check WiFi Config in iot_config.h:**
```cpp
#define WIFI_SSID "MILANO95"
#define WIFI_PASS "68686868"
```

**Verify:**
- [ ] ESP32 is connected to same WiFi (MILANO95)
- [ ] Server machine is on same network
- [ ] Both show network connectivity

**Test from Serial Monitor:**
Should show during boot:
```
✅ WiFi connected
📡 WiFi connected, IP: 192.168.x.x
```

### 2. **IP Address Mismatch**
Server is at `192.168.1.177:8080`, but ESP32 might see different IP.

**Find Actual Server IP:**
```bash
ifconfig | grep "inet "  # On server machine
```

**Common IP formats:**
- 192.168.1.x (most common)
- 192.168.0.x (some routers)
- 10.0.x.x (less common)

**If Different:**
Update `iot_config.h`:
```cpp
#define SERVER_HOST "http://YOUR_ACTUAL_IP:8080"
```

Then recompile and upload.

### 3. **Firewall Blocking Port 8080**
macOS firewall might be blocking external connections.

**Check Firewall Status:**
```bash
# macOS
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --getglobalstate
```

**If Blocked:**
```bash
# Disable firewall (risky)
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --setglobalstate off

# Or allow Flask specifically
sudo ufw allow 8080
```

### 4. **Router Network Isolation**
Some routers isolate WiFi devices from each other.

**Check Router Settings:**
- Login to WiFi router admin panel
- Look for: "Client Isolation", "AP Isolation", "Device Isolation"
- DISABLE if found

### 5. **Network Routing Issue**
ESP32 might see server on different subnet.

**Debug from ESP32:**
Add this test code to `main.cpp`:
```cpp
void testNetworkDebug() {
    Serial.printf("🔍 Local IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("🔍 Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("🔍 Subnet: %s\n", WiFi.subnetMask().toString().c_str());
}

// Call in setup():
testNetworkDebug();
```

This shows if ESP32 is on same subnet as server.

---

## Quickest Solution

### **Option A: Ensure Same Network (RECOMMENDED)**

1. **Verify WiFi:**
   - Check ESP32 connects to "MILANO95"
   - Check server machine on same "MILANO95"

2. **Find Server IP:**
   ```bash
   ifconfig | grep "inet " | grep -v 127.0.0.1
   ```
   
   Note the IP (e.g., `192.168.1.177`)

3. **Check Server Running:**
   ```bash
   curl http://192.168.1.177:8080/api/images
   # Should return: {"images": [...]}
   ```

4. **If curl fails:**
   - Server not running: `python3 audio_server.py`
   - Wrong IP: Find correct IP above
   - Firewall: Disable or allow port 8080

5. **Verify from ESP32:**
   - Check serial output
   - Should see: `📡 WiFi connected, IP: 192.168.1.x`
   - Should see attempt to capture/send image

### **Option B: Use Hotspot (Alternative)**

If sharing network is hard:
1. Create Personal Hotspot on server machine
2. Connect ESP32 to that hotspot  
3. Use `192.168.1.1` or `192.168.0.1` as server IP

### **Option C: USB Serial Debugging**

Connect ESP32 via USB and monitor serial output:
```bash
pio device monitor
```

Look for these messages:
- `✅ WiFi connected` - WiFi working
- `📡 WiFi connected, IP: ...` - Shows ESP32 IP
- `⏳ Sending image...` - Attempting upload
- `❌ Upload failed: connection refused` - Connection problem

The IP shown here should be in same subnet as server's IP.

---

## Step-by-Step Fix

### 1. Terminal on Server Machine

```bash
# Find server IP
SERVER_IP=$(ifconfig | grep "inet " | grep -v 127.0.0.1 | awk '{print $2}' | head -1)
echo "Server IP: $SERVER_IP"

# Start Flask server (if not running)
cd /Users/macos/Documents/IOT
python3 audio_server.py

# In another terminal, verify it's responding
curl http://$SERVER_IP:8080/api/images
```

### 2. Update ESP32 Config

Edit `/Users/macos/Documents/IOT/include/iot_config.h`:

```cpp
// If server IP is 192.168.1.177:
#define SERVER_HOST "http://192.168.1.177:8080"

// Or whatever the correct IP is:
#define SERVER_HOST "http://192.168.1.YOUR_IP:8080"
```

### 3. Compile & Upload

```bash
# Build
pio run -e esp32s3-n16r8-cam

# Upload to ESP32
pio run -e esp32s3-n16r8-cam -t upload

# Monitor
pio device monitor
```

### 4. Monitor Serial Output

Watch for:
```
✅ WiFi connected
📷 Capturing image...
✅ Image captured: 65536 bytes
⏳ Sending image...
🌐 POST http://192.168.1.177:8080/api/upload-image
📊 Image size: 65536 bytes
📦 Total payload: 65700 bytes
✅ Upload OK, code: 200
```

### 5. Verify Upload

```bash
# Check if image appears
ls -lah ./images/ | tail -5

# Should show new file:
# image_20260129_182248.jpg   15B
```

---

## Network Diagnosis Checklist

Run this checklist to isolate the problem:

```
☐ Server running on correct IP?
  curl http://192.168.1.177:8080/api/images
  
☐ ESP32 WiFi connected?
  Serial monitor shows: ✅ WiFi connected
  
☐ ESP32 IP on same subnet?
  Check serial: 192.168.1.x matches server
  
☐ Firewall not blocking?
  Try: sudo /usr/libexec/ApplicationFirewall/socketfilterfw --setglobalstate off
  
☐ Flask binding to 0.0.0.0?
  Check last line of audio_server.py:
  app.run(debug=True, host='0.0.0.0', port=8080)
  
☐ Router allows device-to-device communication?
  Check router settings for "Isolation"
  
☐ No conflicting processes on 8080?
  lsof -i :8080
```

---

## If Still Not Working

### Advanced Debug: Add Test Code

In `main.cpp`, add:
```cpp
void testHTTPConnection() {
    Serial.println("\n🧪 Testing HTTP connection...");
    
    HTTPClient http;
    String url = "http://192.168.1.177:8080/api/images";
    
    http.begin(url);
    http.setTimeout(5000);
    
    Serial.println("⏳ Connecting to " + url);
    int code = http.GET();
    
    Serial.printf("📊 Response code: %d\n", code);
    if (code > 0) {
        Serial.println("✅ Connection successful!");
        Serial.println("📝 Response:");
        Serial.println(http.getString());
    } else {
        Serial.printf("❌ Connection failed: %s\n", http.errorToString(code).c_str());
    }
    
    http.end();
}

// In setup(), call:
testHTTPConnection();
```

Then:
1. Add `#include <HTTPClient.h>` at top
2. Compile and upload
3. Check serial output for result

---

## Success Indicators

✅ **When working:**
- Serial shows: `✅ Upload OK, code: 200`
- New file appears in `./images/`
- Web UI shows image: http://192.168.1.177:8080
- Timestamp is recent (last 30 seconds)

---

## Common Error Messages & Fixes

| Error | Cause | Fix |
|-------|-------|-----|
| `connection refused` | Can't reach server | Check IP, firewall, WiFi |
| `❌ WiFi not connected` | No WiFi | Check SSID/password |
| `❌ Not enough memory` | PSRAM not available | Check PSRAM config |
| `HTTP -1` | Connection timeout | Server too slow or unreachable |
| `HTTP 405` | Wrong HTTP method | Server method mismatch (should be POST) |
| `HTTP 500` | Server error | Check Flask logs |

---

## Next Action

**Most Likely Fix (90% of issues):**

1. Find your server's actual IP:
   ```bash
   ifconfig | grep "inet " | grep -v 127
   ```

2. Update `iot_config.h` with correct IP

3. Recompile & upload

4. Check serial monitor

That should fix it! 🚀
