#ifndef TEXT_HPP
#define TEXT_HPP

#include <unordered_map>

#include <glm/glm.hpp>
using namespace glm;
#include <text-gl/text.h>
#include <boost/filesystem.hpp>

#include "alloc.hpp"
#include "load.hpp"


class TextRenderer: public TextGL::GLTextLeftToRightIterator, Initializable
{
    private:
        GLRef pBuffer,
              pProgram;

        mat4 projection;

        void OnGlyph(const TextGL::UTF8Char, const TextGL::GlyphQuad &, const TextGL::TextSelectionDetails &);
    public:
        void SetProjection(const mat4 &);

        void TellInit(Loader &);
};

enum FontStyleChoice
{
    FONT_SMALLBLACK
};

class FontManager
{
    private:
        TextGL::FontData mFontData;

        std::unordered_map<FontStyleChoice, TextGL::GLTextureFont *> mFonts;

        TextGL::GLTextureFont *InitFont(const FontStyleChoice, const TextGL::FontStyle &style);
    public:
        const TextGL::GLTextureFont *GetFont(const FontStyleChoice);

        void InitAll(const boost::filesystem::path &);
        void DestroyAll(void);
};

#endif  // TEXT_HPP
