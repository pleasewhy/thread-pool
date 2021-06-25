#ifndef RUN_TIMER_HPP
#define RUN_TIMER_HPP
#include <stdio.h>
#include <sys/time.h>

class RunTimer {
private:
  timeval start_;
  timeval end_;
  bool started_;

public:
  void start() {
    started_ = true;
    gettimeofday(&start_, 0);
  }

  void end() {
    if (!started_) {
      return;
    }
    started_ = false;
    gettimeofday(&end_, 0);
    int s = start_.tv_sec * 1000 + start_.tv_usec / 1000;
    int e = end_.tv_sec * 1000 + end_.tv_usec / 1000;
    printf("run time:%dms\n", e-s);
  }

  int ElapseMs() {
    started_ = false;
    gettimeofday(&end_, 0);
    return (end_.tv_sec - start_.tv_sec) * 1000 +
           (end_.tv_usec - start_.tv_usec) / 1000;
  }

  RunTimer() = default;
  ~RunTimer() = default;
};
#endif