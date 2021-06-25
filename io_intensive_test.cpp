#include "include/process_pool.hpp"
#include "include/run_timer.hpp"
#include "include/thread_pool.hpp"
#include <cstdlib>
#include <fcntl.h>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <sys/time.h>
#include <thread>
#include <unistd.h>

/**
 * IO密集测试：写文件n次
 */
void CreateAndWrite(int n) {
  string prefix = "./target/test/";
  string suffix = to_string(getpid() + n);
  string path = prefix + suffix+"_test";
  // cout << path<<endl;
  int fd = open(path.c_str(), O_CREAT | O_RDWR);
  for (int i = 0; i < n; i++) {
    write(fd, "abcd\n", 4);
  }
  close(fd);
}

// 0-1000
void IoIntensiveTest1() {
  int x = rand() % 1000;
  CreateAndWrite(x);
};

// 5000-6000
void IoIntensiveTest2() {
  int x = (rand() % 1000) + 5000;
  CreateAndWrite(x);
};

// 10000-11000
void IoIntensiveTest3() {
  int x = (rand() % 1000) + 10000;
  CreateAndWrite(x);
};

// 738 747
// 664 716
// 608 685
// 565 601
// 534 565



#define ThreadNumber  15
void TestThreadPool() {
  printf("#########Test Thread Pool#########\n");
  ThreadPool pool(ThreadNumber);
  RunTimer timer;
  timer.start();
  for (int i = 0; i < 100; i++) {
    pool.Execute(IoIntensiveTest3);
    pool.Execute(IoIntensiveTest1);
    pool.Execute(IoIntensiveTest2);
  }
  auto fu = pool.Execute(IoIntensiveTest1);
  fu.get();
  pool.Shutdown();
  timer.end();
  printf("#########Test Thread Pool#########\n");
}

void TestProcessPool() {
  printf("#########Test Process Pool#########\n");
  ProcessPool pool(ThreadNumber);
  pool.RegisterHander(1, IoIntensiveTest1);
  pool.RegisterHander(2, IoIntensiveTest2);
  pool.RegisterHander(3, IoIntensiveTest3);
  RunTimer timer;
  timer.start();
  for (int i = 0; i < 100; i++) {
    pool.Execute(3);
    pool.Execute(1);
    pool.Execute(2);
  }
  pool.Shutdown();
  timer.end();
  printf("#########Test Process Pool#########\n");
}

void OrdinaryTest() {
  printf("#########Ordinary Test#########\n");
  RunTimer timer;
  timer.start();
  for (int i = 0; i < 100; i++) {
    IoIntensiveTest3();
    IoIntensiveTest1();
    IoIntensiveTest2();
  }
  timer.end();
  printf("#########Ordinary Test#########\n");
}

int main() {
  printf("\n");
  TestProcessPool();
  printf("\n");
  TestThreadPool();
  // printf("\n");
  // OrdinaryTest();
  return 0;
}
