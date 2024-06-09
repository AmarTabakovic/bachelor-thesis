#ifndef LOADWORKERMANAGER_H
#define LOADWORKERMANAGER_H

#include "loadworkerthread.h"

class LoadWorkerManager
{
public:
    LoadWorkerManager(unsigned numThreads);
    void start();
    std::vector<LoadWorkerThread> _workers;
};

#endif // LOADWORKERMANAGER_H
