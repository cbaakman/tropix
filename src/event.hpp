#ifndef EVENT_H
#define EVENT_H

#include <SDL2/SDL.h>


class EventListener
{
public:
    virtual void OnEvent(const SDL_Event &);
};


#endif  // EVENT_H
