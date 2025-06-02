from params import PROCEDURES_DIR, get_map_file_for_func_file


# Iterate over all .c files in the procedures directory
for func_path in PROCEDURES_DIR.iterdir():
    if func_path.is_dir() or func_path.suffix != '.c':
        continue
    # Check if the .c file is empty
    if func_path.stat().st_size != 0:
        continue
    map_path = get_map_file_for_func_file(func_path)
    # Delete the corresponding mapping file if it exists
    if not map_path.exists():
        map_path.unlink()
    if func_path.exists():
        func_path.unlink()
    print(f"DEL {func_path.stem}")
