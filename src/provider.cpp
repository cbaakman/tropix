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

#include "provider.h"


ConstantStringProvider::ConstantStringProvider(const std::string &s):
    mString(s)
{
}
std::string ConstantStringProvider::GetValue(void) const
{
    return mString;
}
TranslatedPos2Provider::TranslatedPos2Provider(const std::shared_ptr<Pos2Provider> pSrc,
                                               const vec2 &translation):
        pSource(pSrc),
        mTranslation(translation)
{
}
vec2 TranslatedPos2Provider::GetValue(void) const
{
    return pSource->GetValue() + mTranslation;
}
