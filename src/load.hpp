#ifndef LOAD_HPP
#define LOAD_HPP

#include <list>
#include <exception>
#include <thread>
#include <mutex>
#include <memory>

#include <boost/filesystem.hpp>


class LoadJob
{
    public:
        // This must be thread-safe.
        virtual void Run(void) = 0;
};

struct LoadStats
{
    size_t countJobsAtStart,
           countJobsRemain;
};

class Loader
{
    private:
        std::mutex mtxQueue;
        std::list<LoadJob *> mQueue;
        std::list<std::exception_ptr> mErrors;

        size_t countJobsAtStart;
        std::mutex mtxStats;

        size_t countWorkers;
        std::thread *mWorkers;

        LoadJob *Take(void);

        void PushError(const std::exception_ptr &);
        void ThrowAnyError(void);

        static void Work(Loader *);
    public:
        Loader(const size_t concurrency);
        ~Loader(void);

        void Add(LoadJob *);  // Job is deleted when done.

        void Run(void);

        void GetStats(LoadStats &);
};

class Initializable
{
    public:
        virtual void TellInit(Loader &loader) = 0;
};

#endif  // LOAD_HPP
