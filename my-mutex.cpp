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

class Petersons : public Mutex
{
    std::atomic_bool flag[2];
    std::atomic_int turn;
    int pidA;
    int pidB;

    // Takes two thread ids that can use the mutex
    Petersons(int A, int B)
    {
        pidA = A;
        pidB = B;
    }

    void Lock()
    {
        pthread_t threadId = pthread_self();
        if(threadId != pidA || threadId != pidB) throw std::exception();
        int id = (pthread_self() == pidA) ? 0 : 1;

        flag[id] = true;
        turn = 1 - id;
        while(flag[1-id] && turn != id);
    }

    void Unlock()
    {
        pthread_t threadId = pthread_self();
        if(threadId != pidA || threadId != pidB) throw std::exception();
        int id = (pthread_self() == pidA) ? 0 : 1;
        flag[id] = false;
    }
};

class TestAndSet : public Mutex
{
    std::atomic_flag flag;

    void Lock() { while(flag.test_and_set()); }
    void Unlock() { flag.clear(); }
};

class FetchAndIncrement : public Mutex
{
    std::atomic_ullong token;
    std::atomic_ullong turn;

    void Lock()
    {
        int ticket = token++;
        while(ticket != turn);
    }

    void Unlock() { ++turn; }
};

struct argPasser
{
    Mutex* mutex;
    int* sharedCounter;
};

const int TESTVALUE = 100;

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

    cout << "Counter finished with " << *(args.sharedCounter) << ". Expected " << threadCt * TESTVALUE << std::endl;
}

// Do thread stuff in here
void *threadFunction(void* arg)
{
    argPasser* args = reinterpret_cast<argPasser*>(arg);

    for(int i=0; i<TESTVALUE; i++)
    {
        args->mutex->Lock();
        *(args->sharedCounter) += 1;
        args->mutex->Unlock();
    }
    
    pthread_exit(NULL);
}
