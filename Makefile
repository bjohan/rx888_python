venv: venv/touchfile

venv/touchfile: requirements.txt
	python3 -m venv venv
	./venv/bin/pip install -Ur requirements.txt
	touch venv/touchfile

test: venv
	./venv/bin/activate nosetests project/test

rx888:
	git clone https://github.com/RaspSDR/rx888.git

rx888/target/release/libsddc.so: rx888
	cd rx888; cargo build --release

ringbuffer/ring_buffer.cpython-312-aarch64-linux-gnu.so: ringbuffer/ring_buffer.cpp ringbuffer/setup.py
	cd ringbuffer; ../venv/bin/python3 setup.py build_ext --inplace

.phony: deps firmware run depclean
deps: venv rx888/target/release/libsddc.so ringbuffer/ring_buffer.cpython-312-aarch64-linux-gnu.so

run: deps 
	./venv/bin/python3 main.py

firmware: rx888
	timeout 2 ./rx888/target/release/usb_throughput_test || true
	lsusb | grep -q 04b4:3ddc

clean:
	find -iname "*.pyc" -delete
	rm ring_buffer.o
	rm ring_buffer.so

depclean:
	rm -rf venv
	rm -rf rx888
