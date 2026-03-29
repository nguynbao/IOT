#!/usr/bin/env python3
"""
Test script cho audio upload endpoint
"""
import requests
import json
import struct

def test_json_upload():
    """Test upload audio dùng JSON format"""
    print("\n🧪 Test 1: JSON format upload")
    samples = [100, 200, 300, 400, 500]
    payload = {
        'samples': samples,
        'sample_rate': 16000
    }
    
    try:
        response = requests.post(
            'http://localhost:8000/api/upload-audio',
            json=payload,
            timeout=5
        )
        print(f"Status: {response.status_code}")
        print(f"Response: {response.json()}")
        return response.status_code == 200
    except Exception as e:
        print(f"❌ Error: {e}")
        return False


def test_raw_bytes_upload():
    """Test upload audio dùng raw bytes"""
    print("\n🧪 Test 2: Raw bytes (audio/wav) upload")
    samples = [100, 200, 300, 400, 500]
    
    # Create WAV header
    import wave
    import io
    
    wav_buffer = io.BytesIO()
    with wave.open(wav_buffer, 'wb') as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(16000)
        audio_bytes = b''.join(struct.pack('<h', int(s)) for s in samples)
        wav_file.writeframes(audio_bytes)
    
    wav_data = wav_buffer.getvalue()
    
    try:
        response = requests.post(
            'http://localhost:8000/api/upload-audio',
            data=wav_data,
            headers={'Content-Type': 'audio/wav'},
            timeout=5
        )
        print(f"Status: {response.status_code}")
        print(f"Response: {response.json()}")
        return response.status_code == 200
    except Exception as e:
        print(f"❌ Error: {e}")
        return False


def test_multipart_upload():
    """Test upload audio dùng multipart/form-data"""
    print("\n🧪 Test 3: Multipart file upload")
    
    samples = [100, 200, 300, 400, 500]
    import wave
    import io
    
    wav_buffer = io.BytesIO()
    with wave.open(wav_buffer, 'wb') as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(16000)
        audio_bytes = b''.join(struct.pack('<h', int(s)) for s in samples)
        wav_file.writeframes(audio_bytes)
    
    wav_buffer.seek(0)
    
    try:
        response = requests.post(
            'http://localhost:8000/api/upload-audio',
            files={'file': ('test_audio.wav', wav_buffer, 'audio/wav')},
            timeout=5
        )
        print(f"Status: {response.status_code}")
        print(f"Response: {response.json()}")
        return response.status_code == 200
    except Exception as e:
        print(f"❌ Error: {e}")
        return False


def test_get_recordings():
    """Test lấy danh sách recordings"""
    print("\n🧪 Test 4: Get recordings list")
    try:
        response = requests.get('http://localhost:8000/api/recordings', timeout=5)
        print(f"Status: {response.status_code}")
        data = response.json()
        print(f"Total recordings: {len(data.get('recordings', []))}")
        return response.status_code == 200
    except Exception as e:
        print(f"❌ Error: {e}")
        return False


if __name__ == '__main__':
    print("=" * 50)
    print("🎤 Audio Upload Endpoint Test")
    print("=" * 50)
    
    results = []
    results.append(("JSON Upload", test_json_upload()))
    results.append(("Raw Bytes Upload", test_raw_bytes_upload()))
    results.append(("Multipart Upload", test_multipart_upload()))
    results.append(("Get Recordings", test_get_recordings()))
    
    print("\n" + "=" * 50)
    print("📊 Test Results:")
    print("=" * 50)
    for test_name, passed in results:
        status = "✅ PASS" if passed else "❌ FAIL"
        print(f"{test_name}: {status}")
    
    total = len(results)
    passed = sum(1 for _, p in results if p)
    print(f"\nTotal: {passed}/{total} tests passed")
