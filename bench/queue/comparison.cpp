#include <chrono>
#include <iostream>
#include <thread>

#include <boost/lockfree/spsc_queue.hpp>
// #include <folly/ProducerConsumerQueue.h>



#define NUM_ITEMS 1000000

int iters = NUM_ITEMS;

void boost_consumer(boost::lockfree::spsc_queue<int> q){
     for (int i = 0; i < iters; ++i) {
        int val;
        while (q.pop(&val, 1) != 1)
          ;
        if (val != i) {
          throw std::runtime_error("");
        }
      }
}


void boost_lockfree_spes_queue(){

    std::cout << "boost::lockfree::spsc:" << std::endl;
  
    boost::lockfree::spsc_queue<int> q(1000);

    std::thread t(boost_consumer, q);

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iters; ++i) {
      while (!q.push(i))
        ;
    }
    t.join();
    auto stop = std::chrono::steady_clock::now();
    std::cout << iters * 1000000 /
                     std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                          start)
                         .count()
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
