#include <iostream>

#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/glm.hpp>
using namespace glm;

#include "load.hpp"
#include "error.hpp"
#include "app.hpp"
#include "shader.hpp"


Loader::Loader(void)
{
}
Loader::~Loader(void)
{
    Clear();

    mConcurrentManager.JoinAll();
}
void Loader::Work(Loader *pLoader)
{
    LoadJob *pJob;
    while ((pJob = pLoader->Take()) != NULL)
    {
        try
        {
            pJob->Run();
        }
        catch (...)
        {
            pLoader->PushError(std::current_exception());
        }

        delete pJob;
    }
}
void Loader::Clear(void)
{
    // Delete the remaining jobs.
    std::scoped_lock(mtxQueue);
    for (LoadJob *pJob : mQueue)
    {
        delete pJob;
    }

    // Clear the queue, so that workers don't pick up any deleted pointers.
    mQueue.clear();
}
void Loader::GetStats(LoadStats &stats)
{
    std::scoped_lock(mtxStats, mtxQueue);

    stats.countJobsAtStart = countJobsAtStart;
    stats.countJobsRemain = mQueue.size();
}
void Loader::Run(void)
{
    {
        std::scoped_lock(mtxStats);
        countJobsAtStart = mQueue.size();
    }

    Config config;
    App::Instance().GetConfig(config);

    mConcurrentManager.Start(config.loadConcurrency, Work, this);

    mConcurrentManager.JoinAll();
    ThrowAnyError();
}
void Loader::Add(LoadJob *pJob)
{
    std::scoped_lock(mtxQueue);
    mQueue.push_back(pJob);
}
LoadJob *Loader::Take(void)
{
    std::scoped_lock(mtxQueue);
    if (mQueue.size() > 0)
    {
        LoadJob *pJob = mQueue.front();
        mQueue.pop_front();
        return pJob;
    }
    else
        return NULL;
}
void Loader::PushError(const std::exception_ptr &e)
{
    std::scoped_lock(mtxQueue);
    mErrors.push_back(e);
}
void Loader::ThrowAnyError(void)
{
    std::scoped_lock(mtxQueue);
    if (mErrors.size() > 0)
        std::rethrow_exception(mErrors.front());
}

struct LoadVertex
{
    vec2 position;
};

#define LOAD_POSITON_INDEX 0

char loadVertexShaderSrc[] = R"shader(
#version 150

in vec2 position;

out VertexData
{
    vec2 position;
} vertexOut;

void main()
{
    vertexOut.position = position;
}
)shader",
loadGeometryShaderSrc[] = R"shader(
#version 150

uniform float fracDone;
const float barWidth = 0.1,
            barFrameDist = 0.02,
            frameWidth = 0.03;

layout(lines) in;
layout(triangle_strip, max_vertices=14) out;

in VertexData
{
    vec2 position;
} verticesIn[];

void main()
{
    vec2 bar[4],
         frameIn[4],
         frameOut[4];

    bar[0] = bar[2] = verticesIn[0].position;
    bar[1] = bar[3] = verticesIn[0].position + (verticesIn[1].position - verticesIn[0].position) * fracDone;

    bar[0].y += barWidth / 2;  bar[1].y += barWidth / 2;
    bar[2].y -= barWidth / 2;  bar[3].y -= barWidth / 2;

    frameIn[0].x = frameIn[3].x = verticesIn[0].position.x - barFrameDist;
    frameIn[1].x = frameIn[2].x = verticesIn[1].position.x + barFrameDist;
    frameIn[0].y = frameIn[1].y = bar[0].y + barFrameDist;
    frameIn[2].y = frameIn[3].y = bar[2].y - barFrameDist;

    frameOut[0] = frameIn[0] + vec2(-frameWidth, frameWidth);
    frameOut[1] = frameIn[1] + vec2(frameWidth, frameWidth);
    frameOut[2] = frameIn[2] + vec2(frameWidth, -frameWidth);
    frameOut[3] = frameIn[3] + vec2(-frameWidth, -frameWidth);


    int i;

    for (i = 0; i < 5; i++)
    {
        gl_Position = vec4(frameOut[i % 4], 0.0, 1.0);
        EmitVertex();
        gl_Position = vec4(frameIn[i % 4], 0.0, 1.0);
        EmitVertex();
    }
    EndPrimitive();

    for (i = 0; i < 4; i++)
    {
        gl_Position = vec4(bar[i], 0.0, 1.0);
        EmitVertex();
    }
    EndPrimitive();
}
)shader",
loadFragmentShaderSrc[] = R"shader(
#version 150

out vec4 fragColor;

void main()
{
    fragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
)shader";
LoadScene::LoadScene(InitializableScene *p)
: pLoaded(p)
{
    GLLock lockGL = App::Instance().GetGLLock();

    pLoaded->TellInit(mLoader);

    pProgram = App::Instance().GetGLManager()->AllocShaderProgram();
    VertexAttributeMap attributes;
    attributes["position"] = LOAD_POSITON_INDEX;
    ShaderLoadJob job(*pProgram, loadVertexShaderSrc,
                                 loadGeometryShaderSrc,
                                 loadFragmentShaderSrc, attributes);
    job.Run();


    const static GLfloat line[] = {-0.8f, 0.0f, 0.8f, 0.0f};

    pBuffer = App::Instance().GetGLManager()->AllocBuffer();

    glBindBuffer(GL_ARRAY_BUFFER, *pBuffer);
    CHECK_GL();

    glBufferData(GL_ARRAY_BUFFER, sizeof(line), line, GL_STATIC_DRAW);
    CHECK_GL();
}
LoadScene::~LoadScene(void)
{
    // When this scene ceases to exist, the loading thread must also stop.
    InterruptLoading();
}
void LoadScene::LoadThreadFunc(LoadScene *p)
{
    App::Instance().GetGLManager()->GarbageCollect();

    p->mLoader.Run();
}
void LoadScene::Start(void)
{
    mLoadThread = std::thread(LoadThreadFunc, this);
}
void LoadScene::Stop(void)
{
    InterruptLoading();
}
void LoadScene::InterruptLoading(void)
{
    mLoader.Clear();

    if (mLoadThread.joinable())
        mLoadThread.join();
}
void LoadScene::Render(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    CHECK_GL();

    glDisable(GL_CULL_FACE);
    CHECK_GL();

    glDisable(GL_DEPTH_TEST);
    CHECK_GL();

    glUseProgram(*pProgram);
    CHECK_GL();

    GLint fracDoneLocation = glGetUniformLocation(*pProgram, "fracDone");
    CHECK_GL();
    CHECK_UNIFORM_LOCATION(fracDoneLocation);

    LoadStats stats;
    mLoader.GetStats(stats);
    glUniform1f(fracDoneLocation, float(stats.countJobsAtStart - stats.countJobsRemain) / stats.countJobsAtStart);
    CHECK_GL();

    glBindBuffer(GL_ARRAY_BUFFER, *pBuffer);
    CHECK_GL();

    glEnableVertexAttribArray(LOAD_POSITON_INDEX);
    CHECK_GL();
    glVertexAttribPointer(LOAD_POSITON_INDEX, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GL_FLOAT), 0);
    CHECK_GL();

    glDrawArrays(GL_LINES, 0, 2);
    CHECK_GL();
}
void LoadScene::Update(void)
{
    LoadStats stats;
    mLoader.GetStats(stats);

    if (stats.countJobsRemain <= 0)
    {
        mLoadThread.join();
        App::Instance().SwitchScene(pLoaded);
    }
}

