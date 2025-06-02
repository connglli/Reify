import subprocess
import time
import uuid
import signal
from pathlib import Path

MAIN_TEMPLATE = """
#include <stdarg.h>
unsigned int g[256];
int ac, i, ad;
void ax(unsigned int *a){{
  unsigned int c = 3988292384;
  for (unsigned int d = 0; d < 256; d++){{
    unsigned int aj = d;
    for (int j = 8; j; j--)
      if (aj & 1)
        aj = aj >> 1 ^ c;
      else
        aj >>= 1;
    a[d] = aj;
  }}
}}
unsigned int ak(unsigned int as, unsigned char b){{ return as >> 8 ^ g[(as ^ b) & 255]; }}
unsigned int at(unsigned int as, unsigned int e){{
  as = ak(as, e);
  as = ak(as, e >> 8);
  as = ak(as, e >> 16);
  as = ak(as, e >> 24);
  return as;
}}
int computeStatelessChecksum(int av, ...){{
  va_list args;
  va_start(args, av);
  unsigned int aa = 4294967295;
  for (int d = 0; d < av; ++d) {{
    int ab = va_arg(args, int);
    aa = at(aa, ab);
  }}
  aa = aa ^ 4294967295;
  return aa;
}}
{proc_code}
int main(int argc, int* argv[]) {{
    ax(g);
    {proc_name}({proc_args});
    return 0;
}}
"""


def check_ubs(fname, mname):
    fpath, mpath = Path(fname), Path(mname)
    proc_code = fpath.read_text()
    proc_name = fpath.stem
    for mm in mpath.read_text().splitlines():
        proc_args = mm.split(' : ')[0].rstrip(',')
        Path('/tmp/a.c').write_text(
            MAIN_TEMPLATE.format(
                proc_code=proc_code,
                proc_name=proc_name,
                proc_args=proc_args
            )
        )
        try:
            subprocess.run(['gcc', '-fno-tree-slsr', '-fno-ivopts', '-fsanitize=undefined', '-o', '/tmp/a.o', '/tmp/a.c'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)
        except subprocess.CalledProcessError as e:
            print(f'Skip: PCMP ERROR ({e.returncode}): {e.stdout}')
            return
        try:
            p = subprocess.run(['/tmp/a.o'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)
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
    exit_code = None
    output = None
    while time.time() - start_time < timeout:
        if process.poll() is not None: 
            isProcessCompleted = True
            exit_code = process.returncode
            output, _ = process.communicate()
            output = output.decode('utf-8')
            if exit_code == 1:
                print(f"Skip PGEN ERROR ({exit_code}): {output}")
                return None
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
        exit_code = process.returncode
        print(f"Skip PGEN ERROR (timeout)")
        return None
    return output.splitlines()


def main(count=1_000_000):
    uuid_val = str(uuid.uuid4())
    index = 0
    while index < count:
        print(index)
        out = check_run("./build/bin/fgen", index, uuid_val)
        if out:
            check_ubs(*out)
        index += 1


if __name__ == '__main__':
    main(1_000)
