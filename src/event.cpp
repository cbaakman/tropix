#include <iostream>

#include "event.hpp"


void EventListener::OnEvent(const SDL_Event &event)
{
    switch (event.type)
    {
    case SDL_MOUSEMOTION:
        OnMouseMove(event.motion);
        break;
    default:
        break;
    }
}
void EventListener::OnMouseMove(const SDL_MouseMotionEvent &)
{
}
