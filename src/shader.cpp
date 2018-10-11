#include <iostream>

#include "shader.hpp"
#include "app.hpp"
#include "alloc.hpp"


GLuint MakeShader(const std::string &source, const GLenum type)
{
    GLint result;
    int logLength;

    GLuint shader = glCreateShader(type);
    CHECK_GL();

    const char *pSource = source.c_str();
    glShaderSource(shader, 1, &pSource, NULL);
    CHECK_GL();

    glCompileShader(shader);
    CHECK_GL();

    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    CHECK_GL();

    if (result != GL_TRUE)
    {
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        CHECK_GL();

        char *errorString = new char[logLength + 1];
        glGetShaderInfoLog(shader, logLength, NULL, errorString);
        CHECK_GL();

        ShaderError error("error while compiling shader: %s", errorString);
        delete[] errorString;

        glDeleteShader(shader);
        CHECK_GL();

        throw error;
    }

    return shader;
}

void LinkShaders(const GLuint program,
                 const GLuint vertexShader,
                 const GLuint fragmentShader,
                 const VertexAttributeMap &vertexAttribLocations)
{
    GLint result;
    int logLength;

    glAttachShader(program, vertexShader);
    CHECK_GL();

    glAttachShader(program, fragmentShader);
    CHECK_GL();

    for (const auto &pair : vertexAttribLocations)
    {
        glBindAttribLocation(program, std::get<1>(pair),
                                      std::get<0>(pair).c_str());
        CHECK_GL();
    }

    glLinkProgram(program);
    CHECK_GL();

    glGetProgramiv(program, GL_LINK_STATUS, &result);
    CHECK_GL();

    if (result != GL_TRUE)
    {
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        CHECK_GL();

        char *errorString = new char[logLength + 1];
        glGetProgramInfoLog(program, logLength, NULL, errorString);
        CHECK_GL();

        ShaderError error("error while linking shader: %s", errorString);
        delete[] errorString;

        throw error;
    }
}

ShaderError::ShaderError(const char *format, ...)
{
    va_list pArgs;
    va_start(pArgs, format);
    vsnprintf(buffer, ERRORBUFFER_SIZE, format, pArgs);
    va_end(pArgs);
}

ShaderLoadJob::ShaderLoadJob(GLuint prg,
                             const std::string &vSrc,
                             const std::string &fSrc,
                             const VertexAttributeMap &attrs)
: program(prg), vertexSrc(vSrc), fragmentSrc(fSrc), attributes(attrs)
{
}
void DeleteShader(GLuint shader)
{
    glDeleteShader(shader);
    CHECK_GL();
}
void ShaderLoadJob::Run(void)
{
    GLLock scopedLock = App::Instance().GetGLLock();

    GLScoped scopedVertexShader(MakeShader(vertexSrc, GL_VERTEX_SHADER), DeleteShader),
             scopedFragmentShader(MakeShader(fragmentSrc, GL_FRAGMENT_SHADER), DeleteShader);

    LinkShaders(program, *scopedVertexShader, *scopedFragmentShader, attributes);
}
