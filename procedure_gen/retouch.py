import os

# Define your directories
procedures_dir = 'procedures'
mappings_dir = 'mappings'

# Iterate over all .c files in the procedures directory
for filename in os.listdir(procedures_dir):
    if filename.endswith('.c'):
        procedure_path = os.path.join(procedures_dir, filename)
        # Check if the .c file is empty
        if os.path.getsize(procedure_path) == 0:
            base_name = filename[:-2]  # Strip the '.c'
            mapping_filename = f"{base_name}_mapping"
            mapping_path = os.path.join(mappings_dir, mapping_filename)
            # Delete the corresponding mapping file if it exists
            if os.path.exists(mapping_path):
                os.remove(mapping_path)
            if os.path.exists(procedure_path):
                os.remove(procedure_path)
                print(f"Deleted: {mapping_path}")
