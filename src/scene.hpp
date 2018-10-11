#ifndef SCENE_H
#define SCENE_H

#include "event.hpp"
#include "load.hpp"

class Scene: public EventListener, public Initializable
{
    public:
        virtual void Render(void) = 0;
};

#endif  // SCENE_H
