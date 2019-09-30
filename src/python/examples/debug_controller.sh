LD_PRELOAD=/lib64/libpython2.7.so.1.0 gdb -ex 'set environ LD_PRELOAD' --args `which python` ./controller.py
