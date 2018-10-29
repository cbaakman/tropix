#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "game.hpp"
#include "error.hpp"
#include "app.hpp"
#include "texture.hpp"


#define DAYPERIOD 5.0

InGameScene::InGameScene(void)
: mChunkManager(483417628069),
  mSkyRenderer(20),
  prevTime(std::chrono::system_clock::now()), t(0.0f)
{
    mChunkManager.Connect(&mGroundRenderer);
    mChunkManager.Connect(&mPlayer);

    Config config;
    App::Instance().GetConfig(config);

    mTextRenderer.SetProjection(ortho(0.0f, (GLfloat)config.resolution.width,
                                      0.0f, (GLfloat)config.resolution.height,
                                      -1.0f, 1.0f));
    mTextParams.startX = 10.0f;
    mTextParams.startY = config.resolution.height - 30.0f;
    mTextParams.maxWidth = FLT_MAX;
    mTextParams.lineSpacing = 20.0f;
    mTextParams.align = TextGL::TEXTALIGN_LEFT;
}
InGameScene::~InGameScene(void)
{
    mChunkManager.DestroyAll();
}
Player::Player(void)
 :yaw(0.0f), pitch(0.0f), position(0.0f, 2.0f, 0.0f)
{
}
void InGameScene::TellInit(Queue &queue)
{
    mTextRenderer.TellInit(queue);
    mChunkManager.TellInit(queue);
    mSkyRenderer.TellInit(queue);
    mGroundRenderer.TellInit(queue);
    mWaterRenderer.TellInit(queue);
}
SDL_Keycode KeyInterpreter::GetConfigKeyCode(const KeyBinding binding) const
{
    Config config;
    App::Instance().GetConfig(config);

    if (config.controls.find(binding) != config.controls.end())
        return config.controls.at(binding);
    else
        return NULL;
}
bool KeyInterpreter::IsKeyDown(const KeyBinding kb) const
{
    SDL_Keycode kc = GetConfigKeyCode(kb);
    if (kc == NULL)
        return false;

    SDL_Scancode sc = SDL_GetScancodeFromKey(kc);
    const Uint8 *state = SDL_GetKeyboardState(NULL);

    return state[sc] != NULL;
}
bool KeyInterpreter::IsKey(const KeyBinding kb, const SDL_Keycode kc) const
{
    return GetConfigKeyCode(kb) == kc;
}
void InGameScene::Start(void)
{
    mChunkManager.Start();
}
void InGameScene::Stop(void)
{
    mChunkManager.Stop();
}
#define MOVE_SPEED 10.0f
void InGameScene::Update(void)
{
    std::chrono::time_point<std::chrono::system_clock> time = std::chrono::system_clock::now();
    dt = float(std::chrono::duration_cast<std::chrono::milliseconds>(time - prevTime).count()) / 1000;
    prevTime = time;
    t += dt;


    dayCycle = fmod(dayCycle + (double)dt / DAYPERIOD, 1.0);

    mPlayer.Update(dt);

    mChunkManager.ThrowAnyError();
}
void Player::Update(const float dt)
{
    {
        std::scoped_lock lock(mtxPosition);

        if (mKeyInterpreter.IsKeyDown(KEYB_JUMP))
            position += vec3(0.0f, 1.0f, 0.0f) * MOVE_SPEED * dt;
        else if(mKeyInterpreter.IsKeyDown(KEYB_DUCK))
            position -= vec3(0.0f, 1.0f, 0.0f) * MOVE_SPEED * dt;

        if(mKeyInterpreter.IsKeyDown(KEYB_GOFORWARD))
            position += MOVE_SPEED * dt * rotate(vec3(0.0f, 0.0f, -1.0f), radians(yaw), vec3(0.0f, 1.0f, 0.0f));
        else if(mKeyInterpreter.IsKeyDown(KEYB_GOBACK))
            position += MOVE_SPEED * dt * rotate(vec3(0.0f, 0.0f, 1.0f), radians(yaw), vec3(0.0f, 1.0f, 0.0f));

        if(mKeyInterpreter.IsKeyDown(KEYB_GOLEFT))
            position += MOVE_SPEED * dt * rotate(vec3(-1.0f, 0.0f, 0.0f), radians(yaw), vec3(0.0f, 1.0f, 0.0f));
        else if(mKeyInterpreter.IsKeyDown(KEYB_GORIGHT))
            position += MOVE_SPEED * dt * rotate(vec3(1.0f, 0.0f, 0.0f), radians(yaw), vec3(0.0f, 1.0f, 0.0f));
    }
}
void InGameScene::Render(void)
{
    mat4 view, proj;
    vec3 position;

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

    Config config;
    App::Instance().GetConfig(config);

    proj = perspectiveFov(45.0f,
                          (GLfloat)config.resolution.width, (GLfloat)config.resolution.height,
                          0.1f, config.render.distance);

    position = mPlayer.GetWorldPosition();
    view = translate(view, position);
    view = rotate(view, radians(mPlayer.GetYaw()), vec3(0.0f, 1.0f, 0.0f));
    view = rotate(view, radians(mPlayer.GetPitch()), vec3(1.0f, 0.0f, 0.0f));
    view = inverse(view);

    mSkyRenderer.Render(proj, view, position.y, horizonColor, skyColor);

    mGroundRenderer.Render(proj, view, position, horizonColor, lightDirection);

    mWaterRenderer.Render(proj, view, position, lightDirection, t);

    char text[256];
    sprintf(text, "dt: %.3f, FPS: %.1f", dt, 1.0f / dt);
    mTextRenderer.IterateText(App::Instance().GetFontManager()->GetFont(FONT_SMALLBLACK),
                              (int8_t *)text, mTextParams);
}
void InGameScene::OnEvent(const SDL_Event &event)
{
    mPlayer.OnEvent(event);

    EventListener::OnEvent(event);
}
#define MOUSE_SENSITIVITY 1.0f
void Player::OnMouseMove(const SDL_MouseMotionEvent &event)
{
    std::scoped_lock lock(mtxPosition);

    yaw -= event.xrel * MOUSE_SENSITIVITY;
    pitch -= event.yrel * MOUSE_SENSITIVITY;

    if (pitch < -90.0f)
        pitch = -90.0f;
    else if (pitch > 90.0f)
        pitch = 90.0f;
}
vec3 Player::GetWorldPosition(void) const
{
    std::scoped_lock lock(mtxPosition);

    return position;
}
float Player::GetYaw(void) const
{
    std::scoped_lock lock(mtxPosition);

    return yaw;
}
float Player::GetPitch(void) const
{
    std::scoped_lock lock(mtxPosition);

    return pitch;
}
