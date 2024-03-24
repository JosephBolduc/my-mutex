#include <iostream>
#include <string>
#include <vector>
#include <math.h>
#include <atomic>
#include <pthread.h>
#include <stdexcept>

using std::cout;
using std::vector;

void *threadFunction(void*);

class Mutex
{
    public:
    virtual void Lock() = 0;
    virtual void Unlock() = 0;
};

class Petersons
{
    std::atomic_bool flag[2];
    std::atomic_int turn;
public:
    void Lock(bool isLeft)
    {
        int id = isLeft ? 0 : 1;
        flag[id] = true;
        turn = 1 - id;
        while(flag[1-id] && turn != id);
    }

    void Unlock(bool isLeft)
    {
        int id = isLeft ? 0 : 1;
        flag[id] = false;
    }
};

class Tournament : public Mutex
{
    vector<Petersons*> mutexList;
    pthread_t* threadList;

    int threadCount;

    public:
    Tournament(pthread_t* threads, int threadCt)
    {
        threadCount = threadCt;
        threadList = threads;
        for(int i=0; i<threadCount; i++) mutexList.push_back(new Petersons());
    }

    int getThreadID()
    {
        pthread_t thread = pthread_self();
        while(true) for(int idx = 0; idx < threadCount; idx++) if (thread == threadList[idx]) return idx;

    }

    void Lock()
    {
        int threadID = getThreadID();
        int currentLock = threadCount - threadID + 1;
        int prevLock = 9999;
        while(true)
        {
            bool lockFromLeft = getLockDirection(prevLock, currentLock);
            mutexList.at(currentLock)->Lock(lockFromLeft);
            if (currentLock == 0 ) break;
            prevLock = currentLock;
            currentLock = getParentNodeIdx(currentLock);
        }
    }


    void Unlock()
    {
        int threadID = getThreadID();
        int currentLock = threadCount - threadID + 1;
        vector<int> lockList;
        while(true)
        {
            if(currentLock == 0) 
            {
                lockList.push_back(0);
                break;
            }
            lockList.push_back(currentLock);
            currentLock = getParentNodeIdx(currentLock);
        }

        for(int idx = lockList.size() - 1; idx >= 0; idx--)
        {
            bool unlockFromLeft;
            if(idx > 0) unlockFromLeft = getLockDirection(lockList[idx-1], lockList[idx]);
            else unlockFromLeft = getLockDirection(9999, lockList[0]);

            mutexList[lockList[idx]]->Unlock(unlockFromLeft);
        }

    }

    int getParentNodeIdx(int idx)
    {
        return ceil((float)idx / 2.0f) - 1;
    }

    int getChildNode(int idx, bool getLeft = true)
    {
        return 2 * idx + (getLeft ? 1 : 2);
    }

    // Returns if the lock is left or not left
    bool getLockDirection(int previousIdx, int targetIdx)
    {
        if (getChildNode(targetIdx) > threadCount)
        {
            if (getThreadID() % 2 == 0) return true;
            else return false;
        }
        if (getChildNode(targetIdx) == previousIdx) return true;
        return false;
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
    pthread_t* threads = (pthread_t*) malloc(sizeof(pthread_t) * threadCt);
    for(int idx = 0; idx < threadCt; idx++) threads[idx] = 0;

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
        mutex = new Tournament(threads, threadCt);
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
        threads[i] = newThread;
    }

    for(int idx = 0; idx < threadCt; idx++) pthread_join(threads[idx], NULL);

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
