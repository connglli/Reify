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

from params import DEFAULT_OUTPUT_DIR, get_func_map_files, get_prog_file
from ubchk import check_ubs, check_ubs_once
from utils import run_proc


def gen_func(
    exec,
    *,
    output,
    sano,
    uuid_val,
    with_main=False,
    verbose=False,
    seed=None,
    extra_opts=None,
    timeout=60,
):
    try:
        st_time = time.time()
        cmd = [exec]
        if with_main:
            cmd += ["-m"]
        if verbose:
            cmd += ["-v"]
        if seed:
            cmd += ["-s", str(seed)]
        if extra_opts:
            cmd += extra_opts.split()
        cmd += ["-o", str(output), "-n", str(sano), uuid_val]
        run_proc(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=True,
            timeout=timeout,
        )
        ed_time = time.time()
        return True, ed_time - st_time
    except CalledProcessError as e:
        print(
            f"GEN ERROR ({e.returncode}): {e.stdout.decode('utf-8') or '<no output>'}"
        )
        return False, None
    except subprocess.TimeoutExpired:
        print(f"GEN HANG (.): generation timeout (>{timeout}s)")
        return False, None


def main(*, output, limit, seed, mainf, check, timeout, extra_opts):
    if seed >= 0:
        random.seed(seed)
        next_seed = lambda: random.randint(0, 2147483647)
    else:
        next_seed = lambda: None
    func_uuid, func_sano = str(uuid.uuid4()), 0
    print(
        f"UUID={func_uuid}, output={output}, "
        f"limit={limit if limit != 0 else '<INF>'}, "
        f"seed={seed if seed >= 0 else '<RND>'}, "
        f"check={check}, timeout={timeout}s, "
        f"extra='{extra_opts}'"
    )
    while limit == 0 or func_sano < limit:
        print(f"[{func_sano}]: Generate ...", end=" ", flush=True)
        succ, elapsed = gen_func(
            "./build/bin/fgen",
            output=output,
            sano=func_sano,
            uuid_val=func_uuid,
            timeout=timeout,
            verbose=check,
            with_main=mainf,
            seed=next_seed(),
            extra_opts=extra_opts,
        )
        if succ:
            print(f"SUCC (time={elapsed}s)")
        if check and succ:
            print(f"[{func_sano}]: CheckUBs ...", end=" ", flush=True)
            check_ubs(*get_func_map_files(func_uuid, func_sano, gen_dir=output))
            if mainf:
                check_ubs_once(get_prog_file(func_uuid, func_sano, gen_dir=output))
            print("NO UBs")
        func_sano += 1


if __name__ == "__main__":
    parser = ArgumentParser(
        "fgen", description="Tool for generating a set of functions"
    )

    parser.add_argument(
        "--output",
        type=str,
        default=str(DEFAULT_OUTPUT_DIR),
        help="the directory saving the generated functions and their mappings",
    )
    parser.add_argument(
        "--limit",
        type=int,
        default=0,
        help="the number of functions to generate (0 for unlimited)",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=-1,
        help="the seed for generation (negative for truly random)",
    )
    parser.add_argument(
        "--main",
        action="store_true",
        default=False,
        help="enable the generation of a main function",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        default=False,
        help="enable UB check per generated function",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=15,
        help="timeout (in seconds) for generating a program",
    )
    parser.add_argument(
        "--extra", type=str, default="", help="extra options passed to fgen"
    )

    args = parser.parse_args()

    main(
        output=args.output,
        limit=args.limit,
        seed=args.seed,
        mainf=args.main,
        check=args.check,
        timeout=args.timeout,
        extra_opts=args.extra,
    )
