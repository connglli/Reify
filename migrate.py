import os
import shutil
import sys
# Set source and destination directories

source_dir = sys.argv[1] if len(sys.argv) > 1 else "/path/to/source"
destination_dir = sys.argv[2] if len(sys.argv) > 2 else "/path/to/destination"

# Ensure destination directory exists
os.makedirs(destination_dir, exist_ok=True)

# Go through each subdirectory in the source
for sub in os.listdir(source_dir):
    sub_path = os.path.join(source_dir, sub)
    if os.path.isdir(sub_path):
        # Prepare the corresponding subdirectory in destination
        dest_sub_path = os.path.join(destination_dir, sub)
        os.makedirs(dest_sub_path, exist_ok=True)

        # Copy only files (not subdirectories) from the current subdirectory
        for item in os.listdir(sub_path):
            item_path = os.path.join(sub_path, item)
            if os.path.isfile(item_path):
                shutil.copy2(item_path, dest_sub_path)