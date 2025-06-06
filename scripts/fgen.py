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

import random
import subprocess
import time
import uuid
from argparse import ArgumentParser
from subprocess import CalledProcessError

from params import get_func_map_files
from ubchk import check_ubs
from utils import run_proc


def gen_func(exec, sano, uuid_val, timeout=60, verbose=False, seed=None):
    try:
        st_time = time.time()
        cmd = [exec]
        if verbose: cmd += ["-v"]
        if seed:
            cmd += ["-s", str(seed)]
        cmd += ["-n", str(sano), uuid_val]
        run_proc(
            cmd,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            check=True, timeout=timeout
        )
        ed_time = time.time()
        return True, ed_time - st_time
    except CalledProcessError as e:
        print(f"GEN ERROR ({e.returncode}): {e.stdout.decode('utf-8') or '<no output>'}")
        return False, None
    except subprocess.TimeoutExpired:
        print(f"GEN HANG (.): generation timeout (>{timeout}s)")
        return False, None


def main(limit, seed, check, timeout):
    if seed >= 0:
        random.seed(seed)
        next_seed = lambda: random.randint(0, 2147483647)
    else:
        next_seed = lambda: None
    func_uuid, func_sano = str(uuid.uuid4()), 0
    print(f"UUID={func_uuid}, limit={limit if limit is not None else '<INF>'}, seed={seed if seed >= 0 else '<RND>'}, check={check}, timeout={timeout}s")
    while limit == 0 or func_sano < limit:
        print(f"[{func_sano}]: Generate ...", end=" ", flush=True)
        succ, elapsed = gen_func("./build/bin/fgen", func_sano, func_uuid, timeout, verbose=check, seed=next_seed())
        if succ:
            print(f"SUCC (time={elapsed}s)")
        if check and succ:
            print(f"[{func_sano}]: CheckUBs ...", end=" ", flush=True)
            check_ubs(*get_func_map_files(func_uuid, func_sano))
            print("NO UBs")
        func_sano += 1


if __name__ == '__main__':
    parser = ArgumentParser("fgen", description="Tool for generating a set of functions")

    parser.add_argument("--limit", type=int, default=0, help="the number of functions to generate (0 for unlimited)")
    parser.add_argument("--seed", type=int, default=-1, help="the seed for generation (negative for truly random)")
    parser.add_argument("--check", action="store_true", default=False, help="enable UB check per generated function")
    parser.add_argument("--timeout", type=int, default=60, help="timeout (in seconds) for generating a program")

    args = parser.parse_args()

    main(args.limit, seed=args.seed, check=args.check, timeout=args.timeout)
