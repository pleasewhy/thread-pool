#ifndef PROCESS_POOL_HPP
#define PROCESS_POOL_HPP
#include <cstring>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace std;

typedef std::function<void()> Handler;
typedef int HandlerId;

const uint16_t kCountOfHandler = 5;
const HandlerId kProcessExit = -1;

class ProcessPool;

class WorkerProcess {
  friend ProcessPool;
  int worker_id_;
  int pid_;             // 进程id
  int parent2child_[2]; // 0读，1写
  Handler *handlers_ptr_;

  WorkerProcess(int worker_id_) : worker_id_(worker_id_) {}
  /**
   * @brief 执行任务
   *
   * @param handler_id
   */
  void DoWork(HandlerId handler_id);

  /**
   * @brief 等待父进程发送任务
   */
  void WaitService();

  /**
   * @brief 运行该worker，这会创建一个进程
   * 并运行WaitService函数等待父进程发送任务。
   * 父进程可以使用this->parent2child_管道向
   * 子进程发送hander_id，来表明子进程需要执行
   * 那个函数。
   *
   */
  void Run();
};

class ProcessPool {
  friend WorkerProcess;

public:
  ProcessPool(int core_pool_size) : core_pool_size_(core_pool_size) {
    memset(handlers, 0, sizeof(handlers));
  }
  ~ProcessPool() = default;
  /**
   * @brief 注册hander,
   * @note 由于需要跨进程通信，所以不能执行任何
   * 类型的任务,只能执行预先定义任务，然后通过
   * %num运行，对应的任务
   *
   * @param num handler对应num
   * @param handler void()类型函数
   */
  void RegisterHander(HandlerId id, Handler handler) {
    ProcessPool::handlers[id] = handler;
  }

  /**
   * @brief 提交给进程池一个任务，这个任务立即执行
   * 或者在未来的某个时间执行。注意，这个函数会立即
   * 返回，并且不能获取到返回值。
   *
   * @param num 需要执行的任务的handler号
   */
  bool Execute(HandlerId id) {
    if (worker_num_ < core_pool_size_) {
      AddWorker(id);
      return true;
    }
    lock_guard<mutex> lock(workers_lock_);
    for (; select_cnt_ < worker_num_; select_cnt_++) {
      if (workers_[select_cnt_] == nullptr) {
        continue;
      }
      if (Send2Child(select_cnt_, &id, sizeof(id)) != sizeof(id)) {
        printf("error");
        return false;
      }
    }
    return true;
  }

  bool AddWorker(HandlerId id) {
    { // 用于锁的RAII
      lock_guard<mutex> lock(workers_lock_);
      if (worker_num_ >= core_pool_size_) { // 防止虚假添加
        return false;
      }
      WorkerProcess *w = new WorkerProcess(worker_num_);
      workers_.push_back(unique_ptr<WorkerProcess>(w));
      w->Run();
      Send2Child(worker_num_, &id, sizeof(id));
      worker_num_++;
    }
    return true;
  }

  void Shutdown() {
    printf("shut down\n");
    for (int i = 0; i < worker_num_; i++) {
      Send2Child(i, &kProcessExit, sizeof(kProcessExit));
      CloseWriteEnd(i);
      wait(0);
    }
  }

  int Send2Child(int idx, const void *buf, int n) {
    return write(workers_[idx]->parent2child_[1], buf, n);
  }

  void CloseWriteEnd(int idx) { close(workers_[idx]->parent2child_[1]); }

public:
  static Handler handlers[5];

private:
  int core_pool_size_;
  vector<unique_ptr<WorkerProcess>> workers_;
  mutex workers_lock_;
  int worker_num_ = 0;
  atomic<uint16_t> select_cnt_{0}; // 用于平均分配任务
};

Handler ProcessPool::handlers[5];

void WorkerProcess::DoWork(HandlerId hander_id) {
  if (ProcessPool::handlers[hander_id] == nullptr) {
    return;
  }
  (ProcessPool::handlers[hander_id])();
}

void WorkerProcess::WaitService() {
  HandlerId handler_id;
  while (read(parent2child_[0], &handler_id, sizeof(handler_id)) ==
         sizeof(handler_id)) {
    if (handler_id == kProcessExit) {
      printf("worker id=%d exit\n", worker_id_);
      exit(0);
    } else {
      DoWork(handler_id);
    }
  }
  exit(-1);
}

void WorkerProcess::Run() {
  pipe(parent2child_);
  int pid = fork();
  if (pid == 0) {
    close(parent2child_[1]); //关闭子进程的写端
    WaitService();
  }
  close(parent2child_[0]); // 关闭父进程的读端
}

#endif