#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>


struct Resolution
{
    size_t width, height;
};


struct Config
{
    size_t loadConcurrency;

    bool fullscreen;
    Resolution resolution;
};

#endif  // CONFIG_H
