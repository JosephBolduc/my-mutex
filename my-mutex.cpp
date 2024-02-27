#include <iostream>
#include <string>
#include <vector>
#include <atomic>
#include <pthread.h>

using std::cout;
using std::vector;

void *threadFunction(void*);

class Mutex
{
    public:
    virtual void Lock() = 0;
    virtual void Unlock() = 0;
};

class Tournament : public Mutex
{

};

class TestAndSet : public Mutex
{
    std::atomic_flag flag;

    public:
    void Lock()
    {
        while(flag.test_and_set());
    }

    void Unlock()
    {
        flag.clear();
    }

};

class FetchAndIncrement : public Mutex
{
    std::atomic_int counter;

    void Lock()
    {
        int ticket = counter++;
        std::string msg = "Locking from thread" + std::to_string(pthread_self()) + "with ticket = " + std::to_string(ticket) + "\n";
        cout << msg;
        while(counter == ticket);
    }

    void Unlock()
    {
        std::string msg = "UnLocking from thread" + std::to_string(pthread_self()) + "\n";
        cout << msg;
        ++counter;
    }

};

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cout << "Enter two arguments: Algorithm type (0 for TT, 1 for TAS, 2 for FAI) and thread count!\n";
        return -1;
    }

    const int algoMode = std::stoi(argv[1]);
    const int threadCt = std::stoi(argv[2]);
    vector<pthread_t> threads;

    if (algoMode < 0 || algoMode > 2)
    {
        cout << "Enter algorithm type 0 for TT, 1 for TAS, 2 for FAI!\n";
        return -1;
    }

    if (threadCt < 1)
    {
        cout << "Use at least one thread!\n";
        return -1;
    }

    Mutex* mutex = new FetchAndIncrement();

    for (size_t i = 0; i < threadCt; i++)
    {
        pthread_t newThread;
        pthread_create(&newThread, NULL, threadFunction, mutex);
        threads.push_back(newThread);
    }

    for(auto thread : threads) pthread_join(thread, NULL);
}

// Do thread stuff in here
void *threadFunction(void* arg)
{
    Mutex* mutex = reinterpret_cast<Mutex*>(arg);

    mutex->Lock();
    pthread_t self = pthread_self();
    cout << "Current thread is " << self << "\n";
    mutex->Unlock();

    pthread_exit(NULL);
}
