#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "game.hpp"
#include "error.hpp"
#include "app.hpp"
#include "texture.hpp"


#define DAYPERIOD 5.0

InGameScene::InGameScene(const Config &config)
: mGroundRenderer(483417628069, config.render.distance, config.render.gridSubdiv),
  mSkyRenderer(config.render.distance, config.render.gridSubdiv),
  yaw(0.0f), pitch(0.0f), position(0.0f, 2.0f, 0.0f),
  renderDistance(config.render.distance),
  prevTime(std::chrono::system_clock::now())
{
    mKeyInterpreter.Set(config.controls);
}
void InGameScene::TellInit(Loader &loader)
{
    mSkyRenderer.TellInit(loader);
    mGroundRenderer.TellInit(loader);
}
void KeyInterpreter::Set(const Controls &cs)
{
    controls = cs;
}
bool KeyInterpreter::IsKeyDown(const KeyBinding kb) const
{
    if (controls.find(kb) == controls.end())
        return false;

    SDL_Scancode sc = SDL_GetScancodeFromKey(controls.at(kb));
    const Uint8 *state = SDL_GetKeyboardState(NULL);

    return state[sc] != NULL;
}
bool KeyInterpreter::IsKey(const KeyBinding kb, const SDL_Keycode kc) const
{
    if (controls.find(kb) != controls.end())
        return controls.at(kb) == kc;

    return false;
}
#define MOVE_SPEED 5.0f
void InGameScene::Update(void)
{
    std::chrono::time_point<std::chrono::system_clock> time = std::chrono::system_clock::now();
    float dt = float(std::chrono::duration_cast<std::chrono::milliseconds>(time - prevTime).count()) / 1000;
    prevTime = time;


    dayCycle = fmod(dayCycle + (double)dt / DAYPERIOD, 1.0);


    if (mKeyInterpreter.IsKeyDown(KEYB_JUMP))
        position.y += MOVE_SPEED * dt;
    else if(mKeyInterpreter.IsKeyDown(KEYB_DUCK))
        position.y -= MOVE_SPEED * dt;
    if(mKeyInterpreter.IsKeyDown(KEYB_GOFORWARD))
        position += MOVE_SPEED * dt * rotate(vec3(0.0f, 0.0f, -1.0f), radians(yaw), vec3(0.0f, 1.0f, 0.0f));
    else if(mKeyInterpreter.IsKeyDown(KEYB_GOBACK))
        position += MOVE_SPEED * dt * rotate(vec3(0.0f, 0.0f, 1.0f), radians(yaw), vec3(0.0f, 1.0f, 0.0f));
    if(mKeyInterpreter.IsKeyDown(KEYB_GOLEFT))
        position += MOVE_SPEED * dt * rotate(vec3(-1.0f, 0.0f, 0.0f), radians(yaw), vec3(0.0f, 1.0f, 0.0f));
    else if(mKeyInterpreter.IsKeyDown(KEYB_GORIGHT))
        position += MOVE_SPEED * dt * rotate(vec3(1.0f, 0.0f, 0.0f), radians(yaw), vec3(0.0f, 1.0f, 0.0f));
}
void InGameScene::Render(void)
{
    mat4 view, proj;

    double angle = 2 * pi<double>() * dayCycle,
           sAngle = sin(angle), cAngle = cos(angle),
           daylight = max(0.0, -sAngle),
           sunset = abs(cAngle);
    vec3 lightDirection(0.0f, sAngle, cAngle);
    vec4 horizonColor, skyColor;

    horizonColor = vec4(sunset, 0.2f + 0.6f * daylight + 0.2 * sunset, 0.5f + 0.5f * daylight, 1.0f);
    skyColor = vec4(0.0f, 0.15f + 0.35f * daylight, 0.3f + 0.7f * daylight, 1.0f);

    glClearDepth(1.0f);
    CHECK_GL();

    glClear(GL_DEPTH_BUFFER_BIT);
    CHECK_GL();

    size_t w, h;
    App::Instance().GetScreenDimensions(w, h);
    proj = perspectiveFov(45.0f, (GLfloat)w, (GLfloat)h, 0.1f, renderDistance);

    view = translate(view, position);
    view = rotate(view, radians(yaw), vec3(0.0f, 1.0f, 0.0f));
    view = rotate(view, radians(pitch), vec3(1.0f, 0.0f, 0.0f));
    view = inverse(view);

    mSkyRenderer.Render(proj, view, position.y, horizonColor, skyColor);

    mGroundRenderer.Render(proj, view, vec2(position.x, position.z), horizonColor, lightDirection);
}
#define MOUSE_SENSITIVITY 1.0f
void InGameScene::OnMouseMove(const SDL_MouseMotionEvent &event)
{
    yaw -= event.xrel * MOUSE_SENSITIVITY;
    pitch -= event.yrel * MOUSE_SENSITIVITY;

    if (pitch < -90.0f)
        pitch = -90.0f;
    else if (pitch > 90.0f)
        pitch = 90.0f;
}
