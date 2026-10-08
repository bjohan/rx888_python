#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <cstring>
#include <vector>
#include <cstdint>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <iostream>

namespace py = pybind11;


class RingBuffer{
	public:
		RingBuffer(size_t nBuf, size_t bufSz);
		void write(const int16_t *buf, size_t sz);
		int16_t* get_read(size_t *size);
		bool read_next();
		py::array_t<int16_t> read(size_t nBuf);

	private:
		std::vector<std::vector<int16_t>> _bufs;
		std::vector<size_t> _bufFill;
		std::atomic<size_t> _currentReadBuf;
		std::atomic<size_t> _currentWriteBuf;
		size_t _bufSz;
		size_t _numBuf;
		std::condition_variable _writeEvent;
		std::mutex _writeEventMutex;
};

void Rx888Callback(const int16_t *buf, uint32_t sz, void *arg){
	//std::cout << "data!"<< sz << std::endl;
	RingBuffer *rb = (RingBuffer*) arg;
	rb->write(buf, sz);
}



RingBuffer::RingBuffer(size_t numBuf, size_t bufSz){
	_numBuf = numBuf;
	_bufSz = bufSz;
	std::vector<int16_t> init;
	init.resize(bufSz, 0);
	_bufFill.resize(bufSz, 0);
	_bufs.resize(numBuf, init);
	_currentReadBuf = 0;
	_currentWriteBuf = 0;
	std::cout << "Created ring buffer at " << this << std::endl;
	std::cout << "Created ring buffer at 0x" << std::hex << reinterpret_cast<std::uintptr_t>(this)<< std::dec<< std::endl;
	std::cout << "Callback at " << Rx888Callback << std::endl;
	std::cout << "Size of each buffer: " << _bufSz << " number of buffers: " << _numBuf << std::endl;
}

void RingBuffer::write(const int16_t *buf, size_t sz){
	size_t free = _bufSz-_bufFill[_currentWriteBuf];
       	if (free < sz){
	      	size_t nextBuf = (_currentWriteBuf+1)%_numBuf;
	       	if(nextBuf == _currentReadBuf){
			std::cout << "O";
		     	return;
		}
		std::cout << "new buf " << _bufSz << " " << _bufFill[_currentWriteBuf] << " " << _currentWriteBuf << std::endl;
	       	_currentWriteBuf = nextBuf;
		_writeEvent.notify_one();
 	}
	std::size_t pos = _bufFill[_currentWriteBuf];
	std::memcpy(&_bufs[_currentWriteBuf].data()[pos], buf, sz*sizeof(int16_t));
	_bufFill[_currentWriteBuf]+=sz;
	//std::cout << "Got: " << sz << " " << buf[0] <<std::endl;
	       
}

int16_t *RingBuffer::get_read(size_t *size){
	if(not size) return NULL;
	//if(_currentReadBuf == _currentWriteBuf) return NULL;
	if(_currentReadBuf == _currentWriteBuf){
		std::unique_lock lk(_writeEventMutex);
		std::cout << "Waiting" << std::endl;
		_writeEvent.wait(lk);
		//return get_read(size);
	}
	*size=_bufFill[_currentReadBuf];
	return _bufs[_currentReadBuf].data();
}

bool RingBuffer::read_next(){
	//if(_currentReadBuf == _currentWriteBuf) return false;
	if(_currentReadBuf == _currentWriteBuf){
		std::cout << "Waiting for buffer " << _currentReadBuf << " " << _currentWriteBuf << std::endl;
		std::unique_lock lk(_writeEventMutex);
		_writeEvent.wait(lk);
	}
	_bufFill[_currentReadBuf]=0;
	_currentReadBuf = (_currentReadBuf+1)%_numBuf;
	return true;
}

py::array_t<int16_t> RingBuffer::read(size_t nBuf){
	py::array_t<int16_t> ret(nBuf*_bufSz);
	auto r = ret.mutable_unchecked<1>();
	//for(size_t i = 0 ; i < ret.shape(0) ; i++){
	//	r(i)=i;
	//}
	std::cout << "####################3shape: " << ret.shape(0)  << std::endl;
	//auto r = ret.mutable_unchecked<1>();
	int16_t *pd = ret.mutable_data();
	size_t pos = 0;
	//std::cout << "Enter loop" << std::endl;
	while(nBuf--){
		size_t sz;
		//std::cout << "Getting buffer" << std::endl;
		int16_t *buf = get_read(&sz);
		//std::cout << "Reading: " << sz << " from " << _currentReadBuf << std::endl;
		/*for(size_t i = 0 ; i < sz ; i++){
			int16_t v = buf[i];
			//if(i < 10) std::cout << v << " " << pos <<" " << i << std::endl;
			////*ret.mutable_data(pos++)=v;
			//pd[pos++]=i%100;
			//r(pos)=i%100;
			//pos++;
			//int16_t *p;
			//p = (int16_t*)r.data();
			pd[pos++]=v;
		}*/
		memcpy(&pd[pos], buf, sz*sizeof(int16_t));
		pos+=sz;
		//size_t rb = _currentReadBuf;
		//std::cout << "Reading " << _bufFill[rb] << " samples from " << rb << std::endl;
		//for(size_t i = 0 ; i < _bufFill[rb] ; i++)
		//	r(pos++)=3;//_bufs[rb][i];
		read_next();
	}
	//auto b = ret.unchecked();
	//for(size_t i = 0 ; i < 10 ; i++)
	//	std::cout << "v: " << ret.data()[i] << " " <<  b(i) << " " << r(i) <<std::endl;
	return ret;
	//
/*
    auto out = py::array_t<int16_t>(500);
    auto o   = out.mutable_unchecked<1>();
 
    ssize_t N = out.shape(0);
    //int radius = static_cast<int>(3 * sigma);
 
    for (ssize_t i = 0; i < N; i++) {
        o(i) = i;//sum / weight;
    }
    return out;*/

}

PYBIND11_MODULE(ring_buffer, m) {
    /*py::class_<Processor, PyProcessor, std::shared_ptr<Processor>>(m, "Processor")
        .def(py::init<>())
        .def("process", &Processor::process)
        .def("name",    &Processor::name)
        .def("run",     &Processor::run);*/
 
    py::class_<RingBuffer>(m, "RingBuffer")
        .def(py::init<size_t, size_t>())
        .def("write",          	&RingBuffer::write)
        .def("get_read", 	&RingBuffer::get_read)
        .def("read_next",      	&RingBuffer::read_next)
	.def("read", 		&RingBuffer::read, py::arg("nBuf"))
	.def("address", [](RingBuffer& p) {
        return reinterpret_cast<std::uintptr_t>(&p);});

    m.def("Rx888Callback", &Rx888Callback);
    m.def("get_Rx888Callback", []() {
    return reinterpret_cast<std::uintptr_t>(&Rx888Callback); });
}

