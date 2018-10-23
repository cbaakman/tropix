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
#include "concurrency.hpp"
#include "text.hpp"


class KeyInterpreter
{
    private:
        SDL_Keycode GetConfigKeyCode(const KeyBinding) const;
    public:
        bool IsKeyDown(const KeyBinding) const;
        bool IsKey(const KeyBinding, const SDL_Keycode) const;
};


class Player: public ChunkObserver, public EventListener
{
    private:
        KeyInterpreter mKeyInterpreter;

        vec3 position;
        float yaw, pitch;
        mutable std::recursive_mutex mtxPosition;
    public:
        Player(void);

        vec3 GetWorldPosition(void) const;
        float GetYaw(void) const;
        float GetPitch(void) const;

        void Update(const float dt);

        void OnMouseMove(const SDL_MouseMotionEvent &);
};

class InGameScene: public InitializableScene
{
    private:
        std::chrono::time_point<std::chrono::system_clock> prevTime;

        Player mPlayer;

        double dayCycle;
        float dt;

        TextGL::TextParams mTextParams;
        TextRenderer mTextRenderer;

        GroundRenderer mGroundRenderer;
        SkyRenderer mSkyRenderer;
        ChunkManager mChunkManager;
    public:
        InGameScene(void);
        ~InGameScene(void);

        void Start(void);
        void Update(void);
        void Render(void);
        void Stop(void);

        void TellInit(Loader &);

        void OnEvent(const SDL_Event &);
};

#endif  // GAME_HPP
