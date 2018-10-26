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
  pCurrentScene(NULL)
{
}
App::~App(void)
{
    if (HasSystem())
    {
        mFontManager.DestroyAll();
        mGLManager.DestroyAll();
        SystemFree();
    }
}
void App::GetConfig(Config &config)
{
    if (!boost::filesystem::exists(exePath))
        throw IOError("No valid executable path is set!");

    std::scoped_lock lock(mtxConfig);

    config.loadConcurrency = 4;
    config.fullscreen = false;
    config.resolution.width = 800;
    config.resolution.height = 600;

    config.render.distance = 1000.0f;

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
FontManager *App::GetFontManager(void)
{
    return &mFontManager;
}
boost::filesystem::path App::GetResourcePath(const std::string &location) const
{
    return exePath.parent_path() / "resources" / location;
}
bool App::HasSystem(void)
{
    return mMainGLContext != NULL;
}
void App::SystemFree(void)
{
    if (mMainGLContext != NULL)
        SDL_GL_DeleteContext(mMainGLContext);
    mMainGLContext = NULL;

    if (mMainWindow != NULL)
        SDL_DestroyWindow(mMainWindow);
    mMainWindow = NULL;

    SDL_Quit();
}
void App::SystemInit(void)
{
    Config config;
    GetConfig(config);

    int error = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
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
    if (mMainWindow == NULL)
    {
        throw InitError("SDL_CreateWindow failed: %s", SDL_GetError());
    }

    if (SDL_SetRelativeMouseMode(SDL_TRUE) < 0)
    {
        throw InitError("Failed to set relative mouse mode: %s", SDL_GetError());
    }

    mMainGLContext = SDL_GL_CreateContext(mMainWindow);
    if (mMainGLContext == NULL)
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
}
void App::SwitchScene(Scene *p)
{
    std::scoped_lock lock(mtxCurrentScene);

    if (pCurrentScene != NULL)
        pCurrentScene->Stop();

    pCurrentScene = p;
    if (pCurrentScene != NULL)
        pCurrentScene->Start();
}
void App::PushGL(LoadJob *p)
{
    mGLQueue.Add(p);
}
void App::Run(void)
{
    SDL_Event event;

    if (!HasSystem())
        SystemInit();

    mFontManager.InitAll(GetResourcePath("tiki.svg"));

    // Scene scope.
    InGameScene gameScene;
    LoadScene loadScene(&gameScene);
    SwitchScene(&loadScene);

    running = true;
    while (running)
    {
        while (SDL_PollEvent(&event))
            OnEvent(event);

        mGLQueue.ConsumeAll();

        // In this scope, we lock the current scene.
        {
            std::scoped_lock lock(mtxCurrentScene);
            pCurrentScene->Update();

            // In this scope, we lock the GL context for rendering.
            pCurrentScene->Render();
            SDL_GL_SwapWindow(mMainWindow);
        }
    }

    // Let the current scene Know that it's ending.
    SwitchScene(NULL);
}
void App::OnEvent(const SDL_Event &event)
{
    if (event.type == SDL_QUIT)
        StopRunning();

    else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
        StopRunning();

    else if (pCurrentScene != NULL)
    {
        std::scoped_lock lock(mtxCurrentScene);

        pCurrentScene->OnEvent(event);
    }
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
