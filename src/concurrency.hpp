#ifndef CONCURRENCY_HPP
#define CONCURRENCY_HPP

#include <thread>
#include <list>

#include "error.hpp"


class ConcurrentManager
{
    private:
        size_t countThreads;
        std::thread *mThreads;
        std::recursive_mutex mtxThreads;
    public:
        ConcurrentManager(void): mThreads(NULL), countThreads(0) {}

        ~ConcurrentManager(void) { JoinAll(); }

        template<class Function, class... Args>
        void Start(const size_t count, Function&& f, Args&&... args)
        {
            std::scoped_lock lock(mtxThreads);

            if (mThreads != NULL)
                throw RuntimeError("Earlier worker threads are still allocated!");

            countThreads = count;
            mThreads = new std::thread[countThreads];

            size_t i;
            for (i = 0; i < countThreads; i++)
                mThreads[i] = std::thread(f, args...);
        }

        void JoinAll(void)
        {
            std::scoped_lock lock(mtxThreads);

            size_t i;
            for (i = 0; i < countThreads; i++)
                if (mThreads[i].joinable())
                    mThreads[i].join();

            delete[] mThreads;
            mThreads = NULL;
            countThreads = 0;
        }
};

#endif  // CONCURRENCY_HPP
