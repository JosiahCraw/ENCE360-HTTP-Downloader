import timeit
import os
import time
import matplotlib.pyplot as plt

if __name__ == "__main__":
    num_threads = [1,2,5,10,12,16,20,25,32,64,128]
    out_times = list()
    file_name = 'large.txt'
    for num in num_threads:
        cmd = './downloader ' + file_name + ' ' + str(num) + ' downloads'
        curr = time.time()
        os.system(cmd)
        out_times.append(time.time()-curr)
    print(out_times)
    plt.plot(num_threads, out_times)
    plt.ylabel('Time (s)')
    plt.xlabel('Num threads')
    plt.show()
    
