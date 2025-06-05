import signal
import subprocess
import time
import uuid
from ubchk import check_ubs


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
