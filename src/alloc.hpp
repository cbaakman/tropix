#ifndef ALLOC_HPP
#define ALLOC_HPP

#include <functional>
#include <list>

#include <GL/glew.h>
#include <GL/gl.h>


typedef std::function<void (GLuint)> GLDeleter;


class GLObj
{
    private:
        GLuint handle;
        size_t refCount;

        GLDeleter deleter;
    public:
        GLObj(GLDeleter);

    friend class GLRef;
    friend class GLManager;
};


class GLManager;


class GLRef
{
    private:
        GLObj *pObj;

        void Increment(void);
        void Decrement(void);

        GLRef(GLObj *);
    public:
        GLRef(void);
        GLRef(const GLRef &);
        ~GLRef(void);
        void operator=(const GLRef &);
        GLuint operator*(void) const;

    friend class GLManager;
};


class App;


class GLManager
{
    private:
        std::list<GLObj> mObjs;

        GLObj *AddObj(GLDeleter);
    public:
        GLManager();

        GLRef AllocTexture(void);
        GLRef AllocShaderProgram(void);
        GLRef AllocBuffer(void);
        GLRef AllocFrameBuffer(void);

        void GarbageCollect(void);
        void DestroyAll(void);

    friend class GLRef;
};


class GLScoped
{
    private:
        GLuint handle;
        GLDeleter deleter;

        GLScoped(const GLScoped &) = delete;
        void operator=(const GLScoped &) = delete;
    public:
        GLuint operator*(void) const;

        GLScoped(GLuint, GLDeleter);
        ~GLScoped(void);
};


#endif  // ALLOC_HPP
