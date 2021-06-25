#include "include/process_pool.hpp"
#include "include/run_timer.hpp"
#include "include/thread_pool.hpp"
#include <cstdlib>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <sys/time.h>
#include <thread>
#include <unistd.h>

using namespace std;

bool IsPrime(int n) {
  if (n <= 3) {
    return n > 2;
  }
  for (int i = 2; i < n; i++) {
    if (n % i == 0) {
      return false;
    }
  }
  return true;
}

/**
 * Cpu密集测试：判断一个随机值，是否是质数
 * 一共有3个测试函数: 它们的随机值范围不同
 */

// 0-1000000
void CpuIntensiveTest1() {
  int x = rand() % 1000000;
  if (IsPrime(x)) {
    fprintf(stderr, "%d is prime number\n", x);
  }
};

// 5000000-6000000
void CpuIntensiveTest2() {
  int x = (rand() % 1000000) + 5000000;
  if (IsPrime(x)) {
    fprintf(stderr, "%d is prime number\n", x);
  }
};

// 10000000-11000000
void CpuIntensiveTest3() {
  int x = (rand() % 1000000) + 10000000;
  if (IsPrime(x)) {
    fprintf(stderr, "%d is prime number\n", x);
  }
};
// 4719
// 2361
// 1585 
// 1588 波动大
// 1200
// 1190
// 1200
// 1200
void TestThreadPool() {
  printf("#########Test Thread Pool#########\n");
  ThreadPool pool(8);
  RunTimer timer;
  timer.start();
  for (int i = 0; i < 1000; i++) {
    pool.Execute(CpuIntensiveTest3);
    pool.Execute(CpuIntensiveTest1);
    pool.Execute(CpuIntensiveTest2);
  }
  auto fu = pool.Execute(CpuIntensiveTest1);
  fu.get();
  pool.Shutdown();
  timer.end();
  printf("#########Test Thread Pool#########\n");
}

void OrdinaryTest() {
  printf("#########Ordinary Test#########\n");
  RunTimer timer;
  timer.start();
  for (int i = 0; i < 1000; i++) {
    CpuIntensiveTest3();
    CpuIntensiveTest1();
    CpuIntensiveTest2();
  }
  timer.end();
  printf("#########Ordinary Test#########\n");
}

void TestProcessPool() {
  ProcessPool pool(8);
  pool.RegisterHander(1, CpuIntensiveTest1);
  pool.RegisterHander(2, CpuIntensiveTest2);
  pool.RegisterHander(3, CpuIntensiveTest3);
  RunTimer timer;
  timer.start();
  for (int i = 0; i < 1000; i++) {
    pool.Execute(3);
    pool.Execute(1);
    pool.Execute(2);
  }
  pool.Shutdown();
  timer.end();
}

int main() {
  TestThreadPool();
  printf("\n");
  // TestProcessPool();
  // printf("\n");
  // OrdinaryTest();
  // srand((unsigned)time(NULL));
  // cout << RAND_MAX << endl;
  return 0;
}