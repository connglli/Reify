#  MIT License
#
#  Copyright (c) 2025
#
#  Kavya Chopra (chopra.kavya04@gmail.com)
#  Cong Li (cong.li@inf.ethz.ch)
#
#  Permission is hereby granted, free of charge, to any person obtaining a copy
#  of this software and associated documentation files (the "Software"), to deal
#  in the Software without restriction, including without limitation the rights
#  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#  copies of the Software, and to permit persons to whom the Software is
#  furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in all
#  copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#  SOFTWARE.

import signal
import subprocess
import time
import uuid
from argparse import ArgumentParser
from pathlib import Path

from params import get_simple_program, get_func_map_files, parse_mapping


def check_ubs(func_path, map_path):
    func_code, func_name = func_path.read_text(), func_path.stem
    tmp_c_path, tmp_o_path = Path('/tmp/a.c'), Path('/tmp/a.o')
    for func_args, _ in parse_mapping(map_path):
        tmp_c_path.write_text(get_simple_program(func_name, func_code, func_args))
        try:
            subprocess.run(
                [
                    'gcc',
                    '-fno-tree-slsr', '-fno-ivopts', '-fsanitize=undefined',
                    '-o', str(tmp_o_path),
                    str(tmp_c_path)
                ],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                check=True
            )
        except subprocess.CalledProcessError as e:
            print(f'Skip: PCMP ERROR ({e.returncode}): {e.stdout}')
            continue
        try:
            p = subprocess.run([
                str(tmp_o_path)],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                check=True
            )
            out = p.stdout.decode('utf-8') or "<no output>"
            if "runtime error: " in out:
                print(f'UBs FOUND ({p.returncode}): {out}')
                exit(1)
        except subprocess.CalledProcessError as e:
            out = e.stdout.decode('utf-8') or "<no output>"
            if "runtime error: " in out:
                print(f'UBs FOUND ({e.returncode}): {out}')
                print(f"{proc_name}({proc_args})")
                exit(1)
            print(f'Skip: PEXE ERROR ({e.returncode}): {out}')


def gen_func(exec, index, uuid_val, timeout=60):
    # Link stdout to stdout of run.py
    process = subprocess.Popen([exec, str(index), uuid_val], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    st_time = time.time()
    completed = False
    exit_code = None
    while time.time() - st_time < timeout:
        if process.poll() is not None:
            completed = True
            exit_code = process.returncode
            output, _ = process.communicate()
            output = output.decode('utf-8') or "<no output>"
            if exit_code == 1:
                print(f"Skip PGEN ERROR ({exit_code}): {output}")
                return False
            break
        time.sleep(1)
    if not completed:
        process.send_signal(signal.SIGINT)
        # Wait for the process to terminate and get the exit code
        process.wait(timeout=30)
        # If the process is not terminated, kill it
        if process.poll() is None:
            process.kill()
        process.wait()
        exit_code = process.returncode
        print(f"Skip PGEN ERROR (0): timeout")
        return False
    return True


def main(limit, check, timeout):
    func_uuid, func_index = str(uuid.uuid4()), 0
    print(f"UUID={func_uuid}")
    while func_index < limit:
        print(f"[{func_index}]: Generate ...", end=" ")
        succ = gen_func("./build/bin/fgen", func_index, func_uuid, timeout)
        if succ:
            print("Succeeded")
        if check and succ:
            print(f"[{func_index}]: CheckUBs ...", end=" ")
            check_ubs(*get_func_map_files(func_uuid, func_index))
            print("NO UBs")
        func_index += 1


if __name__ == '__main__':
    parser = ArgumentParser("fgen", description="Tool for generating a set of functions")

    parser.add_argument("--limit", type=int, default=1_000_000, help="the number of functions to generate")
    parser.add_argument("--check", action="store_true", default=False, help="enable UB check per generated function")
    parser.add_argument("--timeout", type=int, default=60, help="timeout (in seconds) for generating a program")

    args = parser.parse_args()

    main(args.limit, check=args.check, timeout=args.timeout)
