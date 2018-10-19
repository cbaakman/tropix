#ifndef APP_HPP
#define APP_HPP

#include <mutex>
#include <atomic>

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#include <boost/filesystem.hpp>

#include "config.hpp"
#include "event.hpp"
#include "scene.hpp"
#include "alloc.hpp"


class GLLock;

class App: public EventListener
{
    private:
        boost::filesystem::path exePath;

        std::mutex mtxConfig;
        Config mConfig;

        SDL_Window *mMainWindow;
        SDL_GLContext mMainGLContext;
        std::recursive_mutex mtxGL;
        size_t lockLevelGL;

        std::atomic<bool> running;

        std::mutex mtxCurrentScene;
        Scene *pCurrentScene;

        GLManager mGLManager;

        void Init(void);
        void Free(void);

        void OnEvent(const SDL_Event &);

        App(void);
        ~App(void);

        App(const App &) = delete;
        void operator=(const App &) = delete;
    public:
        void Run(void);
        void StopRunning(void);
        bool IsRunning(void);

        void GetConfig(Config &);

        GLManager *GetGLManager(void);
        boost::filesystem::path GetResourcePath(const std::string &location) const;

        void GetScreenDimensions(size_t &w, size_t &h) const;

        GLLock GetGLLock(void);  // scoped
        void LockGL(void);
        void UnlockGL(void);

        void SwitchScene(Scene *);

        static App &Instance(void);

    friend int main(int argc, char **argv);
};

class GLLock
{
    private:
        GLLock(const GLLock &) = delete;
        void operator=(const GLLock &) = delete;
    public:
        GLLock(void);
        ~GLLock(void);
};

#endif  // APP_HPP
