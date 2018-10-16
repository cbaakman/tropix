#include <iostream>

#include "event.hpp"
#include "error.hpp"
#include "app.hpp"
#include "load.hpp"
#include "game.hpp"


App &App::Instance(void)
{
    static App app;
    return app;
}
App::App(void)
: mMainGLContext(NULL), mMainWindow(NULL), running(false),
  pCurrentScene(NULL), countLocksGL(0)
{
}
App::~App(void)
{
    Free();
}
void App::Free(void)
{
    if (mMainGLContext)
        SDL_GL_DeleteContext(mMainGLContext);

    if (mMainWindow)
        SDL_DestroyWindow(mMainWindow);

    SDL_Quit();
}
void App::GetConfig(Config &config)
{
    std::scoped_lock lock(mtxConfig);

    config.loadConcurrency = 4;
    config.fullscreen = false;
    config.resolution.width = 800;
    config.resolution.height = 600;

    config.render.distance = 100000.0f;
    config.render.gridSubdiv = 50;

    config.controls[KEYB_JUMP] = SDLK_SPACE;
    config.controls[KEYB_DUCK] = SDLK_LSHIFT;
    config.controls[KEYB_GOFORWARD] = SDLK_w;
    config.controls[KEYB_GOBACK] = SDLK_s;
    config.controls[KEYB_GOLEFT] = SDLK_a;
    config.controls[KEYB_GORIGHT] = SDLK_d;
}
GLManager *App::GetGLManager(void)
{
    return &mGLManager;
}
boost::filesystem::path App::GetResourcePath(const std::string &location) const
{
    return exePath.parent_path() / "resources" / location;
}
void App::Init(void)
{
    Config config;
    GetConfig(config);

    int error = SDL_Init(SDL_INIT_EVERYTHING);
    if (error != 0)
    {
        throw InitError("Unable to initialize SDL: %s", SDL_GetError());
    }

    // Set the openGL parameters we want:
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
    if (config.fullscreen)
        flags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_INPUT_GRABBED;

    mMainWindow = SDL_CreateWindow("Tropix",
                                   SDL_WINDOWPOS_UNDEFINED,
                                   SDL_WINDOWPOS_UNDEFINED,
                                   config.resolution.width,
                                   config.resolution.height,
                                   flags);
    if (!mMainWindow)
    {
        throw InitError("SDL_CreateWindow failed: %s", SDL_GetError());
    }

    if (SDL_SetRelativeMouseMode(SDL_TRUE) < 0)
    {
        throw InitError("Failed to set relative mouse mode: %s", SDL_GetError());
    }

    mMainGLContext = SDL_GL_CreateContext(mMainWindow);
    if (!mMainGLContext)
    {
        throw InitError("Failed to create GL context: %s", SDL_GetError());
    }

    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        throw InitError("glewInit failed: %s", glewGetErrorString(err));
    }

    if (!GLEW_VERSION_3_2)
    {
        throw InitError("OpenGL version 3.2 is not enabled.");
    }

    // Keep the context free until locked.
    SDL_GL_MakeCurrent(NULL, NULL);
}
void App::GetScreenDimensions(size_t &width, size_t &height) const
{
    int w, h;
    SDL_GetWindowSize(mMainWindow, &w, &h);

    width = w;
    height = h;
}
void App::LockGL(void)
{
    mtxGL.lock();
    countLocksGL++;

    if (SDL_GL_MakeCurrent(mMainWindow, mMainGLContext) != 0)
    {
        throw GLError("SDL_GL_MakeCurrent: %s", SDL_GetError());
    }
}
void App::UnlockGL(void)
{
    countLocksGL--;

    if (countLocksGL <= 0 && SDL_GL_MakeCurrent(NULL, NULL) != 0)
    {
        throw GLError("SDL_GL_MakeCurrent: %s", SDL_GetError());
    }
    mtxGL.unlock();
}
GLLock::GLLock()
{
    App::Instance().LockGL();
}
GLLock::~GLLock(void)
{
    App::Instance().UnlockGL();
}
GLLock App::GetGLLock(void)
{
    return GLLock();
}
void App::Run(void)
{
    SDL_Event event;

    Init();

    Config config;
    GetConfig(config);
    {
        std::scoped_lock lock(mtxCurrentScene);
        pCurrentScene = new InGameScene(config);
    }

    // Load resources.
    Loader loader(config.loadConcurrency);

    {
        GLLock lockGL = GetGLLock();
        std::scoped_lock lock(mtxCurrentScene);

        pCurrentScene->TellInit(loader);
    }

    loader.Run();

    running = true;

    while (IsRunning())
    {
        while (SDL_PollEvent(&event))
            OnEvent(event);

        {
            GLLock lockGL = GetGLLock();

            {
                std::scoped_lock lock(mtxCurrentScene);
                pCurrentScene->Update();
                pCurrentScene->Render();
            }

            SDL_GL_SwapWindow(mMainWindow);
        }
    }

    mGLManager.GarbageCollect();
    delete pCurrentScene;

    Free();
}
void App::OnEvent(const SDL_Event &event)
{
    std::scoped_lock lock(mtxCurrentScene);

    if (event.type == SDL_QUIT)
        StopRunning();

    else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
        StopRunning();

    else if (pCurrentScene != NULL)
        pCurrentScene->OnEvent(event);
}
void App::StopRunning(void)
{
    running = false;
}

bool App::IsRunning(void)
{
    return running;
}

int main(int argc, char **argv)
{
    App::Instance().exePath = argv[0];

    try
    {
        App::Instance().Run();

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;

        return 1;
    }
};
