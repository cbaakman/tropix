#ifndef CONCURRENCY_HPP
#define CONCURRENCY_HPP

#include <thread>
#include <mutex>
#include <atomic>
#include <list>
#include <vector>

#include "error.hpp"


class ConcurrentManager
{
    private:
        std::vector<std::thread> mThreads;
        std::recursive_mutex mtxThreads;
    public:
        ~ConcurrentManager(void) { JoinAll(); }

        template<class Function, class... Args>
        void Start(const size_t count, Function&& f, Args&&... args)
        {
            std::scoped_lock lock(mtxThreads);

            if (mThreads.size() > 0)
                throw RuntimeError("Earlier worker threads are still allocated!");


            size_t i;
            mThreads = std::vector<std::thread>(count);
            for (i = 0; i < count; i++)
                mThreads[i] = std::thread(f, args...);
        }

        void JoinAll(void)
        {
            std::scoped_lock lock(mtxThreads);

            size_t i;
            for (i = 0; i < mThreads.size(); i++)
                if (mThreads[i].joinable())
                    mThreads[i].join();

            mThreads.clear();
        }
};

#endif  // CONCURRENCY_HPP
