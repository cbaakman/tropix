#include <iostream>

#include "load.hpp"


Loader::Loader(const size_t concurrency)
{
    countWorkers = concurrency;
    mWorkers = new std::thread[concurrency];
}
Loader::~Loader(void)
{
    // Delete the remaining jobs.
    std::scoped_lock(mtxQueue);
    for (LoadJob *pJob: mQueue)
        delete pJob;

    // Clear the queue, so that workers don't pick up any more jobs.
    mQueue.clear();

    // Delete the worker threads.
    delete[] mWorkers;
}
void Loader::Work(Loader *pLoader)
{
    LoadJob *pJob;
    while ((pJob = pLoader->Take()) != NULL)
    {
        try
        {
            pJob->Run();
        }
        catch (...)
        {
            pLoader->PushError(std::current_exception());
        }

        delete pJob;
    }
}
void Loader::GetStats(LoadStats &stats)
{
    std::scoped_lock(mtxStats, mtxQueue);

    stats.countJobsAtStart = countJobsAtStart;
    stats.countJobsRemain = mQueue.size();
}
void Loader::Run(void)
{
    {
        std::scoped_lock(mtxStats);
        countJobsAtStart = mQueue.size();
    }

    size_t i;
    for (i = 0; i < countWorkers; i++)
        mWorkers[i] = std::thread(Work, this);

    for (i = 0; i < countWorkers; i++)
    {
        mWorkers[i].join();
    }
    ThrowAnyError();
}
void Loader::Add(LoadJob *pJob)
{
    std::scoped_lock(mtxQueue);
    mQueue.push_back(pJob);
}
LoadJob *Loader::Take(void)
{
    std::scoped_lock(mtxQueue);
    if (mQueue.size() > 0)
    {
        LoadJob *pJob = mQueue.front();
        mQueue.pop_front();
        return pJob;
    }
    else
        return NULL;
}
void Loader::PushError(const std::exception_ptr &e)
{
    std::scoped_lock(mtxQueue);
    mErrors.push_back(e);
}
void Loader::ThrowAnyError(void)
{
    std::scoped_lock(mtxQueue);
    if (mErrors.size() > 0)
        std::rethrow_exception(mErrors.front());
}
