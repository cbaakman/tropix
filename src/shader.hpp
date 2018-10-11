#ifndef SHADER_H
#define SHADER_H

#include <map>

#include <GL/glew.h>
#include <GL/gl.h>

#include "error.hpp"
#include "load.hpp"


typedef std::map<std::string, size_t> VertexAttributeMap;


GLuint MakeShader(const std::string &src, const GLenum type);
void LinkShaders(const GLuint shaderProgram, const GLuint vertexShader, const GLuint fragmentShader, const VertexAttributeMap &);

class ShaderError: public Error
{
    public:
        ShaderError(const char *format, ...);
};


class ShaderLoadJob: public LoadJob
{
    private:
        std::string vertexSrc, fragmentSrc;
        VertexAttributeMap attributes;
        GLuint program;
    public:
        ShaderLoadJob(GLuint program,
                      const std::string &vertexSrc,
                      const std::string &fragmentSrc,
                      const VertexAttributeMap &attributes);

        void Run(void);
};

#endif  // SHADER_H