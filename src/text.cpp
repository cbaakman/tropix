#include <memory>

#include <glm/gtc/type_ptr.hpp>

#include "text.hpp"
#include "app.hpp"
#include "shader.hpp"
#include "error.hpp"


#define GLYPHVERTEX_POSITION_INDEX 0
#define GLYPHVERTEX_TEXCOORDS_INDEX 1

const char glyphVertexShaderSrc[] = R"shader(
#version 150

in vec2 position;
in vec2 texCoords;

out VertexData
{
    vec2 texCoords;
} vertexOut;

uniform mat4 projectionMatrix;

void main()
{
    gl_Position = projectionMatrix * vec4(position, 0.0, 1.0);
    vertexOut.texCoords = texCoords;
}
)shader",
         glyphFragmentShaderSrc[] = R"shader(
#version 150

uniform sampler2D tex;

in VertexData
{
    vec2 texCoords;
} vertexIn;

out vec4 fragColor;

void main()
{
    fragColor = texture(tex, vertexIn.texCoords);
}
)shader";

void TextRenderer::TellInit(Loader &loader)
{
    pBuffer = App::Instance().GetGLManager()->AllocBuffer();
    pProgram = App::Instance().GetGLManager()->AllocShaderProgram();

    glBindBuffer(GL_ARRAY_BUFFER, *pBuffer);
    CHECK_GL();

    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(TextGL::GlyphVertex), NULL, GL_DYNAMIC_DRAW);
    CHECK_GL();

    VertexAttributeMap attributes;
    attributes["position"] = GLYPHVERTEX_POSITION_INDEX;
    attributes["texCoords"] = GLYPHVERTEX_TEXCOORDS_INDEX;
    App::Instance().PushGL(new ShaderLoadJob(*pProgram, glyphVertexShaderSrc, glyphFragmentShaderSrc, attributes));
}
void TextRenderer::SetProjection(const mat4 &m)
{
    projection = m;
}
void TextRenderer::OnGlyph(const TextGL::UTF8Char, const TextGL::GlyphQuad &quad, const TextGL::TextSelectionDetails &)
{
    glBindBuffer(GL_ARRAY_BUFFER, *pBuffer);
    CHECK_GL();

    glEnableVertexAttribArray(GLYPHVERTEX_POSITION_INDEX);
    CHECK_GL();
    glEnableVertexAttribArray(GLYPHVERTEX_TEXCOORDS_INDEX);
    CHECK_GL();
    glVertexAttribPointer(GLYPHVERTEX_POSITION_INDEX, 2, GL_FLOAT, GL_FALSE, sizeof(TextGL::GlyphVertex), 0);
    CHECK_GL();
    glVertexAttribPointer(GLYPHVERTEX_TEXCOORDS_INDEX, 2, GL_FLOAT, GL_FALSE, sizeof(TextGL::GlyphVertex), (GLvoid *)(2 * sizeof(GLfloat)));
    CHECK_GL();

    // Fill the buffer.

    TextGL::GlyphVertex *pVertexBuffer = (TextGL::GlyphVertex *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    CHECK_GL();

    pVertexBuffer[0] = quad.vertices[0];
    pVertexBuffer[1] = quad.vertices[1];
    pVertexBuffer[2] = quad.vertices[3];
    pVertexBuffer[3] = quad.vertices[2];

    glUnmapBuffer(GL_ARRAY_BUFFER);
    CHECK_GL();

    // Draw the buffer.

    glDisable(GL_DEPTH_TEST);
    CHECK_GL();

    glEnable(GL_BLEND);
    CHECK_GL();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    CHECK_GL();

    glUseProgram(*pProgram);
    CHECK_GL();

    GLint location = glGetUniformLocation(*pProgram, "projectionMatrix");
    CHECK_GL();
    CHECK_UNIFORM_LOCATION(location);

    glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(projection));
    CHECK_GL();

    glActiveTexture(GL_TEXTURE0);
    CHECK_GL();

    glBindTexture(GL_TEXTURE_2D, quad.texture);
    CHECK_GL();

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    CHECK_GL();

    glDisableVertexAttribArray(GLYPHVERTEX_POSITION_INDEX);
    CHECK_GL();
    glDisableVertexAttribArray(GLYPHVERTEX_TEXCOORDS_INDEX);
    CHECK_GL();
}

TextGL::GLTextureFont *FontManager::InitFont(const FontStyleChoice choice, const TextGL::FontStyle &style)
{
    if (mFonts.find(choice) == mFonts.end())
    {
        std::unique_ptr<TextGL::ImageFont, void (*)(TextGL::ImageFont *)> pImageFont(TextGL::MakeImageFont(mFontData, style), TextGL::DestroyImageFont);

        mFonts.emplace(choice, TextGL::MakeGLTextureFont(pImageFont.get()));
    }

    return mFonts.at(choice);
}
const TextGL::GLTextureFont *FontManager::GetFont(const FontStyleChoice choice)
{
    return mFonts.at(choice);
}
void FontManager::InitAll(const boost::filesystem::path &path)
{
    boost::filesystem::ifstream is;
    is.open(path);

    if (!is.good())
        throw IOError("Cannot read %s", path.string().c_str());

    TextGL::ParseSVGFontData(is, mFontData);

    TextGL::FontStyle style;
    style.size = 16.0;
    style.strokeWidth = 0.0;
    style.fillColor = {0.0, 0.0, 0.0, 1.0};
    InitFont(FONT_SMALLBLACK, style);
}
void FontManager::DestroyAll(void)
{
    for (auto &pair : mFonts)
        TextGL::DestroyGLTextureFont(pair.second);
}
