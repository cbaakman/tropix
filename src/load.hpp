#ifndef LOAD_HPP
#define LOAD_HPP

#include <queue>
#include <exception>
#include <mutex>
#include <memory>

#include <boost/filesystem.hpp>

#include "scene.hpp"
#include "alloc.hpp"
#include "concurrency.hpp"


class Job
{
    public:
        // This must be thread-safe.
        virtual void Run(void) = 0;
};

class Queue
{
    private:
        std::recursive_mutex mtxJobs;
        std::list<std::shared_ptr<Job>> mJobs;
    public:
        std::shared_ptr<Job> Take(void);
        void Add(std::shared_ptr<Job>);
        void Add(Job *);  // Gets deleted automatically.
        size_t Size(void);
};

class ErrorManager
{
    private:
        std::recursive_mutex mtxErrors;
        std::list<std::exception_ptr> mErrors;
    public:
        void PushError(const std::exception_ptr &);
        void ThrowAnyError(void);
};

void WorkThreadFunc(Queue &, ErrorManager &);  // Deletes the jobs after running. Stops if no more jobs.
void WorkAllFrom(Queue &);
void ClearAllFrom(Queue &);

class Initializable
{
    public:
        virtual void TellInit(Queue &) = 0;
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

        size_t countStartJobs;
        Queue mQueue;
        ConcurrentManager mConcurrentManager;
        ErrorManager mErrorManager;

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
