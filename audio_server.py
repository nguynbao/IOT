#!/usr/bin/env python3
"""
Local Audio Server - Nhận và phát lại bản thu mic
- Nhận dữ liệu audio từ ESP32
- Lưu trữ dữ liệu audio
- Phát lại audio thông qua web interface
"""

from flask import Flask, render_template, request, send_file, jsonify
from flask_cors import CORS
import os
import io
import wave
import struct
from datetime import datetime
import threading

app = Flask(__name__)
CORS(app)

# Directory để lưu trữ file audio
AUDIO_FOLDER = os.path.join(os.path.dirname(__file__), 'recordings')
os.makedirs(AUDIO_FOLDER, exist_ok=True)

# Directory để lưu trữ file ảnh
IMAGE_FOLDER = os.path.join(os.path.dirname(__file__), 'images')
os.makedirs(IMAGE_FOLDER, exist_ok=True)

# Biến global để lưu audio hiện tại
current_audio_data = {
    'samples': None,
    'sample_rate': 16000,
    'filename': None,
    'timestamp': None
}

def create_wav_file(samples, sample_rate):
    """Tạo WAV file từ audio samples"""
    # Mỗi sample là int16 (2 bytes)
    wav_buffer = io.BytesIO()
    
    # WAV Header
    num_channels = 1
    bits_per_sample = 16
    byte_rate = sample_rate * num_channels * bits_per_sample // 8
    block_align = num_channels * bits_per_sample // 8
    
    with wave.open(wav_buffer, 'wb') as wav_file:
        wav_file.setnchannels(num_channels)
        wav_file.setsampwidth(bits_per_sample // 8)
        wav_file.setframerate(sample_rate)
        
        # Chuyển samples thành bytes
        audio_bytes = b''.join(
            struct.pack('<h', int(sample)) for sample in samples
        )
        wav_file.writeframes(audio_bytes)
    
    wav_buffer.seek(0)
    return wav_buffer


@app.route('/api/upload-audio', methods=['POST'])
def upload_audio():
    """API endpoint nhận dữ liệu audio từ ESP32"""
    try:
        # Try to parse JSON safely (don't raise on bad Content-Type)
        data = None
        if request.is_json:
            data = request.get_json(silent=True)

        # If not JSON, but body contains bytes, try to decode as JSON
        if data is None:
            raw = request.get_data()
            if raw:
                try:
                    import json
                    data = json.loads(raw.decode('utf-8'))
                except Exception:
                    data = None

        # Case 1: JSON payload with samples
        if data and isinstance(data, dict) and 'samples' in data:
            try:
                samples = data['samples']
                sample_rate = int(data.get('sample_rate', 16000))

                # Save into current_audio_data
                current_audio_data['samples'] = samples
                current_audio_data['sample_rate'] = sample_rate
                current_audio_data['timestamp'] = datetime.now()

                filename = f"recording_{datetime.now().strftime('%Y%m%d_%H%M%S')}.wav"
                current_audio_data['filename'] = filename

                wav_buffer = create_wav_file(samples, sample_rate)
                filepath = os.path.join(AUDIO_FOLDER, filename)
                with open(filepath, 'wb') as f:
                    f.write(wav_buffer.getvalue())

                duration = len(samples) / sample_rate
                print(f"✅ Nhận audio (JSON): {filename} ({duration:.2f}s, {sample_rate}Hz)")
                return jsonify({
                    'success': True,
                    'filename': filename,
                    'duration': duration,
                    'samples': len(samples)
                }), 200
            except Exception as e:
                print(f"❌ Lỗi xử lý JSON samples: {str(e)}")
                return jsonify({'error': f'Error processing samples: {str(e)}'}), 400

        # Case 2: multipart/form-data (ESP32 may send custom boundary without standard form keys)
        content_type = request.headers.get('Content-Type', '')
        raw = request.get_data()
        if content_type.startswith('multipart/form-data') and raw:
            # Try to parse multipart manually using email parser
            try:
                import re
                from email.parser import BytesParser
                from email.policy import default
                import json

                # Build a pseudo-message so the parser can understand the multipart body
                pseudo = b'Content-Type: ' + content_type.encode('utf-8') + b'\r\nMIME-Version: 1.0\r\n\r\n' + raw
                msg = BytesParser(policy=default).parsebytes(pseudo)

                if msg.is_multipart():
                    for part in msg.iter_parts():
                        payload = part.get_payload(decode=True)
                        if not payload:
                            continue

                        # Try JSON first
                        try:
                            part_json = json.loads(payload.decode('utf-8')) # type: ignore
                            if isinstance(part_json, dict) and 'samples' in part_json:
                                samples = part_json['samples']
                                sample_rate = int(part_json.get('sample_rate', 16000))

                                current_audio_data['samples'] = samples
                                current_audio_data['sample_rate'] = sample_rate
                                current_audio_data['timestamp'] = datetime.now()
                                filename = f"recording_{datetime.now().strftime('%Y%m%d_%H%M%S')}.wav"
                                current_audio_data['filename'] = filename
                                wav_buffer = create_wav_file(samples, sample_rate)
                                filepath = os.path.join(AUDIO_FOLDER, filename)
                                with open(filepath, 'wb') as f:
                                    f.write(wav_buffer.getvalue())
                                duration = len(samples) / sample_rate
                                print(f"✅ Nhận audio (multipart JSON): {filename} ({duration:.2f}s, {sample_rate}Hz)")
                                return jsonify({'success': True, 'filename': filename, 'duration': duration, 'samples': len(samples)}), 200
                        except (json.JSONDecodeError, UnicodeDecodeError):
                            pass

                        # If not JSON, treat payload as raw audio bytes
                        try:
                            filename = f"multipart_{datetime.now().strftime('%Y%m%d_%H%M%S')}.wav"
                            filepath = os.path.join(AUDIO_FOLDER, filename)
                            with open(filepath, 'wb') as f:
                                f.write(payload) # type: ignore
                            print(f"✅ Nhận audio (multipart binary): {filename} ({len(payload)} bytes)")
                            return jsonify({'success': True, 'filename': filename, 'size': len(payload)}), 200
                        except Exception as write_err:
                            print(f"❌ Lỗi ghi file multipart: {write_err}")
                            continue
            except Exception as e:
                print(f"❌ Multipart parse error: {e}")

        # Case 3: multipart/form-data with standard file field
        # Accept any file field (prefer 'audio' if provided) to match ESP32 client
        if request.files:
            try:
                # Prefer the field named 'audio' (BackendClient uses name="audio"),
                # otherwise pick the first file field available.
                field_name = 'audio' if 'audio' in request.files else next(iter(request.files))
                up = request.files[field_name]
                buf = up.read()
                if not buf:
                    return jsonify({'error': 'Empty file uploaded'}), 400

                # Save using original filename when available
                safe_name = up.filename or field_name
                filename = f"upload_{datetime.now().strftime('%Y%m%d_%H%M%S')}_{safe_name}"
                # If the filename doesn't already end with .wav and content-type suggests wav, add extension
                content_type = up.mimetype or ''
                if not filename.lower().endswith('.wav') and ('audio' in content_type or content_type == 'application/octet-stream'):
                    filename = filename + '.wav'

                filepath = os.path.join(AUDIO_FOLDER, filename)
                with open(filepath, 'wb') as f:
                    f.write(buf)
                print(f"✅ Nhận audio (file upload: {field_name}): {filename} ({len(buf)} bytes)")
                return jsonify({'success': True, 'filename': filename, 'size': len(buf)}), 200
            except Exception as e:
                print(f"❌ Lỗi upload file: {str(e)}")
                return jsonify({'error': f'Error uploading file: {str(e)}'}), 500

        # Case 4: raw audio bytes (e.g., Content-Type: application/octet-stream or audio/wav)
        if raw and (content_type.startswith('audio') or 'octet-stream' in content_type):
            try:
                # Save raw bytes directly
                filename = f"raw_{datetime.now().strftime('%Y%m%d_%H%M%S')}.wav"
                filepath = os.path.join(AUDIO_FOLDER, filename)
                with open(filepath, 'wb') as f:
                    f.write(raw)
                print(f"✅ Nhận audio (raw bytes): {filename} ({len(raw)} bytes, {content_type})")
                return jsonify({'success': True, 'filename': filename, 'size': len(raw)}), 200
            except Exception as e:
                print(f"❌ Lỗi lưu raw audio: {str(e)}")
                return jsonify({'error': f'Error saving raw audio: {str(e)}'}), 500

        # If nothing matched, return helpful error
        ct = request.headers.get('Content-Type')
        msg = 'No audio data provided or unsupported Content-Type.'
        if ct:
            msg += f" Received Content-Type: {ct}"
        else:
            msg += ' No Content-Type header.'
        return jsonify({'error': msg}), 415
        
    except Exception as e:
        print(f"❌ Lỗi upload audio: {str(e)}")
        return jsonify({'error': str(e)}), 500


@app.route('/api/recordings', methods=['GET'])
def get_recordings():
    """Lấy danh sách tất cả recordings"""
    try:
        files = []
        
        # Đảm bảo thư mục tồn tại
        if not os.path.exists(AUDIO_FOLDER):
            os.makedirs(AUDIO_FOLDER, exist_ok=True)
            return jsonify({'recordings': []}), 200
        
        for filename in sorted(os.listdir(AUDIO_FOLDER), reverse=True):
            if filename.endswith('.wav'):
                filepath = os.path.join(AUDIO_FOLDER, filename)
                try:
                    file_size = os.path.getsize(filepath)
                    
                    # Đọc duration từ WAV file
                    duration = 0
                    try:
                        with wave.open(filepath, 'rb') as wav_file:
                            frames = wav_file.getnframes()
                            rate = wav_file.getframerate()
                            if rate > 0:
                                duration = frames / rate
                    except Exception as wav_err:
                        print(f"⚠️ Không thể đọc duration của {filename}: {wav_err}")
                        duration = 0
                    
                    files.append({
                        'filename': filename,
                        'size': file_size,
                        'duration': duration,
                        'url': f'/api/play/{filename}'
                    })
                except Exception as file_err:
                    print(f"⚠️ Lỗi xử lý file {filename}: {file_err}")
                    continue
        
        print(f"✅ Lấy {len(files)} recordings thành công")
        return jsonify({'recordings': files}), 200
    except Exception as e:
        print(f"❌ Lỗi lấy recordings: {str(e)}")
        return jsonify({'error': str(e), 'recordings': []}), 200


@app.route('/api/play/<filename>', methods=['GET'])
def play_audio(filename):
    """Phát audio file"""
    try:
        if not filename.endswith('.wav'):
            return jsonify({'error': 'Invalid file'}), 400
        
        filepath = os.path.join(AUDIO_FOLDER, filename)
        
        if not os.path.exists(filepath):
            return jsonify({'error': 'File not found'}), 404
        
        return send_file(filepath, mimetype='audio/wav')
    except Exception as e:
        return jsonify({'error': str(e)}), 500


@app.route('/api/current', methods=['GET'])
def get_current_audio():
    """Lấy audio hiện tại"""
    if current_audio_data['samples'] is None:
        return jsonify({'error': 'No audio data available'}), 404
    
    wav_buffer = create_wav_file(
        current_audio_data['samples'],
        current_audio_data['sample_rate']
    )
    
    return send_file(
        wav_buffer,
        mimetype='audio/wav',
        as_attachment=True,
        download_name=current_audio_data['filename'] or 'current_audio.wav'
    )


@app.route('/api/delete/<filename>', methods=['DELETE'])
def delete_recording(filename):
    """Xóa recording"""
    try:
        if not filename.endswith('.wav'):
            return jsonify({'error': 'Invalid file'}), 400
        
        filepath = os.path.join(AUDIO_FOLDER, filename)
        
        if not os.path.exists(filepath):
            return jsonify({'error': 'File not found'}), 404
        
        os.remove(filepath)
        print(f"🗑 Đã xóa: {filename}")
        
        return jsonify({'success': True}), 200
    except Exception as e:
        return jsonify({'error': str(e)}), 500


# ==================== IMAGE ENDPOINTS ====================

@app.route('/api/upload-image', methods=['POST'])
def upload_image():
    """API endpoint nhận ảnh từ ESP32 (camera)"""
    try:
        content_type = request.headers.get('Content-Type', '')
        raw = request.get_data()
        
        print(f"📥 Upload image request - Content-Type: {content_type}, Size: {len(raw)} bytes")
        
        # Case 1: Standard Flask file upload - multipart/form-data
        if request.files and 'image' in request.files:
            print("📦 Processing as Flask multipart file...")
            try:
                image_file = request.files['image']
                if not image_file or image_file.filename == '':
                    return jsonify({'error': 'No image selected'}), 400
                
                buf = image_file.read()
                if not buf:
                    return jsonify({'error': 'Empty image uploaded'}), 400
                
                original_filename = image_file.filename or 'capture.jpg'
                file_ext = os.path.splitext(original_filename)[1].lower()
                if file_ext not in ['.jpg', '.jpeg', '.png', '.gif', '.bmp', '.webp']:
                    file_ext = '.jpg'
                
                filename = f"image_{datetime.now().strftime('%Y%m%d_%H%M%S')}{file_ext}"
                filepath = os.path.join(IMAGE_FOLDER, filename)
                with open(filepath, 'wb') as f:
                    f.write(buf)
                
                print(f"✅ Nhận ảnh (Flask): {filename} ({len(buf)} bytes)")
                return jsonify({
                    'success': True,
                    'filename': filename,
                    'size': len(buf),
                    'url': f'/api/image/{filename}'
                }), 200
            except Exception as e:
                print(f"❌ Flask multipart error: {str(e)}")
                return jsonify({'error': f'Flask parse error: {str(e)}'}), 400
        
        # Case 2: Raw multipart/form-data from ESP32 with manual parsing
        if content_type.startswith('multipart/form-data') and raw:
            print("📦 Processing as raw multipart data...")
            try:
                # Extract boundary from Content-Type header
                boundary_match = content_type.split('boundary=')
                if len(boundary_match) < 2:
                    print("❌ No boundary found in Content-Type")
                    return jsonify({'error': 'Invalid multipart boundary'}), 400
                
                boundary = boundary_match[1].strip('"')
                boundary_bytes = ('--' + boundary).encode()
                end_boundary_bytes = ('--' + boundary + '--').encode()
                
                print(f"   Boundary: {boundary}")
                
                # Split by boundary
                parts = raw.split(boundary_bytes)
                print(f"   Found {len(parts)} parts")
                
                image_data = None
                
                # Process each part
                for i, part in enumerate(parts):
                    if not part or part == b'--' or part.startswith(b'--'):
                        continue
                    
                    print(f"   Processing part {i}...")
                    
                    # Split headers from body
                    if b'\r\n\r\n' in part:
                        headers_section, body = part.split(b'\r\n\r\n', 1)
                    else:
                        continue
                    
                    headers_str = headers_section.decode('utf-8', errors='ignore').lower()
                    
                    # Look for Content-Disposition with "image" field
                    if 'content-disposition' in headers_str and 'image' in headers_str:
                        print(f"   Found image field!")
                        
                        # Remove trailing boundary markers
                        body = body.rstrip(b'\r\n').rstrip(b'-')
                        
                        if len(body) > 0:
                            image_data = body
                            print(f"   Image data size: {len(image_data)} bytes")
                            break
                
                if not image_data:
                    print("❌ No image data found in multipart payload")
                    return jsonify({'error': 'No image field found in multipart data'}), 400
                
                # Verify it's JPEG by checking magic bytes
                is_jpeg = image_data.startswith(b'\xff\xd8')
                ext = '.jpg' if is_jpeg else '.bin'
                
                filename = f"image_{datetime.now().strftime('%Y%m%d_%H%M%S')}{ext}"
                filepath = os.path.join(IMAGE_FOLDER, filename)
                
                with open(filepath, 'wb') as f:
                    f.write(image_data)
                
                print(f"✅ Nhận ảnh (raw multipart): {filename} ({len(image_data)} bytes, JPEG: {is_jpeg})")
                return jsonify({
                    'success': True,
                    'filename': filename,
                    'size': len(image_data),
                    'url': f'/api/image/{filename}'
                }), 200
                
            except Exception as e:
                print(f"❌ Raw multipart parse error: {str(e)}")
                import traceback
                traceback.print_exc()
                return jsonify({'error': f'Multipart parse error: {str(e)}'}), 400
        
        # Case 3: Raw binary image data
        if raw and ('image' in content_type or content_type == 'application/octet-stream'):
            print("📦 Processing as raw binary...")
            try:
                # Detect image format from magic bytes
                ext = '.jpg'
                if raw.startswith(b'\x89PNG'):
                    ext = '.png'
                elif raw.startswith(b'GIF8'):
                    ext = '.gif'
                elif raw.startswith(b'RIFF') and b'WEBP' in raw[:20]:
                    ext = '.webp'
                elif raw.startswith(b'\xff\xd8'):  # JPEG magic bytes
                    ext = '.jpg'
                
                filename = f"image_{datetime.now().strftime('%Y%m%d_%H%M%S')}{ext}"
                filepath = os.path.join(IMAGE_FOLDER, filename)
                
                with open(filepath, 'wb') as f:
                    f.write(raw)
                
                print(f"✅ Nhận ảnh raw: {filename} ({len(raw)} bytes)")
                return jsonify({
                    'success': True,
                    'filename': filename,
                    'size': len(raw),
                    'url': f'/api/image/{filename}'
                }), 200
            except Exception as e:
                print(f"❌ Lỗi lưu ảnh: {str(e)}")
                return jsonify({'error': f'Error saving image: {str(e)}'}), 500
        
        return jsonify({'error': 'No image data provided'}), 415
    
    except Exception as e:
        print(f"❌ Lỗi upload ảnh: {str(e)}")
        return jsonify({'error': str(e)}), 500


@app.route('/api/images', methods=['GET'])
def get_images():
    """Lấy danh sách tất cả ảnh"""
    try:
        images = []
        
        # Đảm bảo thư mục tồn tại
        if not os.path.exists(IMAGE_FOLDER):
            os.makedirs(IMAGE_FOLDER, exist_ok=True)
            return jsonify({'images': []}), 200
        
        for filename in sorted(os.listdir(IMAGE_FOLDER), reverse=True):
            # Accept common image formats
            if filename.lower().endswith(('.jpg', '.jpeg', '.png', '.gif', '.bmp', '.webp')):
                filepath = os.path.join(IMAGE_FOLDER, filename)
                try:
                    file_size = os.path.getsize(filepath)
                    images.append({
                        'filename': filename,
                        'size': file_size,
                        'url': f'/api/image/{filename}',
                        'thumbnail_url': f'/api/image/{filename}?thumbnail=1'
                    })
                except Exception as file_err:
                    print(f"⚠️ Lỗi xử lý ảnh {filename}: {file_err}")
                    continue
        
        print(f"✅ Lấy {len(images)} ảnh thành công")
        return jsonify({'images': images}), 200
    except Exception as e:
        print(f"❌ Lỗi lấy danh sách ảnh: {str(e)}")
        return jsonify({'error': str(e), 'images': []}), 200


@app.route('/api/image/<filename>', methods=['GET'])
def get_image(filename):
    """Lấy file ảnh"""
    try:
        # Validate filename
        if '..' in filename or filename.startswith('/'):
            return jsonify({'error': 'Invalid filename'}), 400
        
        # Check if file has valid image extension
        valid_exts = ('.jpg', '.jpeg', '.png', '.gif', '.bmp', '.webp')
        if not filename.lower().endswith(valid_exts):
            return jsonify({'error': 'Invalid file type'}), 400
        
        filepath = os.path.join(IMAGE_FOLDER, filename)
        
        if not os.path.exists(filepath):
            return jsonify({'error': 'Image not found'}), 404
        
        # Determine MIME type
        mime_type = 'image/jpeg'
        if filename.lower().endswith('.png'):
            mime_type = 'image/png'
        elif filename.lower().endswith('.gif'):
            mime_type = 'image/gif'
        elif filename.lower().endswith('.webp'):
            mime_type = 'image/webp'
        elif filename.lower().endswith('.bmp'):
            mime_type = 'image/bmp'
        
        return send_file(filepath, mimetype=mime_type)
    except Exception as e:
        return jsonify({'error': str(e)}), 500


@app.route('/api/delete-image/<filename>', methods=['DELETE'])
def delete_image(filename):
    """Xóa ảnh"""
    try:
        # Validate filename
        if '..' in filename or filename.startswith('/'):
            return jsonify({'error': 'Invalid filename'}), 400
        
        # Check if file has valid image extension
        valid_exts = ('.jpg', '.jpeg', '.png', '.gif', '.bmp', '.webp')
        if not filename.lower().endswith(valid_exts):
            return jsonify({'error': 'Invalid file type'}), 400
        
        filepath = os.path.join(IMAGE_FOLDER, filename)
        
        if not os.path.exists(filepath):
            return jsonify({'error': 'Image not found'}), 404
        
        os.remove(filepath)
        print(f"🗑 Đã xóa ảnh: {filename}")
        
        return jsonify({'success': True}), 200
    except Exception as e:
        return jsonify({'error': str(e)}), 500


@app.route('/')
def index():
    """Trang web chính"""
    return render_template('index.html')


@app.route('/health', methods=['GET'])
def health():
    """Health check"""
    return jsonify({
        'status': 'ok',
        'recordings': len([f for f in os.listdir(AUDIO_FOLDER) if f.endswith('.wav')])
    }), 200


if __name__ == '__main__':
    print("🎤 Audio Server đang chạy...")
    print("📱 Truy cập: http://localhost:8080")
    print(f"📁 Lưu recordings tại: {AUDIO_FOLDER}")
    app.run(debug=True, host='0.0.0.0', port=8080)
