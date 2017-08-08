/* Copyright (C) 2017 Coos Baakman
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.
  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:
  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <algorithm>

#include "matrix.h"
#include "exception.h"
#include "fade-screen.h"
#include "sprite.h"
#include "client.h"
#include "log.h"


#define TRANSPBLUE_ALPHAMAX 0.5f
#define TRANSPBLUE_FADESPEED 1.0f
TransparentBlueScreenFader::TransparentBlueScreenFader(Client *pCl):
    pClient(pCl),
    bFadeOut(true),
    mAlpha(0.0)
{
}
void TransparentBlueScreenFader::Update(const float dt)
{
    if (bFadeOut)  // turning transparent
        mAlpha = std::max(0.0f, mAlpha - TRANSPBLUE_FADESPEED * dt);
    else  // fading in, turning opaque
        mAlpha = std::min(TRANSPBLUE_ALPHAMAX, mAlpha + TRANSPBLUE_FADESPEED * dt);
}
void TransparentBlueScreenFader::Render(void)
{
    if (mAlpha <= 0.0f)
        return;

    ClientSettings settings;
    pClient->GetSettings(settings);

    glViewport(0, 0, settings.display.resolution.width,
                     settings.display.resolution.height);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(&MatOrtho(0, settings.display.resolution.width,
                            0, settings.display.resolution.height,
                            -1.0f, 1.0f));

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(&MatID());

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING);

    glColor4f(0.5f, 0.5f, 1.0f, mAlpha);

    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(settings.display.resolution.width, 0.0f);
    glVertex2f(settings.display.resolution.width, settings.display.resolution.height);
    glVertex2f(0.0f, settings.display.resolution.height);
    glEnd();
}
void TransparentBlueScreenFader::StartFadeOut(void)
{
    bFadeOut = true;
}
void TransparentBlueScreenFader::StartFadeIn(void)
{
    bFadeOut = false;
}
bool TransparentBlueScreenFader::Ready(void)
{
    return bFadeOut && mAlpha <= 0.0f ||
           !bFadeOut && mAlpha >= TRANSPBLUE_ALPHAMAX;
}
#define FRAMEBUFFER_WIDTH 1024
#define MASK_MAXSIZE 1500.0f
#define MASK_MINSIZE 0.0f
#define MASK_RESIZESPEED 3000.0f
#define MASK_ROT_SIZE_RATIO 0.05f
StarfishScreenFader::StarfishScreenFader(Client *pCl):
    pClient(pCl),
    bFadeOut(false),
    mMaskSize(MASK_MAXSIZE),
    rMaskTexture(pClient->GetResourceManager()->GetTexture("starfish-mask"))
{
    glGenFramebuffers(1, &mFramebuffer);
    glGenTextures(1, &mFramebufferTexture);

    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    glBindTexture(GL_TEXTURE_2D, mFramebufferTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, FRAMEBUFFER_WIDTH, FRAMEBUFFER_WIDTH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mFramebufferTexture, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    glBindFramebuffer(GL_FRAMEBUFFER, NULL);

    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        throw GLFramebufferException("starfishfader framebuffer init", status);
    }
}
StarfishScreenFader::~StarfishScreenFader(void)
{
    glDeleteTextures(1, &mFramebufferTexture);
    glDeleteFramebuffers(1, &mFramebuffer);
}
void StarfishScreenFader::StartFadeOut(void)
{
    bFadeOut = true;
}
void StarfishScreenFader::StartFadeIn(void)
{
    bFadeOut = false;
}
bool StarfishScreenFader::Ready(void)
{
    return bFadeOut && mMaskSize <= MASK_MINSIZE ||
           !bFadeOut && mMaskSize >= MASK_MAXSIZE;
}
void StarfishScreenFader::Update(const float dt)
{
    if (bFadeOut)
        mMaskSize = std::max(MASK_MINSIZE, mMaskSize - MASK_RESIZESPEED * dt);
    else  // fade in
        mMaskSize = std::min(MASK_MAXSIZE, mMaskSize + MASK_RESIZESPEED * dt);
}
void StarfishScreenFader::Render(void)
{
    if (mMaskSize >= MASK_MAXSIZE)
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    glViewport(0, 0, FRAMEBUFFER_WIDTH, FRAMEBUFFER_WIDTH);

    glDisable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (mMaskSize > 0.0f)
    {
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(&MatOrtho(-FRAMEBUFFER_WIDTH / 2, FRAMEBUFFER_WIDTH / 2,
                                -FRAMEBUFFER_WIDTH / 2, FRAMEBUFFER_WIDTH / 2,
                                -1.0f, 1.0f));
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(&MatID());

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, rMaskTexture->tex);

        glRotatef(MASK_ROT_SIZE_RATIO * mMaskSize, 0.0f, 0.0f, 1.0f);

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(-mMaskSize, -mMaskSize);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(mMaskSize, -mMaskSize);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(mMaskSize, mMaskSize);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(-mMaskSize, mMaskSize);
        glEnd();

        glRotatef(-MASK_ROT_SIZE_RATIO * mMaskSize, 0.0f, 0.0f, 1.0f);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, NULL);

    ClientSettings settings;
    pClient->GetSettings(settings);

    glViewport(0, 0, settings.display.resolution.width,
                     settings.display.resolution.height);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(&MatOrtho(0, settings.display.resolution.width,
                            0, settings.display.resolution.height,
                            -1.0f, 1.0f));

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(&MatID());

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    glDisable(GL_LIGHTING);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBindTexture(GL_TEXTURE_2D, mFramebufferTexture);

    GLfloat x1, y1, x2, y2;
    if (settings.display.resolution.width > settings.display.resolution.height)
    {
        x1 = 0.0f;
        y1 = 0.0f - (settings.display.resolution.width - settings.display.resolution.height) / 2;
        x2 = x1 + settings.display.resolution.width;
        y2 = y1 + settings.display.resolution.width;
    }
    else  // settings.display.resolution.height > settings.display.resolution.width
    {
        x1 = 0.0f - (settings.display.resolution.height - settings.display.resolution.width) / 2;
        y1 = 0.0f;
        x2 = x1 + settings.display.resolution.height;
        y2 = y1 + settings.display.resolution.height;
    }

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(x1, y1);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(x2, y1);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(x2, y2);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(x1, y2);
    glEnd();
}
