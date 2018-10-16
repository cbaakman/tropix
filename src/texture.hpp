#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <iostream>
#include <string>

#include <GL/glew.h>
#include <GL/gl.h>
#include <png.h>

#include "error.hpp"
#include "load.hpp"

class PNGError: public Error
{
    public:
        PNGError(const char *format, ...);
};

class TextureError: public Error
{
    public:
        TextureError(const char *format, ...);
};

class PNGImage
{
    private:
        png_voidp data;
        png_uint_32 width, height;
        int colorType;
    public:
        int GetColorType(void) const;  // Either PNG_COLOR_TYPE_RGB or PNG_COLOR_TYPE_RGB_ALPHA.
        void GetDimensions(png_uint_32 &w, png_uint_32 &h) const;
        const png_voidp GetData(void) const;

    friend class PNGReader;
};


/**
 *  Not thread safe.
 */
class PNGReader
{
    private:
        png_struct *pPNG;
        png_info *pInfo,
                 *pEnd;
    public:
        PNGReader(void);
        ~PNGReader(void);

        PNGImage *ReadImage(std::istream &);
        void FreeImage(PNGImage *);
};


void FillGLTexture(const PNGImage *, GLuint texture);


class PNGTextureLoadJob: public LoadJob
{
    private:
        std::string location;
        GLuint texture;
    public:
        PNGTextureLoadJob(const std::string &location, const GLuint texture);
        void Run(void);
};


#endif  // TEXTURE_HPP
