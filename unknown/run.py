import subprocess
import time
import uuid
import signal

def run_with_timeout(executable, number, uuid_val, timeout=60):
    try:
        #link stdout to stdout of run.py
        process = subprocess.Popen([executable, str(number), uuid_val], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        start_time = time.time()
        isProcessCompleted = False
        exit_code = None
        while time.time() - start_time < timeout:
            if process.poll() is not None: 
                isProcessCompleted = True
                print("Process completed.")
                exit_code = process.returncode
                if(exit_code == 1):
                    print("Process exited with error code:", exit_code)
                break
            time.sleep(1)
        if(not isProcessCompleted):
            print("Process timed out. Sending user interrupt (SIGINT).")
            process.send_signal(signal.SIGINT)
            # Wait for the process to terminate and get the exit code
            #if the process is not terminated, kill it
            process.wait(timeout=30)
            if process.poll() is None:
                print("Process still running after SIGINT. Killing it.")
                process.kill()
            process.wait()
            exit_code = process.returncode
            print(f"Process exited with code: {exit_code}")
        # if exit_code == 0:
        #     print("Running ./link.sh" + str(number))
        #     process = subprocess.Popen(["./link.sh", str(number)])
        #     process.wait()
        #     print("Shell script completed successfully.")
    except Exception as e:
        print(f"Error occurred: {e}")
counter = 0
uuid_val = str(uuid.uuid4())
# subprocess.run(["source", "/opt/intel/oneapi/setvars.sh", "intel64"], shell=True, executable="/bin/bash")
while(True):
    print(counter)
    run_with_timeout("./exe", counter, uuid_val)
    counter += 1