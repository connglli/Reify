import subprocess
import sys
from enum import Enum, unique
from pathlib import Path

from params import parse_mapping, get_simple_program, get_map_file_for_func_file
from utils import run_proc


@unique
class UBChkRes(Enum):
    CMP_ERROR = 0
    CMP_HANG = 1
    EXE_ERROR = 3
    EXE_HANG = 4
    UB_FOUND = 5
    UB_FREE = 6


def _ub_exists_in(txt):
    return "runtime error: " in txt


def _check_ubs(c_file, o_file='/tmp/a.out', cmp_tmo=60, exe_tmo=120):
    try:
        run_proc(
            [
                'gcc',
                '-fno-tree-slsr', '-fno-ivopts', '-fsanitize=undefined',
                '-o', str(o_file),
                str(c_file)
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=True, timeout=cmp_tmo
        )
    except subprocess.CalledProcessError as e:
        return UBChkRes.CMP_ERROR, e.returncode, e.stdout.decode('utf-8') or '<no output>'
    except subprocess.TimeoutExpired:
        return UBChkRes.CMP_HANG, 0, f'compilation timeout (>{cmp_tmo}s)'
    try:
        p = run_proc(
            [str(o_file)],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=True, timeout=exe_tmo
        )
        out = p.stdout.decode('utf-8') or '<no output>'
        if _ub_exists_in(out):
            return UBChkRes.UB_FOUND, p.returncode, out
        return UBChkRes.UB_FREE, p.returncode, out
    except subprocess.CalledProcessError as e:
        out = e.stdout.decode('utf-8') or "<no output>"
        if _ub_exists_in(out):
            return UBChkRes.UB_FOUND, e.returncode, out
        return UBChkRes.EXE_ERROR, e.returncode, out
    except subprocess.TimeoutExpired:
        return UBChkRes.EXE_HANG, 0, f'execution timeout (>{exe_tmo}s)'


def check_ubs_once(c_file):
    ub_res, retc, out = _check_ubs(c_file)
    if ub_res == UBChkRes.UB_FOUND:
        print(f'****************************')
        print(f'********* FOUND UB *********')
        print(f'****************************')
        print(f'Func : {c_file.absolute()}')
        print(f'Retc : {retc}')
        print(f'Mesg : {out}')
        print("`````````````````````````````")
        print(c_file.read_text())
        print("`````````````````````````````")
        exit(1)
    elif ub_res == UBChkRes.CMP_ERROR:
        print(f"CMP ERROR ({retc}): {out}")
    elif ub_res == UBChkRes.CMP_HANG:
        print(f'CMP HANG (.): {out}')
    elif ub_res == UBChkRes.EXE_ERROR:
        print(f'EXE ERROR ({retc}): {out}')
    elif ub_res == UBChkRes.EXE_HANG:
        print(f'EXE HANG (.): {out}')
    elif ub_res == UBChkRes.UB_FREE:
        pass


def check_ubs(func_path, map_path):
    func_code, func_name = func_path.read_text(), func_path.stem
    tmp_c_path, tmp_o_path = Path('/tmp/a.c'), Path('/tmp/a.o')
    for map_ind, (func_args, _) in enumerate(parse_mapping(map_path)):
        tmp_c_path.write_text(get_simple_program(func_name, func_code, func_args))
        ub_res, retc, out = _check_ubs(tmp_c_path, o_file=str(tmp_o_path))
        if ub_res == UBChkRes.UB_FOUND:
            print(f'****************************')
            print(f'********* FOUND UB *********')
            print(f'****************************')
            print(f'Func : {func_path.absolute()}')
            print(f'Maps : {map_path.absolute()}')
            print(f'Retc : {retc}')
            print(f'Mesg : {out}')
            print("`````````````````````````````")
            print(tmp_c_path.read_text())
            print("`````````````````````````````")
            exit(1)
        elif ub_res == UBChkRes.CMP_ERROR:
            print(f"{map_ind}: CMP ERROR ({retc}): {out}")
        elif ub_res == UBChkRes.CMP_HANG:
            print(f'{map_ind}: CMP HANG (.): {out}')
        elif ub_res == UBChkRes.EXE_ERROR:
            print(f'{map_ind}: EXE ERROR ({retc}): {out}')
        elif ub_res == UBChkRes.EXE_HANG:
            print(f'{map_ind}: EXE HANG (.): {out}')
        elif ub_res == UBChkRes.UB_FREE:
            pass


if __name__ == '__main__':
    if len(sys.argv) != 2 and len(sys.argv) != 3:
        print("Usage 1: ubchk <procedure_dir> <mapping_dir>")
        print("Usage 2: ubchk <new_procedure_dir>")
        exit(1)
    proc_dir = Path(sys.argv[1])
    map_dir = Path(sys.argv[2]) if len(sys.argv) == 3 else None
    for func_file in proc_dir.iterdir():
        if not func_file.is_file(): continue
        print(f"Check {func_file.stem} ...")
        if map_dir:
            map_file = get_map_file_for_func_file(func_file, mappings_dir=map_dir)
            check_ubs(func_file, map_file)
        else:
            check_ubs_once(func_file)

