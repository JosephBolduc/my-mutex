#include <iostream>
#include <string>
#include <vector>
#include <atomic>
#include <pthread.h>

using std::cout;
using std::vector;

void *threadFunction(void*);

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

    for (size_t i = 0; i < threadCt; i++)
    {
        pthread_t newThread;
        pthread_create(&newThread, NULL, threadFunction, NULL);
    }
}

// Do thread stuff in here
void *threadFunction(void* arg)
{
    pthread_exit(NULL);
}

class Mutex
{
    public:
    virtual void Lock() = 0;
    virtual void Unlock() = 0;
};

class TestAndSet : Mutex
{

};

class FetchAndSet : Mutex
{

};