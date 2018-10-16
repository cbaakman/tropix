#ifndef SCENE_HPP
#define SCENE_HPP

#include "event.hpp"
#include "load.hpp"

class Scene: public EventListener, public Initializable
{
    public:
        virtual void Update(void) = 0;
        virtual void Render(void) = 0;
};

#endif  // SCENE_HPP
