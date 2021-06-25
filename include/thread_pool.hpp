#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP
#include "run_timer.hpp"
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

using namespace std;
using namespace std::placeholders;

typedef std::function<void()> Runnable;

class ThreadPool;

struct TaskException : public exception {
  const char *what() const throw() { return "任务队列已满，添加失败!"; }
};

class Worker {
  friend ThreadPool;

public:
  Runnable first_task;
  int worker_id;
  int completed_task;
  ThreadPool *thread_pool_;
  thread *my_thread;
  // 在后面实现
  Worker(ThreadPool *pool, Runnable first_task);
  ~Worker();
  void run(); // 在后面实现
};

class ThreadPool {
  friend Worker;

public:
  ThreadPool(int worker_num) { this->core_pool_size_ = worker_num; }

  /**
   * @brief 在未来的某个时间执行给定的任务。该任务可以
   * 在一个新的线程或者线程池中的某个线程中执行。
   *
   * @param fn 函数指针
   * @param args %fn的的参数
   * @note 可以通过
   *
   * @return 返回一个future以便调用者获取任务的返回值
   *
   */
  template <class F, class... Args>
  auto Execute(F &&fn, Args &&... args) -> std::future<decltype(fn(args...))> {
    using RetType = decltype(fn(args...));
    auto task = make_shared<packaged_task<RetType()>>(
        bind(forward<F>(fn), forward<Args>(args)...)); // 将fn和args绑定
    auto future = task->get_future();

    // 将task包装成无返回值无参数的函数
    Runnable wraped_task = [task]() { (*task)(); };

    /**
     * 如果现在的worker数量小于core_pool_size_，就添加一个thread
     * 来执行当前的任务，并将该task作为它的first_task。注意AddWorker
     * 函数会保证再次判断，以保证不会错误添加worker。
     */
    if (worker_num_ < core_pool_size_) {
      if (AddWorker(wraped_task)) { //添加成功直接返回
        return future;
      }
    }
    /**
     * 将task添加到任务队列
     */
    if (AddTask(wraped_task))
      return future;
    throw TaskException();
  }

  bool AddTask(Runnable task) {
    lock_guard<mutex> lock(tasks_lock_);
    task_queue_.push(task);
    cv_.notify_one();
    return true;
  }

  Runnable GetTask() {
    Runnable task;
    {
      unique_lock<mutex> lock(tasks_lock_);
      cv_.wait(lock,
               [this] { return this->stop || !this->task_queue_.empty(); });
      if (stop) {
        return nullptr;
      }
      task = task_queue_.front();
      task_queue_.pop();
    }
    return task;
  }

  void RunWorker(Worker *worker) {
    Runnable task = worker->first_task;
    RunTimer timer;
    int run_task_time = 0;
    while (task != nullptr || (task = GetTask()) != nullptr) {
      timer.start();
      task();
      run_task_time += timer.ElapseMs();
      worker->completed_task++;
      task = nullptr;
    }
    printf("worker=%d, complete tasks=%d run task time=%dms\n",
           worker->worker_id, worker->completed_task,run_task_time);
  }

  /**
   * @brief 添加worker
   *
   * @param first_task  woker执行的第一个任务
   */
  bool AddWorker(Runnable first_task) {
    { // 用于锁的RAII
      lock_guard<mutex> lock(workers_lock_);
      if (worker_num_ >= core_pool_size_) { // 防止虚假添加
        return false;
      }
      Worker *w = new Worker(this, first_task);
      workers_.push_back(unique_ptr<Worker>(w));
      w->run();
      worker_num_++;
    }
    return true;
  }

  void Shutdown() {
    stop = true;
    cv_.notify_all();
    for(int i=0;i<worker_num_;i++){
      workers_[i]->my_thread->join();
    }
  }

private:
  mutex tasks_lock_;
  mutex workers_lock_;
  condition_variable cv_;
  vector<unique_ptr<Worker>> workers_;
  queue<Runnable> task_queue_;
  int core_pool_size_;
  atomic<int> worker_num_{0};
  atomic<bool> stop{false};
};

Worker::Worker(ThreadPool *pool, Runnable first_task)
    : first_task(first_task), thread_pool_(pool) {
  worker_id = pool->worker_num_.load();
}

void Worker::run() {
  my_thread =
      new thread(bind(&ThreadPool::RunWorker, _1, _2), thread_pool_, this);
}

Worker::~Worker() { }
#endif