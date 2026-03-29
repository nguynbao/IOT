#!/usr/bin/env python3
"""
Test script for image upload functionality
Requires: requests library (pip install requests)
"""

import requests
import json
import sys
from pathlib import Path

def test_image_api():
    """Test image upload and retrieval"""
    
    SERVER_URL = "http://192.168.1.177:8080"
    API_URL = f"{SERVER_URL}/api"
    
    print("=" * 60)
    print("🖼  IMAGE API TEST SCRIPT")
    print("=" * 60)
    
    # Test 1: Check server health
    print("\n1️⃣  Testing server health...")
    try:
        response = requests.get(f"{SERVER_URL}/health", timeout=5)
        if response.status_code == 200:
            data = response.json()
            print(f"✅ Server is running")
            print(f"   Recordings count: {data.get('recordings', 0)}")
        else:
            print(f"❌ Server returned: {response.status_code}")
            return False
    except Exception as e:
        print(f"❌ Cannot reach server: {e}")
        return False
    
    # Test 2: Get images list (empty)
    print("\n2️⃣  Testing /api/images endpoint...")
    try:
        response = requests.get(f"{API_URL}/images", timeout=5)
        if response.status_code == 200:
            data = response.json()
            print(f"✅ Endpoint working")
            print(f"   Current images: {len(data.get('images', []))}")
        else:
            print(f"❌ Failed: {response.status_code}")
            return False
    except Exception as e:
        print(f"❌ Error: {e}")
        return False
    
    # Test 3: Upload test image
    print("\n3️⃣  Testing image upload...")
    
    # Create a simple test image (1x1 red pixel JPEG)
    test_image_data = (
        b'\xff\xd8\xff\xe0\x00\x10JFIF\x00\x01\x01\x00\x00\x01\x00\x01\x00\x00'
        b'\xff\xdb\x00C\x00\x08\x06\x06\x07\x06\x05\x08\x07\x07\x07\t\t\x08\n\x0c'
        b'\x14\r\x0c\x0b\x0b\x0c\x19\x12\x13\x0f\x14\x1d\x1a\x1f\x1e\x1d\x1a\x1c'
        b'\x1c $.\' ",#\x1c\x1c(7),01444\x1f\'9=82<.342\xff\xc0\x00\x0b\x01\x00'
        b'\x01\x01\x01\x11\x00\xff\xc4\x00\x1f\x00\x00\x01\x05\x01\x01\x01\x01'
        b'\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x01\x02\x03\x04\x05\x06\x07'
        b'\x08\t\n\x0b\xff\xc4\x00\xb5\x10\x00\x02\x01\x03\x03\x02\x04\x03\x05'
        b'\x05\x04\x04\x00\x00\x01}\x01\x02\x03\x00\x04\x11\x05\x12!1A\x06\x13'
        b'Qa\x07"q\x142\x81\x91\xa1\x08#B\xb1\xc1\x15R\xd1\xf0$3br\x82\t\n\x16'
        b'\x17\x18\x19\x1a%&\'()*456789:CDEFGHIJSTUVWXYZcdefghijstuvwxyz\x83'
        b'\x84\x85\x86\x87\x88\x89\x8a\x92\x93\x94\x95\x96\x97\x98\x99\x9a\xa2'
        b'\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba'
        b'\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9'
        b'\xda\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xf1\xf2\xf3\xf4\xf5\xf6'
        b'\xf7\xf8\xf9\xfa\xff\xda\x00\x08\x01\x01\x00\x00?\x00\xfb\xd4\xff\xd9'
    )
    
    try:
        files = {'image': ('test_image.jpg', test_image_data, 'image/jpeg')}
        response = requests.post(
            f"{API_URL}/upload-image",
            files=files,
            timeout=10
        )
        
        if response.status_code == 200:
            data = response.json()
            if data.get('success'):
                uploaded_filename = data.get('filename')
                print(f"✅ Image uploaded successfully")
                print(f"   Filename: {uploaded_filename}")
                print(f"   Size: {data.get('size')} bytes")
            else:
                print(f"❌ Upload returned error: {data.get('error')}")
                return False
        else:
            print(f"❌ Failed: HTTP {response.status_code}")
            print(f"   Response: {response.text}")
            return False
    except Exception as e:
        print(f"❌ Error uploading: {e}")
        return False
    
    # Test 4: Verify image in list
    print("\n4️⃣  Verifying uploaded image in list...")
    try:
        response = requests.get(f"{API_URL}/images", timeout=5)
        data = response.json()
        images = data.get('images', [])
        
        if len(images) > 0:
            print(f"✅ Image found in list")
            for img in images:
                print(f"   - {img['filename']} ({img['size']} bytes)")
        else:
            print(f"❌ No images in list")
            return False
    except Exception as e:
        print(f"❌ Error: {e}")
        return False
    
    # Test 5: Download image
    print("\n5️⃣  Testing image download...")
    try:
        # Get first image
        response = requests.get(f"{API_URL}/images", timeout=5)
        data = response.json()
        if len(data.get('images', [])) > 0:
            filename = data['images'][0]['filename']
            response = requests.get(
                f"{API_URL}/image/{filename}",
                timeout=5
            )
            
            if response.status_code == 200:
                print(f"✅ Image download successful")
                print(f"   Downloaded {len(response.content)} bytes")
            else:
                print(f"❌ Failed: HTTP {response.status_code}")
                return False
        else:
            print(f"⚠️  No images to download")
    except Exception as e:
        print(f"❌ Error: {e}")
        return False
    
    # Test 6: Delete image
    print("\n6️⃣  Testing image deletion...")
    try:
        response = requests.get(f"{API_URL}/images", timeout=5)
        data = response.json()
        if len(data.get('images', [])) > 0:
            filename = data['images'][0]['filename']
            response = requests.delete(
                f"{API_URL}/delete-image/{filename}",
                timeout=5
            )
            
            if response.status_code == 200:
                print(f"✅ Image deleted successfully")
                print(f"   Deleted: {filename}")
            else:
                print(f"❌ Failed: HTTP {response.status_code}")
                return False
        else:
            print(f"⚠️  No images to delete")
    except Exception as e:
        print(f"❌ Error: {e}")
        return False
    
    # Final check
    print("\n7️⃣  Final verification...")
    try:
        response = requests.get(f"{API_URL}/images", timeout=5)
        data = response.json()
        count = len(data.get('images', []))
        print(f"✅ Current images in folder: {count}")
    except Exception as e:
        print(f"❌ Error: {e}")
        return False
    
    print("\n" + "=" * 60)
    print("✅ ALL TESTS PASSED!")
    print("=" * 60)
    return True


if __name__ == "__main__":
    try:
        success = test_image_api()
        sys.exit(0 if success else 1)
    except KeyboardInterrupt:
        print("\n\n❌ Test interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n\n❌ Unexpected error: {e}")
        sys.exit(1)
