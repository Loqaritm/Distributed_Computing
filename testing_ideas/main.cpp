#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

std::mutex can_i_go_mutex;
std::condition_variable cv;
bool ready = false;
std::vector<int> queues[3];

void threadFun1(){

    for(int i = 0; i < 3; ++i){
        {
            std::lock_guard<std::mutex> lk(can_i_go_mutex);
            ready = true;
            std::cout<<"threadFun1 working"<<std::endl;
            queues[0].push_back(i);
        }
        cv.notify_one();
        std::cout<<"dupa"<<std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}

int main(){
    std::thread t1(threadFun1);
    
    std::unique_lock<std::mutex> lk(can_i_go_mutex, std::defer_lock);
    for (int i = 0; i < 3; ++i){
        lk.lock();
        cv.wait(lk, []{return ready;});
        ready = false;
        std::cout<<"odebrałem"<<std::endl;
        for (auto i : queues[0]){
            std::cout<<"wartosc queue "<<i<<std::endl;
        }
        lk.unlock();
    }
    t1.join();
}