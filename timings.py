import timeit
import os
import time

if __name__ == "__main__":
    num_threads = [1,2,5,10,12,16,20,25,32,64,128,256]
    out_times = list()
    file_name = 'large.txt'
    for num in num_threads:
        cmd = './downloader ' + file_name + ' ' + str(num) + ' downloads'
        curr = time.time()
        os.system(cmd)
        out_times.append(time.time()-curr)
    print(out_times)
    
