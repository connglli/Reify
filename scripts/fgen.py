import signal
import subprocess
import time
import uuid
from pathlib import Path

MAIN_TEMPLATE = """
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
// We won't have more than 20 arguments, so use 20 here
#define __COUNT_ARGS__(...) \\
    __COUNT_ARGS_IMPL__(__VA_ARGS__, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#define __COUNT_ARGS_IMPL__(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, ...) N
uint32_t g[256];
int32_t uint32_to_int32(uint32_t u){{
    return u%2147483647;
}}
void generate_crc32_table(uint32_t* crc32_table) {{
    const uint32_t poly = 0xEDB88320UL;
    for (uint32_t i = 0; i < 256; i++) {{
        uint32_t crc = i;
        for (int j = 8; j > 0; j--) {{
            if (crc & 1) {{
                crc = (crc >> 1) ^ poly;
            }} else {{
                crc >>= 1;
            }}
        }}
        crc32_table[i] = crc;
    }}
}}
uint32_t context_free_crc32_byte(uint32_t context, uint8_t b) {{
    return ((context >> 8) & 0x00FFFFFF) ^ g[(context ^ b) & 0xFF];
}}
uint32_t context_free_crc32_4bytes(uint32_t context, uint32_t val) {{
    context = context_free_crc32_byte(context, (val >> 0) & 0xFF);
    context = context_free_crc32_byte(context, (val >> 8) & 0xFF);
    context = context_free_crc32_byte(context, (val >> 16) & 0xFF);
    context = context_free_crc32_byte(context, (val >> 24) & 0xFF);
    return context;
}}
int computeStatelessChecksumImpl(int num_args, ...)
{{
    va_list args;
    va_start(args, num_args);
    uint32_t checksum = 0xFFFFFFFFUL;
    for (int i = 0; i < num_args; ++i) {{
        int arg = va_arg(args, int);
        checksum = context_free_crc32_4bytes(checksum, arg);
    }}
    checksum = uint32_to_int32(checksum ^ 0xFFFFFFFFUL);
    va_end(args);
    return checksum;
}}
// These are hacks as procedure_gen did not count the number of arguments ...
#define computeStatelessChecksum(...) \\
    computeStatelessChecksumImpl(__COUNT_ARGS__(__VA_ARGS__), __VA_ARGS__)
{proc_code}
int main(int argc, int* argv[]) {{
    generate_crc32_table(g);
    printf("%d", {proc_name}({proc_args}));
    return 0;
}}
"""


def check_ubs(fname, mname):
    fpath, mpath = Path(fname), Path(mname)
    proc_code = fpath.read_text()
    proc_name = fpath.stem
    for mm in mpath.read_text().splitlines():
        tmp_c, tmp_o = '/tmp/a.c', '/tmp/a.o'
        proc_args = mm.split(' : ')[0].rstrip(',')
        Path(tmp_c).write_text(
            MAIN_TEMPLATE.format(
                proc_code=proc_code,
                proc_name=proc_name,
                proc_args=proc_args
            )
        )
        try:
            subprocess.run(
                ['gcc', '-fno-tree-slsr', '-fno-ivopts', '-fsanitize=undefined', '-o', tmp_o, tmp_c],
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True
            )
        except subprocess.CalledProcessError as e:
            print(f'Skip: PCMP ERROR ({e.returncode}): {e.stdout}')
            return
        try:
            p = subprocess.run([tmp_o], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)
        except subprocess.CalledProcessError as e:
            out = e.stdout.decode('utf-8')
            if "runtime error: " in out:
                print(f'UBs FOUND ({e.returncode}): {out}')
                print(f"{proc_name}({proc_args})")
                exit(1)
            else:
                print(f'Skip: PEXE ERROR ({e.returncode}): {out}')
        else:
            out = p.stdout.decode('utf-8')
            if "runtime error: " in out:
                print(f'UBs FOUND ({p.returncode}): {out}')
                exit(1)


def check_run(exec, index, uuid_val, timeout=60):
    # Link stdout to stdout of run.py
    process = subprocess.Popen([exec, str(index), uuid_val], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    start_time = time.time()
    isProcessCompleted = False
    while time.time() - start_time < timeout:
        if process.poll() is not None:
            isProcessCompleted = True
            exit_code = process.returncode
            output, _ = process.communicate()
            output = output.decode('utf-8')
            if exit_code == 1:
                print(f"Skip PGEN ERROR ({exit_code}): {output}")
                return False
            break
        time.sleep(1)
    if not isProcessCompleted:
        process.send_signal(signal.SIGINT)
        # Wait for the process to terminate and get the exit code
        process.wait(timeout=30)
        # If the process is not terminated, kill it
        if process.poll() is None:
            process.kill()
        process.wait()
        print(f"Skip PGEN ERROR (timeout)")
        return False
    return True


def main(count=None, check=False):
    uuid_val = str(uuid.uuid4())
    index = 0
    while (count is None) or (index < count):
        print(index)
        succ = check_run("./build/bin/fgen", index, uuid_val)
        if check and succ:
            fuuid_, fsano = uuid_val.replace('-', '_'), index
            check_ubs(f"./procedures/function_{fuuid_}_{fsano}.c", f"./mappings/function_{fuuid_}_{fsano}_mapping")
        index += 1


if __name__ == '__main__':
    # By default, generate unlimited functions without any UB checks
    main(None, False)
