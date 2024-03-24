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
    // Stores both mutexes and threads
    struct TreeNode
    {
        bool isMutex;
        Petersons* mutex;
        pthread_t tId;
    };

    int threadCt;
    pthread_t* threadIdArr;
    int treeSize;
    TreeNode* treeList;

public:
    Tournament(pthread_t* threadArr, int arrSize)
    {
        threadIdArr = threadArr;
        threadCt = arrSize;
        treeSize = arrSize * 2 - 1;
        treeList = new TreeNode[treeSize];
        for (size_t i = 0; i < threadCt-1; i++) 
        {
            treeList[i].isMutex = true;
            treeList[i].mutex = new Petersons();
        }
        for (size_t i = threadCt; i < threadCt*2; i++)
        {
            treeList[i].isMutex = false;
        }
    }

    void Lock()
    {
        int prevNode = getNodeFromTree();
        int curNode = getParentNode(prevNode);
        while(true)
        {
            bool lockFromLeft = getLockDirection(prevNode, curNode);
            treeList[curNode].mutex->Lock(lockFromLeft);
            if(curNode == 0) break;
            prevNode = curNode;
            curNode = getParentNode(curNode);
        }
    }

    void Unlock()
    {
        vector<int> lockList;
        int prevNode = getNodeFromTree();
        int curNode = getParentNode(prevNode);
        lockList.push_back(prevNode);
        while(true)
        {
            lockList.push_back(curNode);
            if(curNode == 0) break;
            curNode = getParentNode(curNode);
        }
        
        for(int i = lockList.size() - 1; i >= 0; i--)
        {
            curNode = lockList[i];
            if(!treeList[curNode].isMutex) return;
            bool lockFromLeft = getLockDirection(lockList[i-1], curNode);
            treeList[curNode].mutex->Unlock(lockFromLeft);
        }
    }

    int getNodeFromTree()
    {
        pthread_t tid = pthread_self();
        int idx = 9999;

        while(idx == 9999)
        {
            for(int i=0; i<threadCt; i++)
            {
                //cout << "checking threadid" << threadIdArr[i] << "\n";
                if(threadIdArr[i] == tid)
                {
                    idx = i;
                }
            }
        }
        treeList[idx + threadCt - 1].tId = tid;
        return idx + threadCt - 1;
    }

    int getParentNode(int idx)
    {
        return ceil((float)idx / 2.0f) - 1;
    }

    int getChildNode(int idx, bool getLeft = true)
    {
        return 2 * idx + (getLeft ? 1 : 2);
    }

    // Returns if the target idx node is the left child of the previous idx node
    bool getLockDirection(int childIdx, int parentIdx)
    {
        if(getChildNode(parentIdx, true) == childIdx) return true;
        if(getChildNode(parentIdx, false) == childIdx) return false;
        throw new std::exception();
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
