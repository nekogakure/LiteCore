#!/usr/bin/env python3
"""
ext2ファイルシステムイメージを作成するスクリプト

使い方:
    python3 mk_ext2_image.py <output_image> <size_kb> [files_or_dir...]

例:
    python3 mk_ext2_image.py ext2.img 1024 test.txt readme.md
    python3 mk_ext2_image.py ext2.img 2048 test_data/  # ディレクトリ全体をコピー
"""

import sys
import os
import subprocess
import tempfile
import shutil

def create_ext2_image(output_path, size_kb, sources):
    """
    ext2ファイルシステムイメージを作成する
    
    Args:
        output_path: 出力イメージファイルのパス
        size_kb: イメージサイズ（KB）
        sources: コピーするファイルまたはディレクトリのリスト
    """
    
    # 一時ディレクトリを作成
    with tempfile.TemporaryDirectory() as tmpdir:
        img_path = os.path.join(tmpdir, "ext2.img")
        mount_point = os.path.join(tmpdir, "mnt")
        os.makedirs(mount_point)
        
        print(f"Creating ext2 image: {output_path} ({size_kb}KB)")
        
        # 空のイメージファイルを作成
        with open(img_path, 'wb') as f:
            f.write(b'\x00' * (size_kb * 1024))
        
        # ext2ファイルシステムを作成
        print("Creating ext2 filesystem...")
        result = subprocess.run(
            ['mkfs.ext2', '-F', img_path],
            capture_output=True,
            text=True
        )
        
        if result.returncode != 0:
            print(f"Error: mkfs.ext2 failed: {result.stderr}")
            return False
        
        # イメージをマウント（root権限が必要）
        print("Mounting image...")
        result = subprocess.run(
            ['sudo', 'mount', '-o', 'loop', img_path, mount_point],
            capture_output=True,
            text=True
        )
        
        if result.returncode != 0:
            print(f"Error: mount failed: {result.stderr}")
            print("Note: This script requires sudo privileges to mount the image.")
            return False
        
        try:
            # ファイルまたはディレクトリをコピー
            for source_path in sources:
                if not os.path.exists(source_path):
                    print(f"Warning: Not found: {source_path}")
                    continue
                
                if os.path.isdir(source_path):
                    # ディレクトリの場合、中身を再帰的にコピー
                    print(f"Copying directory contents from {source_path}...")
                    for root, dirs, files in os.walk(source_path):
                        # 相対パスを計算
                        rel_path = os.path.relpath(root, source_path)
                        if rel_path == '.':
                            dest_dir = mount_point
                        else:
                            dest_dir = os.path.join(mount_point, rel_path)
                        
                        # ディレクトリを作成
                        for dir_name in dirs:
                            dir_path = os.path.join(dest_dir, dir_name)
                            if not os.path.exists(dir_path):
                                result = subprocess.run(['sudo', 'mkdir', '-p', dir_path])
                                if result.returncode == 0:
                                    subprocess.run(['sudo', 'chmod', '755', dir_path])
                                    print(f"  Created directory: {os.path.relpath(dir_path, mount_point)}")
                        
                        # ファイルをコピー
                        for file_name in files:
                            src_file = os.path.join(root, file_name)
                            dest_file = os.path.join(dest_dir, file_name)
                            subprocess.run(['sudo', 'cp', src_file, dest_file])
                            subprocess.run(['sudo', 'chmod', '644', dest_file])
                            print(f"  Copied: {os.path.relpath(dest_file, mount_point)}")
                else:
                    # ファイルの場合
                    print(f"Copying file {source_path}...")
                    dest = os.path.join(mount_point, os.path.basename(source_path))
                    subprocess.run(['sudo', 'cp', source_path, dest])
                    subprocess.run(['sudo', 'chmod', '644', dest])
            
            # ファイルシステムを同期
            subprocess.run(['sync'])
            
        finally:
            # アンマウント
            print("Unmounting image...")
            subprocess.run(['sudo', 'umount', mount_point])
        
        # 最終イメージを出力先にコピー
        shutil.copy2(img_path, output_path)
        print(f"Successfully created: {output_path}")
        
        return True

def main():
    if len(sys.argv) < 3:
        print("Usage: mk_ext2_image.py <output_image> <size_kb> [files_or_dir...]")
        print("Example: mk_ext2_image.py ext2.img 1024 test.txt readme.md")
        print("         mk_ext2_image.py ext2.img 2048 test_data/")
        sys.exit(1)
    
    output_path = sys.argv[1]
    
    try:
        size_kb = int(sys.argv[2])
        if size_kb < 64:
            print("Error: Image size must be at least 64KB")
            sys.exit(1)
    except ValueError:
        print("Error: Size must be a number in KB")
        sys.exit(1)
    
    sources = sys.argv[3:] if len(sys.argv) > 3 else []
    
    if not create_ext2_image(output_path, size_kb, sources):
        sys.exit(1)

if __name__ == '__main__':
    main()
