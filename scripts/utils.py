import os
import signal
from subprocess import Popen, CompletedProcess, CalledProcessError


def safe_killpg(pid, sig):
    try:
        os.killpg(pid, sig)
    except ProcessLookupError:
        pass  # Ignore if there is no such process


# Fix: subprocess.run(cmd) series methods, when timed out, only sends a SIGTERM
# signal to cmd while does not kill cmd's subprocess. We let each command to run
# in a new process group by adding start_new_session flag, and kill the whole
# process group such that all cmd's subprocess are also killed when timed out.
def run_proc(cmd, stdout, stderr, timeout, check):
    with Popen(cmd, stdout=stdout, stderr=stderr, start_new_session=True) as proc:
        try:
            output, err_msg = proc.communicate(timeout=timeout)
        except:  # Including TimeoutExpired, KeyboardInterrupt, communicate handled that.
            safe_killpg(os.getpgid(proc.pid), signal.SIGKILL)
            # We don't call proc.wait() as .__exit__ does that for us.
            raise
        retcode = proc.poll()
    if check and retcode != 0:
        raise CalledProcessError(retcode, cmd, output, err_msg)
    return CompletedProcess(proc.args, retcode, output, err_msg)
