#ifndef EVENT_HPP
#define EVENT_HPP

#include <SDL2/SDL.h>


class EventListener
{
public:
    virtual void OnMouseMove(const SDL_MouseMotionEvent &);
    virtual void OnEvent(const SDL_Event &);
};


#endif  // EVENT_HPP
