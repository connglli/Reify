#  MIT License
#
#  Copyright (c) 2025.
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

import os
import signal
from subprocess import Popen, CompletedProcess, CalledProcessError, STDOUT, PIPE
from typing import List


def safe_killpg(pid, sig):
  try:
    os.killpg(pid, sig)
  except ProcessLookupError:
    pass  # Ignore if there is no such process


# Fix: subprocess.run(cmd) series methods, when timed out, only sends a SIGTERM
# signal to cmd while does not kill cmd's subprocess. We let each command to run
# in a new process group by adding start_new_session flag, and kill the whole
# process group such that all cmd's subprocess are also killed when timed out.
def run_proc(cmd: List[str], *, stdout=PIPE, stderr=STDOUT, timeout=None, check=False):
  with Popen(cmd, stdout=stdout, stderr=stderr, start_new_session=True) as proc:
    try:
      output, err_msg = proc.communicate(timeout=timeout)
    except:  # Including TimeoutExpired, KeyboardInterrupt, communicate handled that.
      safe_killpg(os.getpgid(proc.pid), signal.SIGKILL)
      # We don't call proc.wait() as .__exit__ does that for us.
      raise
    retcode = proc.poll()
    output = output.decode('utf-8') if output else ''
    err_msg = err_msg.decode('utf-8') if err_msg else ''
  if check and retcode != 0:
    raise CalledProcessError(retcode, cmd, output, err_msg)
  return CompletedProcess(proc.args, retcode, output, err_msg)


def check_run(cmd: List[str], *, timeout=None):
  return run_proc(cmd, stdout=PIPE, stderr=STDOUT, timeout=timeout, check=True).returncode


def check_out(cmd: List[str], *, timeout=None):
  proc = run_proc(cmd, stdout=PIPE, stderr=STDOUT, timeout=timeout, check=True)
  return proc.stdout
