#include <iostream>

#include "alloc.hpp"
#include "error.hpp"
#include "app.hpp"


void GLRef::Increment(void)
{
    if (pObj != NULL)
        pObj->refCount++;
}
void GLRef::Decrement(void)
{
    if (pObj != NULL)
    {
        pObj->refCount--;

        if (pObj->refCount <= 0)
            pObj = NULL;
    }
}

GLRef::GLRef(void): pObj(NULL)
{
}
GLRef::GLRef(GLObj *p): pObj(p)
{
    Increment();
}
GLRef::GLRef(const GLRef &other): pObj(other.pObj)
{
    Increment();
}
GLRef::~GLRef(void)
{
    Decrement();
}
void GLRef::operator=(const GLRef &other)
{
    Decrement();
    pObj = other.pObj;
    Increment();
}
GLuint GLRef::operator*(void) const
{
    if (pObj != NULL)
        return pObj->handle;
    else
        return 0;
}
GLObj::GLObj(GLDeleter d): deleter(d), refCount(0), handle(0)
{
}
GLManager::GLManager()
{
}
GLObj *GLManager::AddObj(GLDeleter d)
{
    mObjs.push_back(GLObj(d));
    return &mObjs.back();
}
GLRef GLManager::AllocTexture(void)
{
    GLLock scopedLock = App::Instance().GetGLLock();

    GLObj *pObj = AddObj([](GLuint texture) { glDeleteTextures(1, &texture); CHECK_GL(); });
    glGenTextures(1, &(pObj->handle));
    CHECK_GL();

    if (pObj->handle == 0)
        throw GLError("No texture was allocated.");

    return GLRef(pObj);
}
GLRef GLManager::AllocShaderProgram(void)
{
    GLLock scopedLock = App::Instance().GetGLLock();

    GLObj *pObj = AddObj([](GLuint program) { glDeleteProgram(program); CHECK_GL(); });
    pObj->handle = glCreateProgram();
    CHECK_GL();

    if (pObj->handle == 0)
        throw GLError("No shader program was allocated.");

    return GLRef(pObj);
}
GLRef GLManager::AllocBuffer(void)
{
    GLLock scopedLock = App::Instance().GetGLLock();

    GLObj *pObj = AddObj([](GLuint buffer) { glDeleteBuffers(1, &buffer); CHECK_GL(); });
    glGenBuffers(1, &(pObj->handle));
    CHECK_GL();

    if (pObj->handle == 0)
        throw GLError("No buffer was allocated.");

    return GLRef(pObj);
}
void GLManager::GarbageCollect(void)
{
    auto it = mObjs.begin();
    while (it != mObjs.end())
    {
        if (it->refCount <= 0)
        {
            GLLock scopedLock = App::Instance().GetGLLock();

            it->deleter(it->handle);
            it = mObjs.erase(it);
        }
        else
            it++;
    }
}
void GLManager::DestroyAll(void)
{
    GLLock scopedLock = App::Instance().GetGLLock();

    for (GLObj &obj : mObjs)
    {
        obj.deleter(obj.handle);
    }
    mObjs.clear();
}
GLScoped::GLScoped(GLuint h, GLDeleter d): handle(h), deleter(d)
{
}
GLScoped::~GLScoped(void)
{
    deleter(handle);
}
GLuint GLScoped::operator*(void) const
{
    return handle;
}
