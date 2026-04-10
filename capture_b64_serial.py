#!/usr/bin/env python3
import serial
import serial.tools.list_ports
import os
import base64
import time

def find_esp32_port():
    import serial.tools.list_ports
    ports = serial.tools.list_ports.comports()
    for port in ports:
        # Loại bỏ các cổng ảo Bluetooth của Mac vì nếu nhận nhầm sẽ treo im re
        if "Bluetooth" in port.description or "Bluetooth" in port.device:
            continue
        if "usb" in port.device.lower() or "uart" in port.description.lower() or "ch340" in port.description.lower() or "cp210" in port.description.lower() or "ACM" in port.device:
            return port.device
            
    # Nếu không tìm thấy cái nào khớp chữ "usb", chọn cái cổng đầu tiên KHÔNG phải Bluetooth
    for port in ports:
        if "Bluetooth" not in port.description and "Bluetooth" not in port.device:
            return port.device
    return None

def main():
    print("🎧 Đang lắng nghe luồng âm thanh trả về từ server...")
    
    capturing = False
    b64_content = []

    while True:
        try:
            port = find_esp32_port()
            if not port:
                time.sleep(1)
                continue

            with serial.Serial(port, 115200, timeout=1) as ser:
                print(f"🔌 (Re)Connected to {port}")
                while True:
                    if ser.in_waiting > 0:
                        line = ser.readline().decode('utf-8', errors='ignore').strip()
                        
                        # In toàn bộ log của ESP32 ra màn hình để báo cáo (Ngoại trừ khối base64 quá dài)
                        if not capturing:
                            print(f"[ESP32] {line}")
                        
                        if "---B64_RESPONSE_START---" in line:
                            print("\n📥 Bắt đầu TỰ ĐỘNG nhận dữ liệu Base64...")
                            capturing = True
                            b64_content = []
                            continue
                        
                        if "---B64_RESPONSE_END---" in line:
                            print("✅ Kết thúc nhận dữ liệu.")
                            capturing = False
                            
                            full_b64 = "".join(b64_content)
                            
                            # Tự động ghi vào paste_b64_here.txt
                            b64_path = os.path.join(os.path.dirname(__file__), 'paste_b64_here.txt')
                            with open(b64_path, 'w') as f:
                                f.write(full_b64)
                            print(f"💾 TỰ ĐỘNG LƯU: Đã ghi chuỗi gốc vào {b64_path}")
                            
                            # Tự động tạo file WAV
                            try:
                                raw_bytes = base64.b64decode(full_b64)
                                wav_path = os.path.join(os.path.dirname(__file__), 'output_audio.wav')
                                import wave
                                with wave.open(wav_path, 'wb') as wav_file:
                                    wav_file.setnchannels(1)
                                    wav_file.setsampwidth(2)
                                    wav_file.setframerate(16000)
                                    wav_file.writeframes(raw_bytes)
                                print(f"🎵 TỰ ĐỘNG GIẢI MÃ: Đã tạo file WAV: {wav_path}")
                                
                                import subprocess
                                print("▶️ Đang mở loa máy Mac phát tự động...")
                                subprocess.run(["afplay", wav_path])
                                print("✅ Phát xong!")
                                
                            except Exception as e:
                                print(f"⚠️ Lỗi giải mã: {e}")
                        
                        if capturing and not "---B64_RESPONSE_" in line:
                            b64_content.append(line)
                    else:
                        time.sleep(0.01)
        except serial.SerialException as e:
            # Device disconnected or multiple access -> Wait and reconnect
            print(f"\n⚠️ Mất kết nối Serial (có thể ESP32 khởi động lại hoặc trùng cổng). Đang thử kết nối lại...")
            time.sleep(2)
        except KeyboardInterrupt:
            print("\n👋 Đã thoát lắng nghe.")
            break
        except Exception as e:
            print(f"Lỗi không xác định: {e}")
            time.sleep(2)

if __name__ == '__main__':
    main()
