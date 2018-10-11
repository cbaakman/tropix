#include "error.hpp"


const char *Error::what(void) const noexcept
{
    return buffer;
}
IOError::IOError(const char *format, ...)
{
    va_list pArgs;
    va_start(pArgs, format);
    vsnprintf(buffer, ERRORBUFFER_SIZE, format, pArgs);
    va_end(pArgs);
}
FormatError::FormatError(const char *format, ...)
{
    va_list pArgs;
    va_start(pArgs, format);
    vsnprintf(buffer, ERRORBUFFER_SIZE, format, pArgs);
    va_end(pArgs);
}
ShutdownError::ShutdownError(const char *format, ...)
{
    va_list pArgs;
    va_start(pArgs, format);
    vsnprintf(buffer, ERRORBUFFER_SIZE, format, pArgs);
    va_end(pArgs);
}
InitError::InitError(const char *format, ...)
{
    va_list pArgs;
    va_start(pArgs, format);
    vsnprintf(buffer, ERRORBUFFER_SIZE, format, pArgs);
    va_end(pArgs);
}
ParseError::ParseError(const char *format, ...)
{
    va_list pArgs;
    va_start(pArgs, format);
    vsnprintf(buffer, ERRORBUFFER_SIZE, format, pArgs);
    va_end(pArgs);
}
GLError::GLError(const char *format, ...)
{
    va_list pArgs;
    va_start(pArgs, format);
    vsnprintf(buffer, ERRORBUFFER_SIZE, format, pArgs);
    va_end(pArgs);
}
GLError::GLError(const GLenum err, const char *filename, const size_t lineNumber)
{
    switch (err)
    {
    case GL_NO_ERROR:
        snprintf(buffer, ERRORBUFFER_SIZE, "GL_NO_ERROR at %s line %u", filename, lineNumber);
        break;
    case GL_INVALID_ENUM:
        snprintf(buffer, ERRORBUFFER_SIZE, "GL_INVALID_ENUM at %s line %u", filename, lineNumber);
        break;
    case GL_INVALID_VALUE:
        snprintf(buffer, ERRORBUFFER_SIZE, "GL_INVALID_VALUE at %s line %u", filename, lineNumber);
        break;
    case GL_INVALID_OPERATION:
        snprintf(buffer, ERRORBUFFER_SIZE, "GL_INVALID_OPERATION at %s line %u", filename, lineNumber);
        break;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        snprintf(buffer, ERRORBUFFER_SIZE, "GL_INVALID_FRAMEBUFFER_OPERATION at %s line %u", filename, lineNumber);
        break;
    case GL_OUT_OF_MEMORY:
        snprintf(buffer, ERRORBUFFER_SIZE, "GL_OUT_OF_MEMORY at %s line %u", filename, lineNumber);
        break;
    case GL_STACK_UNDERFLOW:
        snprintf(buffer, ERRORBUFFER_SIZE, "GL_STACK_UNDERFLOW at %s line %u", filename, lineNumber);
        break;
    case GL_STACK_OVERFLOW:
        snprintf(buffer, ERRORBUFFER_SIZE, "GL_STACK_OVERFLOW at %s line %u", filename, lineNumber);
        break;
    default:
        snprintf(buffer, ERRORBUFFER_SIZE, "glGetError: 0x%x at at %s line %u", err, filename, lineNumber);
        break;
    }
}
GLUniformLocationError::GLUniformLocationError(const char *filename, const size_t lineNumber)
{
    snprintf(buffer, ERRORBUFFER_SIZE, "Uniform location error at %s line %u", filename, lineNumber);
}
void CheckGL(const char *filename, const size_t lineNumber)
{
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
        throw GLError(err, filename, lineNumber);
}
void CheckUniformLocation(GLint location, const char *filename, const size_t lineNumber)
{
    if (location < 0)
        throw GLUniformLocationError(filename, lineNumber);
}
