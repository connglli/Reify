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

import subprocess
import time
import uuid
from argparse import ArgumentParser
from pathlib import Path
from subprocess import CalledProcessError

from params import get_simple_program, get_func_map_files, parse_mapping
from utils import run_proc


def check_ubs(func_path, map_path):
    func_code, func_name = func_path.read_text(), func_path.stem
    tmp_c_path, tmp_o_path = Path('/tmp/a.c'), Path('/tmp/a.o')
    for map_ind, (func_args, _) in enumerate(parse_mapping(map_path)):
        tmp_c_path.write_text(get_simple_program(func_name, func_code, func_args))
        try:
            run_proc(
                [
                    'gcc',
                    '-fno-tree-slsr', '-fno-ivopts', '-fsanitize=undefined',
                    '-o', str(tmp_o_path),
                    str(tmp_c_path)
                ],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                check=True, timeout=60  # allow 60s to compile
            )
        except subprocess.CalledProcessError as e:
            print(f"Skip {map_ind}: PCMP ERROR ({e.returncode}): {e.stdout.decode('utf-8')}")
            continue
        except subprocess.TimeoutExpired:
            print(f'Skip {map_ind}: PCMP HANG (.): timeout')
            continue
        try:
            p = run_proc([
                str(tmp_o_path)],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                check=True, timeout=120  # allow 120s to execute
            )
            out = p.stdout.decode('utf-8') or "<no output>"
            if "runtime error: " in out:
                print(f'UBs FOUND ({p.returncode}): {out}')
                print("===========================================")
                print(tmp_c_path.read_text())
                print("===========================================")
                exit(1)
        except subprocess.CalledProcessError as e:
            out = e.stdout.decode('utf-8') or "<no output>"
            if "runtime error: " in out:
                print(f'UBs FOUND ({e.returncode}): {out}')
                print("===========================================")
                print(tmp_c_path.read_text())
                print("===========================================")
                exit(1)
            print(f'Skip {map_ind}: PEXE ERROR ({e.returncode}): {out}')
        except subprocess.TimeoutExpired:
            print(f'Skip {map_ind}: PEXE HANG (.): timeout')
            continue


def gen_func(exec, sano, uuid_val, timeout=60, verbose=False):
    try:
        st_time = time.time()
        run_proc(
            [
                exec, "-n", str(sano), "-v", uuid_val
            ] if verbose else [
                exec, "-n", str(sano), uuid_val
            ],
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            check=True, timeout=timeout
        )
        ed_time = time.time()
        return True, ed_time - st_time
    except CalledProcessError as e:
        print(f"Skip: PGEN ERROR ({e.returncode}): {e.stdout.decode('utf-8')}")
        return False, None
    except subprocess.TimeoutExpired:
        print(f"Skip: PGEN HANG (.): timeout")
        return False, None


def main(limit, check, timeout):
    func_uuid, func_sano = str(uuid.uuid4()), 0
    print(f"UUID={func_uuid}, limit={limit if limit is not None else '<INF>'}, check={check}, timeout={timeout}s")
    while limit is None or func_sano < limit:
        print(f"[{func_sano}]: Generate ...", end=" ", flush=True)
        succ, elapsed = gen_func("./build/bin/fgen", func_sano, func_uuid, timeout, verbose=check)
        if succ:
            print(f"SUCC (time={elapsed}s)")
        if check and succ:
            print(f"[{func_sano}]: CheckUBs ...", end=" ", flush=True)
            check_ubs(*get_func_map_files(func_uuid, func_sano))
            print("NO UBs")
        func_sano += 1


if __name__ == '__main__':
    parser = ArgumentParser("fgen", description="Tool for generating a set of functions")

    parser.add_argument("--limit", type=int, default=None, help="the number of functions to generate")
    parser.add_argument("--check", action="store_true", default=False, help="enable UB check per generated function")
    parser.add_argument("--timeout", type=int, default=60, help="timeout (in seconds) for generating a program")

    args = parser.parse_args()

    main(args.limit, check=args.check, timeout=args.timeout)
