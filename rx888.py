from cffi import FFI
import time
import queue
import numpy as np
import copy
import ringbuffer.ring_buffer
def prepare_for_cffi(data):
    #Quick hack to get cffi to accept header
    out = []
    for line in data.splitlines():
        if "#" not in line:
            if 'extern "C"' not in line:
                out.append(line)
    out = '\n'.join(out)
    return out

ffi = FFI()
ffi.cdef(prepare_for_cffi(open("./rx888/target/release/libsddc.h").read()));
lib = ffi.dlopen("./rx888/target/release/libsddc.so")

class Rx888:
    def __init__(self, index=0, sample_rate=64000000, nbuf=512, bufsz=8192*64):
        print("Buffer rate:", 1/(bufsz/sample_rate), "Hz")
        print("Total buffer time:", nbuf*bufsz/sample_rate)
        self.rb = ringbuffer.ring_buffer.RingBuffer(nbuf, bufsz)
        self.sample_rate = sample_rate
        self.index = index
        self.phandle = ffi.new("struct sddc_dev_t **")
        fval = ffi.new("float [1]")
        uival = ffi.new("uint32_t [1]")
        if lib.sddc_open(self.phandle,index):
            raise Exception("Failed to open dev %d"%(index))
        if lib.sddc_set_xtal_freq(self.phandle[0], sample_rate):
            raise Exception("Failed to set sample rate %d"%(ret_code))
        if lib.sddc_set_direct_sampling(self.phandle[0], True):
            raise Exception("Failed to set direct sampling")
        if lib.sddc_set_center_freq64(self.phandle[0], 7100000):
            raise Exception("Failed to set center frequency")
        if lib.sddc_set_if_gain(self.phandle[0], 10.0):
            raise Exception("Failed to set if gain")
        if lib.sddc_set_rf_gain(self.phandle[0], 30.0):
            raise Exception("Failed to set rf gain")
        if lib.sddc_enable_bias_tee(self.phandle[0], True):
            raise Exception("Failed to enable bias")
        #ring_buffers[index] = RingBuffer(nbuf, bufsz)
        lib.sddc_get_xtal_freq(self.phandle[0], uival)
        print("Sample rate:", uival[0])
        print("Center freq:", lib.sddc_get_center_freq64(self.phandle[0]))
        lib.sddc_get_rf_gain(self.phandle[0],fval)
        print("RF gain:", fval[0])
        lib.sddc_get_if_gain(self.phandle[0],fval)
        print("IF gain:", fval[0])

    def start_async(self, callback=ringbuffer.ring_buffer.get_Rx888Callback()):
        #if lib.sddc_read_async(self.phandle[0], callback, ffi.cast("void *", self.index)):
        if lib.sddc_read_async(self.phandle[0], ffi.cast("void(*)(const int16_t *, uint32_t, void*)", callback), ffi.cast("void *", self.rb.address())):
            raise Exception("Failed to set callback")

    def set_if_gain(self, gain):
        lib.sddc_set_if_gain(self.phandle[0], gain)

    def cancel_async(self):
        lib.sddc_cancel_async(self.phandle[0])

    def __delete__(self):
        lib.sddc_close(self.phandle[0])
        del ring_buffers[self.index]

    def read(self, n=1):
        return self.rb.read(n)

    #def read_peek_size(self):
    #    return ring_buffers[self.index].read()
