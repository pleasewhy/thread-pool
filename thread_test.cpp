#include "include/thread_pool.hpp"
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <thread>
#include <unistd.h>

using namespace std;

int add(int a, int b) { return a + b; }

void print() { cout << "test function" << endl; }

class Test {
public:
  void test() { cout << "test methods" << endl; }
};

int main() {
  ThreadPool pool(5);
  for (int i = 0; i < 10; i++) {
    try {
      auto fu = pool.Execute(add, 1, 1);
      fu.get();
    } catch (exception &e) {
      cout << e.what() << endl;
    }
  }
  pool.Shutdown();
  return 0;
}