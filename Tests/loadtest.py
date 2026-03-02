import threading
import os
#Windows
#COMMAND = "C:\\Opensource\Wget\wget.exe --no-check-certificate "
#Mac OS tahoe
COMMAND = "/opt/local/bin/wget --no-check-certificate "
ITERATIONS = 10000
THREAD_COUNT = 100

urls = [
	"http://127.0.0.1:8080/frompython?",
    "http://[::1]:8080/frompython?",
    "https://127.0.0.1:8081/frompython?",
    "https://[::1]:8081/frompython?",
    "http://127.0.0.1:8080/frompython2?",
    "http://[::1]:8080/frompython2?",
    "https://127.0.0.1:8081/frompython2?",
    "https://[::1]:8081/frompython2?",
    "http://127.0.0.1:8080/frompython3?",
    "http://[::1]:8080/frompython3?",
    "https://127.0.0.1:8081/frompython3?",
    "https://[::1]:8081/frompython3?",
    "http://127.0.0.1:8080/sendimage?",
    "http://[::1]:8080/sendimage?",
    "https://127.0.0.1:8081/sendimage?",
    "https://[::1]:8081/sendimage?"
]



def worker(thread_id):
    for i in range(ITERATIONS):
        print(" Thread {thread_id} - Request {i} " )
	# Windows
        #os.system(COMMAND + " -O " + "OutputFiles\\output." + str(thread_id)+"."+str(i) + "    "+urls[i%16] )
	# Mac OS tahoe
        os.system(COMMAND + " -O " + "OutputFiles/output." + str(thread_id)+"."+str(i) + "    "+urls[i%16] )
    print(f"Thread {thread_id} finished")

os.system("mkdir OutputFiles")
threads = []

for t in range(THREAD_COUNT):
    th = threading.Thread(target=worker, args=(t,))
    threads.append(th)
    th.start()

for th in threads:
    th.join()

print("All threads completed")

