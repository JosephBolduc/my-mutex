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
    int threadCount;


    public:
    Tournament(int threadCt)
    {
        threadCount = threadCt;
    }

    void Lock()
    {

    }

    void Unlock()
    {

    }
};

class TestAndSet : public Mutex
{
    std::atomic_flag flag;

    public:
    void Lock()
    {
        std::string msg = "Locking from thread" + std::to_string(pthread_self()) +"\n";
        cout << msg;
        while(flag.test_and_set());
    }

    void Unlock()
    {
        std::string msg = "Unlocking from thread" + std::to_string(pthread_self()) + "\n";
        cout << msg;
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

struct argPasser
{
    Mutex* mutex;
    int* sharedCounter;
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


    Mutex* mutex;
    switch (algoMode)
    {
    case 0:
        mutex = new Tournament(threadCt);
        break;
    case 1:
        mutex = new TestAndSet();
        break;
    case 2:
        mutex = new FetchAndIncrement();
        break;
    default:
        return -1;
    }

    int sharedCounter = 0;
    argPasser args = {mutex, &sharedCounter};

    for (size_t i = 0; i < threadCt; i++)
    {
        pthread_t newThread;
        pthread_create(&newThread, NULL, threadFunction, &args);
        threads.push_back(newThread);
    }

    for(auto thread : threads) pthread_join(thread, NULL);
}

// Do thread stuff in here
void *threadFunction(void* arg)
{
    argPasser* args = reinterpret_cast<argPasser*>(arg);

    args->mutex->Lock();
    pthread_t self = pthread_self();
    cout << "Current thread is " << self << "\n";
    args->mutex->Unlock();

    pthread_exit(NULL);
}
