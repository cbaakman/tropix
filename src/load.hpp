#ifndef LOAD_HPP
#define LOAD_HPP

#include <list>
#include <exception>
#include <thread>
#include <mutex>
#include <memory>

#include <boost/filesystem.hpp>

#include "scene.hpp"
#include "alloc.hpp"
#include "concurrency.hpp"


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

        ConcurrentManager mConcurrentManager;

        LoadJob *Take(void);

        void PushError(const std::exception_ptr &);
        void ThrowAnyError(void);

        static void Work(Loader *);
    public:
        Loader(void);
        ~Loader(void);

        void Add(LoadJob *);  // Job is deleted when done.

        void Run(void);
        void Clear(void);  // Removes remaining jobs, stopping 'Run'.

        void GetStats(LoadStats &);
};

class Initializable
{
    public:
        virtual void TellInit(Loader &loader) = 0;
};

class InitializableScene: public Initializable, public Scene
{
};

class LoadScene: public Scene
{
    private:
        GLRef pProgram,
              pBuffer;

        InitializableScene *pLoaded;

        Loader mLoader;

        std::thread mLoadThread;
        static void LoadThreadFunc(LoadScene *);
        void InterruptLoading(void);
    public:
        LoadScene(InitializableScene *pLoaded);
        ~LoadScene(void);

        void Start(void);
        void Render(void);
        void Update(void);
        void Stop(void);
};

#endif  // LOAD_HPP
