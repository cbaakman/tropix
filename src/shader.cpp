#include <iostream>
#include <memory>
#include <list>

#include "shader.hpp"
#include "app.hpp"
#include "alloc.hpp"



std::string getShaderTypeName(const GLenum type)
{
    switch (type)
    {
    case GL_COMPUTE_SHADER:
        return "GL_COMPUTE_SHADER";
    case GL_VERTEX_SHADER:
        return "GL_VERTEX_SHADER";
    case GL_TESS_CONTROL_SHADER:
        return "GL_TESS_CONTROL_SHADER";
    case GL_TESS_EVALUATION_SHADER:
        return "GL_TESS_EVALUATION_SHADER";
    case GL_GEOMETRY_SHADER:
        return "GL_GEOMETRY_SHADER";
    case GL_FRAGMENT_SHADER:
        return "GL_FRAGMENT_SHADER";
    default:
        throw GLError("unknown shader type");
    };
}

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

        std::unique_ptr<char[]> errorString(new char[logLength + 1]);
        glGetShaderInfoLog(shader, logLength, NULL, errorString.get());
        CHECK_GL();

        glDeleteShader(shader);
        CHECK_GL();

        throw ShaderError("error while compiling %s: %s", getShaderTypeName(type).c_str(), errorString.get());
    }

    return shader;
}
void LinkShaders(const GLuint program,
                 const std::list<GLuint> &shaders,
                 const VertexAttributeMap &vertexAttribLocations)
{
    GLint result;
    int logLength;

    for (GLuint shader : shaders)
    {
        glAttachShader(program, shader);
        CHECK_GL();
    }

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

        std::unique_ptr<char[]> errorString(new char[logLength + 1]);
        glGetProgramInfoLog(program, logLength, NULL, errorString.get());
        CHECK_GL();

        throw ShaderError("error while linking shaders: %s", errorString.get());
    }
}
void LinkShaders(const GLuint program,
                 const GLuint vertexShader,
                 const GLuint fragmentShader, const VertexAttributeMap &attributes)
{
    std::list<GLuint> shaders;
    shaders.push_back(vertexShader);
    shaders.push_back(fragmentShader);
    LinkShaders(program, shaders, attributes);
}
void LinkShaders(const GLuint program,
                 const GLuint vertexShader,
                 const GLuint geometryShader,
                 const GLuint fragmentShader, const VertexAttributeMap &attributes)
{
    std::list<GLuint> shaders;
    shaders.push_back(vertexShader);
    shaders.push_back(geometryShader);
    shaders.push_back(fragmentShader);
    LinkShaders(program, shaders, attributes);
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
: program(prg), vertexSrc(vSrc), geometrySrc(""), fragmentSrc(fSrc), attributes(attrs)
{
}
ShaderLoadJob::ShaderLoadJob(GLuint prg,
                             const std::string &vSrc,
                             const std::string &gSrc,
                             const std::string &fSrc,
                             const VertexAttributeMap &attrs)
: program(prg), vertexSrc(vSrc), geometrySrc(gSrc), fragmentSrc(fSrc), attributes(attrs)
{
}
void DeleteShader(GLuint shader)
{
    glDeleteShader(shader);
    CHECK_GL();
}
void ShaderLoadJob::Run(void)
{
    if (geometrySrc.length() > 0)
    {
        GLScoped scopedVertexShader(MakeShader(vertexSrc, GL_VERTEX_SHADER), DeleteShader),
                 scopedGeometryShader(MakeShader(geometrySrc, GL_GEOMETRY_SHADER), DeleteShader),
                 scopedFragmentShader(MakeShader(fragmentSrc, GL_FRAGMENT_SHADER), DeleteShader);

        LinkShaders(program, *scopedVertexShader,
                             *scopedGeometryShader,
                             *scopedFragmentShader, attributes);
    }
    else
    {
        GLScoped scopedVertexShader(MakeShader(vertexSrc, GL_VERTEX_SHADER), DeleteShader),
                 scopedFragmentShader(MakeShader(fragmentSrc, GL_FRAGMENT_SHADER), DeleteShader);

        LinkShaders(program, *scopedVertexShader, *scopedFragmentShader, attributes);
    }
}
