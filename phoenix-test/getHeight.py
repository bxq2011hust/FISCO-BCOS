from prettytable import PrettyTable
import os
import glob
import time
import numpy as np

cur_block = [0,0,0,0,0,0,0]
def get_nodes(number):
    for i in range(0,number):
        dirname = "./node"+str(i)+"/log/*.log"
        list_of_files = glob.glob(dirname)
        latest_file = max(list_of_files, key=os.path.getctime)
        # print(latest_file)
        # f = open(latest_file)
        # print(f)
        # print(open(latest_file,'rb').readlines())
        for line in reversed(list(open(latest_file,'rb').readlines())):
            try:
                if "Notify blocknumber" in line.decode():
                    # print(int(line.decode().split(":")[-1].strip()))
                    cur_block[i] = int(line.decode().split(":")[-1].strip())
                    # print(cur_block[i])
                    break
            except Exception as e:
                if "invalid continuation byte" not in str(e):
                    print(e)
                continue

get_nodes(7)
# print(cur_block)
avg_var = np.mean(cur_block)
for b in cur_block:
    if (avg_var - b) > 100 or (b - avg_var) > 100:
        print("0")
        exit()
print("1")
