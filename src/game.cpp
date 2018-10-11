#include <glm/gtc/matrix_transform.hpp>

#include "game.hpp"
#include "error.hpp"
#include "app.hpp"
#include "texture.hpp"


InGameScene::InGameScene()
{
}
void InGameScene::TellInitJobs(Loader &loader)
{
    pHorizonTexture = App::Instance().GetGLManager()->AllocTexture();
    loader.Add(new PNGTextureLoadJob("horizon", *pHorizonTexture));

    pSandTexture = App::Instance().GetGLManager()->AllocTexture();
    loader.Add(new PNGTextureLoadJob("sand", *pSandTexture));

    mQuadRenderer.TellInitJobs(loader);
}
void InGameScene::Render(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CHECK_GL();

    glDisable(GL_CULL_FACE);
    CHECK_GL();

    glDisable(GL_DEPTH_TEST);
    CHECK_GL();

    RenderQuad quad;
    quad.texture = *pHorizonTexture;
    quad.vertices[0].position = vec3(-1.0f, -1.0f, 0.0f);
    quad.vertices[0].texCoords = vec2(0.0f, 0.0f);
    quad.vertices[1].position = vec3(1.0f, -1.0f, 0.0f);
    quad.vertices[1].texCoords = vec2(1.0f, 0.0f);
    quad.vertices[2].position = vec3(1.0f, 1.0f, 0.0f);
    quad.vertices[2].texCoords = vec2(1.0f, 1.0f);
    quad.vertices[3].position = vec3(-1.0f, 1.0f, 0.0f);
    quad.vertices[3].texCoords = vec2(0.0f, 1.0f);

    mat4 proj = ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);

    mQuadRenderer.Render(proj, quad);
}
