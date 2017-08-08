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

#ifndef PROVIDER_H
#define PROVIDER_H

#include <memory>
#include <string>
#include "vec.h"


template <typename T>
class Provider
{
public:
    virtual T GetValue(void) const = 0;
};

typedef Provider<vec2> Pos2Provider;

class TranslatedPos2Provider : public Pos2Provider
{
private:
    vec2 mTranslation;
    const std::shared_ptr<Pos2Provider> pSource;
public:
    TranslatedPos2Provider(const std::shared_ptr<Pos2Provider>,
                           const vec2 &translation);

    vec2 GetValue(void) const;
};

typedef Provider<std::string> StringProvider;

class ConstantStringProvider : public StringProvider
{
private:
    std::string mString;
public:
    ConstantStringProvider(const std::string &);

    std::string GetValue(void) const;
};

template <typename T>
class ValueStringProvider : public StringProvider
{
private:
    T mInput;
protected:
    virtual std::string ToString(const T &value) const = 0;
public:
    std::string GetValue(void) const
    {
        return ToString(mInput);
    }
    void SetInput(const T &value)
    {
        mInput = value;
    }
    T GetInput(void)
    {
        return mInput;
    }
    ValueStringProvider(const T &value):
        mInput(value)
    {
    }
};

#endif  // PROVIDER_H
