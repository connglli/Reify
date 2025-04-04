import subprocess
import time
import signal

def run_with_timeout(executable, number, timeout=60):
    try:
        process = subprocess.Popen([executable, str(number)], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        start_time = time.time()
        while time.time() - start_time < timeout:
            if process.poll() is not None: 
                print("Process completed successfully.")
                exit_code = process.returncode
                if exit_code != 1:
                    print("Running ./link.sh" + str(number))
                    process = subprocess.Popen(["./link.sh", str(number)])
                return
            time.sleep(1) 
        print("Process timed out. Sending user interrupt (SIGINT).")
        process.send_signal(signal.SIGINT)
        process.wait()  # Ensure it properly terminates
        exit_code = process.returncode
        print(f"Process exited with code: {exit_code}")
    except Exception as e:
        print(f"Error occurred: {e}")
counter = 0
while(True):
    print(counter)
    run_with_timeout("./exe", counter)
    counter += 1