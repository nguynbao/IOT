#!/usr/bin/env python3
"""
Utility script để giúp cấu hình ESP32 kết nối tới Audio Server
Hiển thị IP address và hướng dẫn cấu hình
"""

import socket
import subprocess
import platform
import sys

def get_local_ip():
    """Lấy local IP address"""
    try:
        # Cách đơn giản nhất
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception as e:
        print(f"❌ Lỗi lấy IP: {e}")
        return None

def get_all_ips():
    """Lấy tất cả IP addresses"""
    try:
        result = subprocess.run(['ifconfig'], capture_output=True, text=True)
        lines = result.stdout.split('\n')
        ips = []
        
        for line in lines:
            if 'inet ' in line and 'addr:' not in line:
                parts = line.split()
                if len(parts) > 1:
                    ip = parts[1]
                    if ip.startswith('inet'):
                        ip = parts[1] if 'inet ' not in parts[1] else parts[2]
                    if ip and not ip.startswith('127.0'):
                        ips.append(ip)
        
        return ips
    except:
        return []

def print_config():
    """In hướng dẫn cấu hình"""
    print("\n" + "="*60)
    print("🎤 AUDIO SERVER - ESP32 Configuration Helper")
    print("="*60 + "\n")
    
    ip = get_local_ip()
    
    if ip:
        print(f"✅ Local IP Address: {ip}")
        print(f"📱 Server URL: http://{ip}:5000")
        print(f"📡 API Endpoint: http://{ip}:5000/api/upload-audio\n")
        
        print("=" * 60)
        print("📝 COPY THIS CODE TO YOUR ESP32:")
        print("=" * 60)
        
        esp32_code = f'''// 🎤 Audio Server Configuration
#define SERVER_IP "{ip}"
#define SERVER_PORT 5000

// Example: Send audio to server
void sendAudioToServer(int16_t* samples, int sampleCount, int sampleRate) {{
    if (WiFi.status() != WL_CONNECTED) {{
        Serial.println("WiFi not connected");
        return;
    }}

    String url = String("http://") + SERVER_IP + ":" + SERVER_PORT + "/api/upload-audio";
    
    // Build JSON
    String json = "{{\\"samples\\":[";
    for (int i = 0; i < sampleCount; i++) {{
        json += String(samples[i]);
        if (i < sampleCount - 1) json += ",";
    }}
    json += "],\\"sample_rate\\":" + String(sampleRate) + "}}";
    
    // Send to server
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST(json);
    
    if (httpCode == 200) {{
        Serial.println("✅ Audio sent to server!");
    }} else {{
        Serial.printf("❌ Server error: %d\\n", httpCode);
    }}
    
    http.end();
}}
'''
        
        print(esp32_code)
        
        print("\n" + "="*60)
        print("🌐 TRUY CẬP WEB INTERFACE:")
        print("="*60)
        print(f"👉 http://{ip}:5000\n")
        
    else:
        print("❌ Could not determine IP address")
        print("Please configure manually or check network connection\n")
    
    print("="*60)
    print("💡 TIPS:")
    print("="*60)
    print("1. Đảm bảo ESP32 và máy tính trên cùng WiFi network")
    print("2. Firewall phải cho phép port 5000")
    print("3. Khởi động server trước: python audio_server.py")
    print("4. Chạy script này mỗi lần cần xem IP\n")

if __name__ == '__main__':
    print_config()
