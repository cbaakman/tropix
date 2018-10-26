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
#include "text.hpp"
#include "load.hpp"


class GLLock;

class App: public EventListener
{
    private:
        boost::filesystem::path exePath;

        std::recursive_mutex mtxConfig;
        Config mConfig;

        SDL_Window *mMainWindow;
        SDL_GLContext mMainGLContext;

        std::atomic<bool> running;

        std::recursive_mutex mtxCurrentScene;
        Scene *pCurrentScene;

        GLManager mGLManager;
        BottleNeckQueue mGLQueue;
        FontManager mFontManager;

        bool HasSystem(void);
        void SystemInit(void);
        void SystemFree(void);

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
        FontManager *GetFontManager(void);

        boost::filesystem::path GetResourcePath(const std::string &location) const;

        void PushGL(LoadJob *);

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
