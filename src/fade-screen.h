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

#ifndef FADE_SCREEN_H
#define FADE_SCREEN_H

#include <GL/glew.h>
#include <GL/gl.h>

#include "resource.h"


class ScreenFader
{
public:
    virtual void Update(const float dt) = 0;

    virtual void StartFadeOut(void) = 0;
    virtual void StartFadeIn(void) = 0;

    virtual bool Ready(void) = 0;

    virtual void Render(void) = 0;
};

class Client;

class StarfishScreenFader : public ScreenFader
{
private:
    Client *pClient;

    float mMaskSize;
    bool bFadeOut;

    GLuint mFramebuffer,
           mFramebufferTexture;

    ResourceReference<Texture> rMaskTexture;
public:
    StarfishScreenFader(Client *);
    ~StarfishScreenFader(void);

    void Update(const float dt);

    void StartFadeOut(void);
    void StartFadeIn(void);

    bool Ready(void);

    void Render(void);
};

class TransparentBlueScreenFader : public ScreenFader
{
private:
    Client *pClient;

    float mAlpha;
    bool bFadeOut;
public:
    TransparentBlueScreenFader(Client *);

    void Update(const float dt);

    void StartFadeOut(void);
    void StartFadeIn(void);

    bool Ready(void);
    void Render(void);
};

#endif  // FADE_SCREEN_H
