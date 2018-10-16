#ifndef GAME_HPP
#define GAME_HPP

#include <chrono>

#include "config.hpp"
#include "scene.hpp"
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

class InGameScene: public Scene
{
    private:
        KeyInterpreter mKeyInterpreter;
        std::chrono::time_point<std::chrono::system_clock> prevTime;

        vec3 position;
        float yaw, pitch;

        double dayCycle;
        GroundRenderer mGroundRenderer;
        SkyRenderer mSkyRenderer;
    public:
        InGameScene(const Config &config);

        void Update(void);
        void Render(void);

        void TellInit(Loader &);

        void OnMouseMove(const SDL_MouseMotionEvent &);
};

#endif  // GAME_HPP
