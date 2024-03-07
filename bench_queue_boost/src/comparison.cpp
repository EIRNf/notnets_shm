#include <chrono>
#include <iostream>
#include <thread>

#include <boost/lockfree/spsc_queue.hpp>
// #include <folly/ProducerConsumerQueue.h>


#define MESSAGE_SIZE 4 //Int
#define EXECUTION_SEC 20

int cpu0 = 0;
int cpu1 = 1;

void pinThread(int cpu) {
#if !defined(__APPLE__)
  if (cpu < 0) {
    return;
  }
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);
  if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) ==
      -1) {
    perror("pthread_setaffinity_no");
    exit(1);
  }
#endif
}

void boost_lockfree_spes_queue(){

    std::cout << "batch-boost::lockfree::spsc:" << std::endl;
  
    boost::lockfree::spsc_queue<u_int> q(16024);

    std::atomic<bool> stop_flag(true);

    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point stop;
    int items_consumed = 0;

    auto t = std::thread([&] {
      pinThread(cpu1);
      u_int i = 0;
      start = std::chrono::steady_clock::now();
      while(!stop_flag.load()){
        u_int val;
        while (q.pop(&val, 1) != 1)
          ;
        if (val != i) {
          throw std::runtime_error("");
        }
        i++;
      }
      stop = std::chrono::steady_clock::now();
      items_consumed = i;
    });

    auto t2 = std::thread([&] {
      pinThread(cpu0);
      u_int i = 0;
      while(!stop_flag.load()){
        while (!q.push(i))
          ;
        i++;
      }
      t.join();
    });
    
    stop_flag.store(false);
    sleep(EXECUTION_SEC);
    stop_flag.store(true);
    t2.join();


    auto difference = stop - start;
    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::seconds>(difference).count() << "sec, ItemsConsumed: " << items_consumed << std::endl;
    std::cout <<  std::chrono::duration_cast<std::chrono::nanoseconds>(difference).count() /items_consumed  << " ns/op" << std::endl;
    std::cout << items_consumed /
                     std::chrono::duration_cast<std::chrono::milliseconds>(difference).count()
              << " ops/ms" << std::endl;

}

// void folly_spsc(){

//     std::cout << "folly::ProducerConsumerQueue:" << std::endl;

//     folly::ProducerConsumerQueue<int> q(queueSize);
//     auto t = std::thread([&] {
//       pinThread(cpu1);
//       for (int i = 0; i < iters; ++i) {
//         int val;
//         while (!q.read(val))
//           ;
//         if (val != i) {
//           throw std::runtime_error("");
//         }
//       }
//     });

//     pinThread(cpu2);

//     auto start = std::chrono::steady_clock::now();
//     for (int i = 0; i < iters; ++i) {
//       while (!q.write(i))
//         ;
//     }
//     t.join();
//     auto stop = std::chrono::steady_clock::now();
//     std::cout << iters * 1000000 /
//                      std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
//                                                                           start)
//                          .count()
//               << " ops/ms" << std::endl;
    
// }

int main(){
    // folly_spsc();

    boost_lockfree_spes_queue();
    return 0;
}
