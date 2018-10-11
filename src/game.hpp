#ifndef GAME_H
#define GAME_H

#include "scene.hpp"
#include "alloc.hpp"
#include "render/quad.hpp"


class InGameScene: public Scene
{
    private:
        GLRef pHorizonTexture, pSandTexture;

        QuadRenderer mQuadRenderer;
    public:
        InGameScene();

        void Render(void);

        void TellInitJobs(Loader &);
};

#endif  // GAME_H
