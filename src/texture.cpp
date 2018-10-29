#include <cstdarg>
#include <cstdio>

#include <boost/format.hpp>

#include "texture.hpp"
#include "error.hpp"
#include "app.hpp"

PNGError::PNGError(const char *format, ...)
{
    va_list pArgs;
    va_start(pArgs, format);
    vsnprintf(buffer, ERRORBUFFER_SIZE, format, pArgs);
    va_end(pArgs);
}

void PNGErrorCallback(png_structp pPNG, png_const_charp message)
{
    throw PNGError(message);
}

void PNGReadCallback(png_structp pPNG, png_bytep outData, png_size_t length)
{
    std::istream *pIs = (std::istream *)png_get_io_ptr(pPNG);

    pIs->read((char *)outData, length);
    std::streamsize n = pIs->gcount();

    if (n < length)
        throw PNGError("only %u bytes read, %u requested", n, length);
}
PNGReader::PNGReader(void)
{
    pPNG = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL,
                                  (png_error_ptr)PNGErrorCallback,
                                  (png_error_ptr)NULL);
    if (pPNG == NULL)
        throw PNGError("error creating png struct");

    pInfo = png_create_info_struct(pPNG);
    if (pInfo == NULL)
        throw PNGError("error creating info struct");

    pEnd = png_create_info_struct(pPNG);
    if (pEnd == NULL)
        throw PNGError("error creating end info struct");
}
PNGReader::~PNGReader(void)
{
    png_destroy_read_struct(&pPNG, &pInfo, &pEnd);
}
PNGImage *PNGReader::ReadImage(std::istream &is)
{
    // Tell libpng that it must take its data from a callback function.
    png_set_read_fn(pPNG, &is, PNGReadCallback);

    // Read info from the header.
    png_read_info(pPNG, pInfo);

    // Get info about the data in the image.
    PNGImage *pImage = new PNGImage;
    int bitDepth;
    png_get_IHDR(pPNG, pInfo,
                 &(pImage->width), &(pImage->height),
                 &bitDepth, &(pImage->colorType),
                 NULL, NULL, NULL);
    if (bitDepth != 8)
        throw FormatError("PNG image bit depth is %d", bitDepth);
    else if (pImage->colorType != PNG_COLOR_TYPE_RGB
             && pImage->colorType != PNG_COLOR_TYPE_RGB_ALPHA)
        throw FormatError("PNG image color type is 0x%x", pImage->colorType);

    // Get bytes per row.
    png_uint_32 bytesPerRow = png_get_rowbytes(pPNG, pInfo);

    // Allocate memory to store pixel data in.
    pImage->data = (png_bytep)png_malloc(pPNG, pImage->height * bytesPerRow);
    if (pImage->data == NULL)
        throw PNGError("cannot allocate image data");

    // Make row pointers for reading.
    png_bytepp pRows = new png_bytep[pImage->height];

    // Place the row pointers correctly.
    for(png_uint_32 i = 0; i < pImage->height; i++)
        pRows[i] = (png_bytep)(pImage->data) + (pImage->height - i - 1) * bytesPerRow;

    try
    {
        // Read the entire image at once.
        png_read_image(pPNG, pRows);

        // Read the end of the file.
        png_read_end(pPNG, pEnd);
    }
    catch (...)
    {
        // If something goes wrong, at least release the memory.
        png_free(pPNG, pImage->data);
        delete pImage;
        delete[] pRows;

        std::rethrow_exception(std::current_exception());
    }

    // At this point, the row pointers aren't needed anymore.
    delete[] pRows;

    return pImage;
}
void PNGReader::FreeImage(PNGImage *pImage)
{
    png_free(pPNG, pImage->data);
    delete pImage;
};
int PNGImage::GetColorType(void) const
{
    return colorType;
}
void PNGImage::GetDimensions(png_uint_32 &w, png_uint_32 &h) const
{
    w = width;
    h = height;
}
const png_voidp PNGImage::GetData(void) const
{
    return data;
}
TextureError::TextureError(const char *format, ...)
{
    va_list pArgs;
    va_start(pArgs, format);
    vsnprintf(buffer, ERRORBUFFER_SIZE, format, pArgs);
    va_end(pArgs);
}
class FillGLTextureJob: public Job
{
    private:
        std::shared_ptr<const PNGImage> pImage;
        GLuint tex;
    public:
        FillGLTextureJob(std::shared_ptr<const PNGImage> p, const GLuint texture)
        :pImage(p), tex(texture)
        {
        }

        void Run(void)
        {
            png_uint_32 w, h;
            pImage->GetDimensions(w, h);

            glBindTexture(GL_TEXTURE_2D, tex);
            CHECK_GL();

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            CHECK_GL();
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            CHECK_GL();

            if (pImage->GetColorType() == PNG_COLOR_TYPE_RGB)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                             (GLsizei)w, (GLsizei)h, 0, GL_RGB,
                             GL_UNSIGNED_BYTE, pImage->GetData());

            else if (pImage->GetColorType() == PNG_COLOR_TYPE_RGBA)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                             (GLsizei)w, (GLsizei)h, 0, GL_RGBA,
                             GL_UNSIGNED_BYTE, pImage->GetData());
            CHECK_GL();

            glGenerateMipmap(GL_TEXTURE_2D);
            CHECK_GL();
        }
};
PNGTextureLoadJob::PNGTextureLoadJob(const std::string &loc, const GLuint tex): location(loc), texture(tex)
{
}
void PNGTextureLoadJob::Run(void)
{
    static thread_local PNGReader reader;
    boost::filesystem::ifstream is;

    boost::filesystem::path path = App::Instance().GetResourcePath((boost::format("textures/%1%.png") % location).str());
    is.open(path, std::ios::binary);

    if (!is.good())
        throw IOError("Error reading %s", path.string().c_str());

    std::shared_ptr<PNGImage> pImage(reader.ReadImage(is), [&reader](PNGImage *p) { reader.FreeImage(p); });

    is.close();

    App::Instance().PushGL(new FillGLTextureJob(pImage, texture));
}
