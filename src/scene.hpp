#ifndef SCENE_HPP
#define SCENE_HPP

#include "event.hpp"

class Scene: public EventListener
{
    public:
        virtual void Start(void) {}
        virtual void Update(void) = 0;
        virtual void Render(void) = 0;
        virtual void Stop(void) {}
};

#endif  // SCENE_HPP
