import time
import queue
import numpy as np
from matplotlib import pyplot as plt
import scipy
import rx888
from threading import Thread

dev = rx888.Rx888(0, sample_rate=64000000, nbuf=64, bufsz=8192*16*64)
thread = Thread(target=dev.start_async())
thread.start()
t0 = time.time();
print("Starting read")
tplt = time.time();
try:
    while True:
        sample_buf = dev.read(1)
        if time.time() > tplt:
            plt.clf();
            plt.plot(sample_buf[:200])
            plt.pause(0.5)
            tplt=time.time()+1

except KeyboardInterrupt:
    pass
print("got", sample_buf.shape, sample_buf.dtype, "samples in", time.time()-t0, "seconds")
dev.cancel_async()
thread.join()
print("sample_buf", sample_buf[100000:100100])
plt.plot(sample_buf[:110000])
plt.show()
