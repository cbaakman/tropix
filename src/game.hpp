#ifndef GAME_HPP
#define GAME_HPP

#include <chrono>
#include <thread>
#include <atomic>

#include "config.hpp"
#include "load.hpp"
#include "alloc.hpp"
#include "render/quad.hpp"
#include "ground.hpp"
#include "sky.hpp"


class KeyInterpreter
{
    private:
        Controls controls;
    public:
        void Set(const Controls &);
        bool IsKeyDown(const KeyBinding) const;
        bool IsKey(const KeyBinding, const SDL_Keycode) const;
};

class InGameScene: public InitializableScene
{
    private:
        KeyInterpreter mKeyInterpreter;
        std::chrono::time_point<std::chrono::system_clock> prevTime;

        std::recursive_mutex mtxPlayer;
        vec3 position;
        float yaw, pitch;
        GLfloat renderDistance;

        double dayCycle;
        GroundRenderer mGroundRenderer;
        SkyRenderer mSkyRenderer;

        size_t countChunkLoadThreads;
        static void ChunkLoadThreadFunc(InGameScene *);
        std::atomic<bool> running;
        std::thread *mChunkLoadThreads;
    public:
        InGameScene(const Config &config);

        void Start(void);
        void Update(void);
        void Render(void);
        void Stop(void);

        void TellInit(Loader &);

        void OnMouseMove(const SDL_MouseMotionEvent &);
};

#endif  // GAME_HPP
