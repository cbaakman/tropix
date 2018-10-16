#ifndef ERROR_HPP
#define ERROR_HPP

#include <cstdarg>
#include <cstdio>
#include <exception>

#include <GL/glew.h>
#include <GL/gl.h>


#define ERRORBUFFER_SIZE 1024


class Error: public std::exception
{
    protected:
        char buffer[ERRORBUFFER_SIZE];
    public:
        const char *what(void) const noexcept;
};


class IOError: public Error
{
    public:
        IOError(const char *format, ...);
};


class ShutdownError: public Error
{
    public:
        ShutdownError(const char *format, ...);
};


class InitError: public Error
{
    public:
        InitError(const char *format, ...);
};


class ParseError: public Error
{
    public:
        ParseError(const char *format, ...);
};

class FormatError: public Error
{
    public:
        FormatError(const char *format, ...);
};

class GLError : public Error
{
    public:
        GLError(const char *format, ...);

        GLError(const GLenum err, const char *filename, const size_t lineNumber);
};

class GLUniformLocationError: public Error
{
    public:
        GLUniformLocationError(const char *filename, const size_t lineNumber);
};

void CheckGL(const char *filename, const size_t lineNumber);

#define CHECK_GL() CheckGL(__FILE__, __LINE__)

void CheckUniformLocation(GLint location, const char *filename, const size_t lineNumber);

#define CHECK_UNIFORM_LOCATION(loc) CheckUniformLocation(loc, __FILE__, __LINE__)

#endif  // ERROR_HPP
