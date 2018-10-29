#include <iostream>

#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/glm.hpp>
using namespace glm;

#include "load.hpp"
#include "error.hpp"
#include "app.hpp"
#include "shader.hpp"


void WorkThreadFunc(Queue &queue, ErrorManager &errorManager)
{
    std::shared_ptr<Job> pJob;
    while ((pJob = queue.Take()) != NULL)
    {
        try
        {
            pJob->Run();
        }
        catch (...)
        {
            errorManager.PushError(std::current_exception());
        }
    }
}
void WorkAllFrom(Queue &queue)
{
    std::shared_ptr<Job> pJob;
    while ((pJob = queue.Take()) != NULL)
        pJob->Run();
}
void ClearAllFrom(Queue &queue)
{
    while (queue.Take() != NULL);
}
size_t Queue::Size(void)
{
    std::scoped_lock lock(mtxJobs);
    return mJobs.size();
}
void Queue::Add(Job *pJob)
{
    Add(std::shared_ptr<Job>(pJob));
}
void Queue::Add(std::shared_ptr<Job> pJob)
{
    std::scoped_lock lock(mtxJobs);
    mJobs.push_back(pJob);
}
std::shared_ptr<Job> Queue::Take(void)
{
    std::shared_ptr<Job> pJob;
    std::scoped_lock lock(mtxJobs);
    if (mJobs.size() > 0)
    {
        pJob = mJobs.front();
        mJobs.pop_front();
        return pJob;
    }
    else
        return NULL;
}
void ErrorManager::PushError(const std::exception_ptr &e)
{
    std::scoped_lock lock(mtxErrors);
    mErrors.push_back(e);
}
void ErrorManager::ThrowAnyError(void)
{
    std::scoped_lock lock(mtxErrors);
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
    pLoaded->TellInit(mQueue);

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
void LoadScene::Start(void)
{
    // Try to free some GL memory before loading.
    App::Instance().GetGLManager()->GarbageCollect();

    Config config;
    App::Instance().GetConfig(config);

    countStartJobs = mQueue.Size();

    mConcurrentManager.Start(config.loadConcurrency, WorkThreadFunc, std::ref(mQueue), std::ref(mErrorManager));
}
void LoadScene::Stop(void)
{
    InterruptLoading();
}
void LoadScene::InterruptLoading(void)
{
    ClearAllFrom(mQueue);
    mConcurrentManager.JoinAll();
    mErrorManager.ThrowAnyError();
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

    size_t countRemainingJobs = mQueue.Size();
    glUniform1f(fracDoneLocation, float(countStartJobs - countRemainingJobs) / countStartJobs);
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
    if (mQueue.Size() <= 0)
    {
        mConcurrentManager.JoinAll();
        mErrorManager.ThrowAnyError();

        App::Instance().SwitchScene(pLoaded);
    }
}

