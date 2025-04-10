import subprocess
import time
import signal

def run_with_timeout(executable, number, timeout=60):
    try:
        #link stdout to stdout of run.py
        process = subprocess.Popen([executable, str(number)])
        start_time = time.time()
        isProcessCompleted = False
        exit_code = None
        while time.time() - start_time < timeout:
            if process.poll() is not None: 
                isProcessCompleted = True
                print("Process completed.")
                exit_code = process.returncode
                break
            time.sleep(1)
        if(not isProcessCompleted):
            print("Process timed out. Sending user interrupt (SIGINT).")
            process.send_signal(signal.SIGINT)
            process.wait()
            exit_code = process.returncode
            print(f"Process exited with code: {exit_code}")
        else:
            if exit_code != 1:
                print("Running ./link.sh" + str(number))
                process = subprocess.Popen(["./link.sh", str(number)])
                process.wait()
                print("Shell script completed successfully.")
    except Exception as e:
        print(f"Error occurred: {e}")
counter = 0
# subprocess.run(["source", "/opt/intel/oneapi/setvars.sh", "intel64"], shell=True, executable="/bin/bash")
while(True):
    print(counter)
    run_with_timeout("./exe", counter)
    counter += 1