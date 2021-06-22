.PHONY: clean
T = target

# -pthread 通过使用POSIX线程库函数来添加多线程支持
# 它会设置linker和preprocessor来使得c++可以使用多线程
# 该选项不会影响程序的线程安全
test: thread_test.cpp
	g++ -std=c++11 thread_test.cpp  -o thread_test -pthread
	./thread_test
clean:
	rm target/**