#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <map>
#include <stddef.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <SDL2/SDL.h>


struct Resolution
{
    size_t width, height;
};

struct Rendering
{
    GLfloat distance;
};

enum KeyBinding
{
    KEYB_JUMP,
    KEYB_DUCK,
    KEYB_GOFORWARD,
    KEYB_GOBACK,
    KEYB_GOLEFT,
    KEYB_GORIGHT,
};

typedef std::map<KeyBinding, SDL_Keycode> Controls;

struct Config
{
    size_t loadConcurrency;

    bool fullscreen;
    Resolution resolution;

    Rendering render;

    Controls controls;
};

#endif  // CONFIG_HPP
