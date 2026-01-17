import os
import json
import re
from wasmtime import Engine, Store, Module, Instance

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
BASE_DIR = os.path.dirname(SCRIPT_DIR)
MAPPINGS_DIR = os.path.join(BASE_DIR, 'generated', 'mappings')
WASM_DIR = os.path.join(BASE_DIR, 'generated', 'wasm')

def run_fuzz_tests():
  engine = Engine()
  
  if not os.path.exists(MAPPINGS_DIR):
    print(f"Error: Mappings directory not found at {MAPPINGS_DIR}")
    return

  mapping_files = sorted([f for f in os.listdir(MAPPINGS_DIR) if f.endswith('.jsonl')])
  
  stats = {"pass": 0, "fail": 0, "trap": 0, "error": 0}
  total_files = len(mapping_files)

  print(f"--- Starting Fuzzing Run: {total_files} suites ---")

  for idx, mapping_file in enumerate(mapping_files, 1):
    match = re.search(r'function_(.+)\.jsonl', mapping_file)
    if not match: continue
      
    uuid_id = match.group(1)
    wat_path = os.path.join(WASM_DIR, f"{uuid_id}.wat")
    jsonl_path = os.path.join(MAPPINGS_DIR, mapping_file)
    
    if not os.path.exists(wat_path):
      print(f"[{idx}/{total_files}] [-] {uuid_id}: SKIP (WAT missing)")
      continue

    try:
      store = Store(engine)
      module = Module.from_file(engine, wat_path)
      instance = Instance(store, module, [])
      
      func_name = f"function_{uuid_id}"
      wasm_func = instance.exports(store).get(func_name)
      
      if not wasm_func:
        print(f"[{idx}/{total_files}] [!] {uuid_id}: FAIL (Export missing)")
        stats["fail"] += 1
        continue

      all_lines_passed = True
      with open(jsonl_path, 'r') as f:
        for line_idx, line in enumerate(f):
          if not line.strip(): continue
          data = json.loads(line)
          
          # prepare input params
          inputs = [item['elems'][0] for item in data['ini']]
          
          # extract ALL expected values from 'fin'
          expected_list = [item['elems'][0] for item in data['fin']]

          try:
            # invoke wasm (actual will be a list/tuple for multi-value returns)
            actual = wasm_func(store, *inputs)
            
            # present 'actual' values as a list for comparison
            if isinstance(actual, (list, tuple)):
              actual_list = list(actual)
            else:
              actual_list = [actual]

            # i32 signedness: wrap to 32-bit signed integers
            actual_list = [(x + 2**31) % 2**32 - 2**31 for x in actual_list]

            if actual_list != expected_list:
              print(f"[{idx}/{total_files}] [X] {uuid_id}: FAIL (Line {line_idx+1})")
              print(f"  Expected: {expected_list}")
              print(f"  Got:    {actual_list}")
              all_lines_passed = False
              stats["fail"] += 1
              break
          except Exception as trap:
            print(f"[{idx}/{total_files}] [T] {uuid_id}: TRAP (Line {line_idx+1}: {trap})")
            all_lines_passed = False
            stats["trap"] += 1
            break
      
      if all_lines_passed:
        print(f"[{idx}/{total_files}] [+] {uuid_id}: PASS")
        stats["pass"] += 1

    except Exception as e:
      print(f"[{idx}/{total_files}] [E] {uuid_id}: SYSTEM ERROR ({str(e)[:100]})")
      stats["error"] += 1

  # --- FINAL SUMMARY ---
  print("\n" + "="*30)
  print("FUZZING SUMMARY")
  print("="*30)
  print(f"TOTAL SUITES: {total_files}")
  print(f"PASSED:     {stats['pass']}")
  print(f"FAILED:     {stats['fail']}")
  print(f"TRAPPED:    {stats['trap']}")
  print(f"ERRORS:     {stats['error']}")
  print("="*30)

if __name__ == "__main__":
  run_fuzz_tests()