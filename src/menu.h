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

#ifndef MENU_H
#define MENU_H

#include <list>
#include <vector>
#include <memory>

#include "event.h"
#include "client.h"
#include "settings.h"
#include "log.h"
#include "provider.h"


class LanguageStringProvider : public StringProvider
{
private:
    Client *pClient;
    std::string mKey;
public:
    LanguageStringProvider(Client *, const std::string &key);

    std::string GetValue(void) const;
};

class Menu;

class MenuElement : public EventProcessor
{
private:
    bool bActive,
         bDisabled;
protected:
    virtual void OnLeave(void) {};
    virtual void OnEnter(void) {};
public:
    /**
     * true when all animation is done.
     */
    virtual bool Ready(void) const = 0;

    virtual bool MouseOver(void) const { return false; };

    void Leave(void);
    void Enter(void);
    bool IsActive(void) const;
    bool IsDisabled(void) const;

    virtual void Render(void) = 0;
    virtual void Update(const float dt) {}

    MenuElement(void);

    void Disable(void);
    void Enable(void);
};

typedef std::function<void (void)> MenuActionFunc;

class Menu : public EventProcessor
{
private:
    bool bActive,
         bElementsLeaving;
    std::list <MenuElement *> mElements;

    MenuActionFunc mLeaveAction;
public:
    Menu(void);
    ~Menu(void);

    bool OnEvent(const SDL_Event &);

    void AddElement(MenuElement *);
    bool IsActive(void) const;

    bool Ready(void) const;
    void Leave(MenuActionFunc = NULL);
    void Enter(void);

    void DisableAll(void);
    void EnableAll(void);

    void Render(void);
    void Update(const float dt);
};

class MenuButton;
typedef std::function <void (MenuButton *)> ButtonActionFunc;

class MenuButton : public MenuElement
{
private:
    bool bClicked,
         bTouched;
    ButtonActionFunc mAction;
protected:
    Client *pClient;
protected:
    virtual void OnTouch(void) {}
    virtual void OnClick(void) {}
    virtual bool Clickable(void) = 0;

    bool OnMouseWheel(const SDL_MouseWheelEvent &);
    bool OnMouseMotion(const SDL_MouseMotionEvent &);
    bool OnMouseButtonDown(const SDL_MouseButtonEvent &);
public:
    MenuButton(Client *, ButtonActionFunc);

    bool IsClicked(void);
    void UnClick(void);
    void SetClicked(void);

    virtual void Update(const float dt);
    virtual void OnLeave(void);
};

class MenuLabel : public MenuElement
{
protected:
    Client *pClient;
public:
    bool MouseOver(void) const;

    MenuLabel(Client *);
};

class AnimatedTextLabel : public MenuLabel
{
private:
    std::shared_ptr<StringProvider> pTextProvider;
    const std::shared_ptr<Pos2Provider> pRestPos2Provider,
                                               pAwayPos2Provider;
    float mAnimTime,
          mAppearTime;

    ResourceReference<Font> rButtonFont;
protected:
    void OnLeave(void);
    void OnEnter(void);
public:
    void Render(void);
    void Update(const float dt);

    bool Ready(void) const;

    void SetTextProvider(const std::shared_ptr<StringProvider> textProvider);

    void Animate(void);

    AnimatedTextLabel(Client *,
                      const std::shared_ptr<Pos2Provider> pRest,
                      const std::shared_ptr<Pos2Provider> pAway,
                      const std::shared_ptr<StringProvider> textProvider);
};

class AnimatedMenuButton : public MenuButton
{
private:
    bool bSpammable;
    std::shared_ptr<StringProvider> pTextProvider;
    float mAnimTime,
          mAppearTime;
    const std::shared_ptr<Pos2Provider> pRestPos2Provider,
                                               pAwayPos2Provider;
    GLfloat mWidth, mHeight;

    ResourceReference<Font> rButtonFont;
    ResourceReference<Texture> rTexPlain,
                               rTexTouched,
                               rTexClicked,
                               rTexDisabled;
protected:
    bool Clickable(void);
    bool MouseOver(void) const;

    void OnTouch(void);
    void OnClick(void);
    void OnLeave(void);
    void OnEnter(void);
public:
    AnimatedMenuButton(Client *, ButtonActionFunc,
                       const std::shared_ptr<Pos2Provider> pRest,
                       const std::shared_ptr<Pos2Provider> pAway,
                       const std::shared_ptr<StringProvider> textProvider,
                       const bool spammable,
                       const GLfloat width, const GLfloat height,
                       ResourceReference<Texture> texPlain,
                       ResourceReference<Texture> texTouched,
                       ResourceReference<Texture> texClicked,
                       ResourceReference<Texture> texDisabled);

    void Render(void);
    void Update(const float dt);

    bool Ready(void) const;

    void SetTextProvider(const std::shared_ptr<StringProvider> textProvider);
};

class AnimatedMenuButton424x96 : public AnimatedMenuButton
{
public:
    AnimatedMenuButton424x96(Client *,
                             ButtonActionFunc,
                             const std::shared_ptr<Pos2Provider> pRest,
                             const std::shared_ptr<Pos2Provider> pAway,
                             const std::shared_ptr<StringProvider> textProvider,
                             const bool spammable);
};

class AnimatedMenuButton164x96 : public AnimatedMenuButton
{
public:
    AnimatedMenuButton164x96(Client *,
                             ButtonActionFunc,
                             const std::shared_ptr<Pos2Provider> pRest,
                             const std::shared_ptr<Pos2Provider> pAway,
                             const std::shared_ptr<StringProvider> textProvider,
                             const bool spammable);
};

class AnimatedMenuButtonArrowRight : public AnimatedMenuButton
{
public:
    AnimatedMenuButtonArrowRight(Client *,
                                 ButtonActionFunc,
                                 const std::shared_ptr<Pos2Provider> pRest,
                                 const std::shared_ptr<Pos2Provider> pAway,
                                 const bool spammable);
};

class AnimatedMenuButtonArrowLeft : public AnimatedMenuButton
{
public:
    AnimatedMenuButtonArrowLeft(Client *,
                                ButtonActionFunc,
                                const std::shared_ptr<Pos2Provider> pRest,
                                const std::shared_ptr<Pos2Provider> pAway,
                                const bool spammable);
};

class AnimatedMenuButtonArrowUp : public AnimatedMenuButton
{
public:
    AnimatedMenuButtonArrowUp(Client *,
                              ButtonActionFunc,
                              const std::shared_ptr<Pos2Provider> pRest,
                              const std::shared_ptr<Pos2Provider> pAway,
                              const bool spammable);
};

class AnimatedMenuButtonArrowDown : public AnimatedMenuButton
{
public:
    AnimatedMenuButtonArrowDown(Client *,
                                ButtonActionFunc,
                                const std::shared_ptr<Pos2Provider> pRest,
                                const std::shared_ptr<Pos2Provider> pAway,
                                const bool spammable);
};

template <typename T>
class ToggleSelection : public MenuElement
{
private:
    Client *pClient;

    AnimatedMenuButton424x96 *mButton;
    int mChoiceIndex;
    std::vector<T> mChoices;
    const std::shared_ptr<ValueStringProvider<T>> pTextProvider;
public:
    ToggleSelection(Client *pCl,
                    const std::shared_ptr<Pos2Provider> pRest,
                    const std::shared_ptr<Pos2Provider> pAway,
                    const std::vector <T> &choices,
                    const std::shared_ptr<ValueStringProvider<T>> textProvider):
        mChoices(choices.begin(), choices.end()),
        pClient(pCl),
        mChoiceIndex(0),
        pTextProvider(textProvider)
    {
        mButton = new AnimatedMenuButton424x96(pClient,
            [&](MenuButton *)
            {
                mChoiceIndex ++;
                if (mChoiceIndex >= mChoices.size())
                    mChoiceIndex = 0;

                pTextProvider->SetInput(mChoices[mChoiceIndex]);
            },
            pRest, pAway, pTextProvider, true);

        SetChoice(pTextProvider->GetInput());
    }
    ~ToggleSelection(void)
    {
        delete mButton;
    }

    void SetChoice(const T &choice)
    {
        int i;

        for (i = 0; i < mChoices.size(); i++)
        {
            if (mChoices[i] == choice)
            {
                mChoiceIndex = i;

                pTextProvider->SetInput(mChoices[mChoiceIndex]);

                return;
            }
        }
    }
    T GetChoice(void) const
    {
        return mChoices[mChoiceIndex];
    }
    void OnEnter(void)
    {
        mButton->Enter();
    }
    void OnLeave(void)
    {
        mButton->Leave();
    }
    bool OnEvent(const SDL_Event &event)
    {
        return mButton->OnEvent(event);
    }
    void Render(void)
    {
        mButton->Render();
    }
    void Update(const float dt)
    {
        mButton->Update(dt);
    }
    bool Ready(void) const
    {
        return mButton->Ready();
    }
};

template <typename T>
class HorizontalSelection : public MenuElement
{
private:
    Client *pClient;

    AnimatedMenuButtonArrowLeft *mButtonLeft;
    AnimatedMenuButtonArrowRight *mButtonRight;
    AnimatedTextLabel *mLabel;

    int mChoiceIndex;
    std::vector<T> mChoices;
    const std::shared_ptr<ValueStringProvider<T>> pTextProvider;
public:
    HorizontalSelection(Client *pCl,
                        const std::shared_ptr<Pos2Provider> pRest,
                        const std::shared_ptr<Pos2Provider> pAway,
                        const GLfloat width,
                        const std::vector <T> &choices,
                        const std::shared_ptr<ValueStringProvider<T>> textProvider):
        mChoices(choices.begin(), choices.end()),
        pClient(pCl),
        mChoiceIndex(0),
        pTextProvider(textProvider)
    {
        mLabel = new AnimatedTextLabel(pClient,
                                       std::make_shared<TranslatedPos2Provider>(pRest, vec2(0.0f, 0.0f)),
                                       std::make_shared<TranslatedPos2Provider>(pAway, vec2(0.0f, 0.0f)),
                                       pTextProvider);

        mButtonLeft = new AnimatedMenuButtonArrowLeft(pClient,
            [&](MenuButton *)
            {
                if (mChoiceIndex > 0)
                    mChoiceIndex --;

                pTextProvider->SetInput(mChoices[mChoiceIndex]);

                mLabel->Animate();
            },
            std::make_shared<TranslatedPos2Provider>(pRest, vec2(-width / 2, 0.0f)),
            std::make_shared<TranslatedPos2Provider>(pAway, vec2(-width / 2, 0.0f)),
            true);

        mButtonRight = new AnimatedMenuButtonArrowRight(pClient,
            [&](MenuButton *)
            {
                if ((mChoiceIndex + 1) < mChoices.size())
                    mChoiceIndex ++;

                pTextProvider->SetInput(mChoices[mChoiceIndex]);

                mLabel->Animate();
            },
            std::make_shared<TranslatedPos2Provider>(pRest, vec2(width / 2, 0.0f)),
            std::make_shared<TranslatedPos2Provider>(pAway, vec2(width / 2, 0.0f)),
            true);

        SetChoice(pTextProvider->GetInput(), false);
    }
    ~HorizontalSelection(void)
    {
        delete mButtonRight;
        delete mButtonLeft;
        delete mLabel;
    }

    void SetChoice(const T &choice, const bool animate=true)
    {
        int i;

        for (i = 0; i < mChoices.size(); i++)
        {
            if (mChoices[i] == choice)
            {
                mChoiceIndex = i;

                pTextProvider->SetInput(mChoices[mChoiceIndex]);

                if (animate)
                    mLabel->Animate();

                return;
            }
        }
    }
    T GetChoice(void) const
    {
        return mChoices[mChoiceIndex];
    }
    void OnEnter(void)
    {
        mButtonLeft->Enter();
        mButtonRight->Enter();
        mLabel->Enter();
    }
    void OnLeave(void)
    {
        mButtonLeft->Leave();
        mButtonRight->Leave();
        mLabel->Leave();
    }
    bool OnEvent(const SDL_Event &event)
    {
        return mButtonLeft->OnEvent(event) ||
               mButtonRight->OnEvent(event) ||
               mLabel->OnEvent(event);
    }
    void Render(void)
    {
        mButtonLeft->Render();
        mButtonRight->Render();
        mLabel->Render();
    }
    void Update(const float dt)
    {
        mButtonLeft->Update(dt);
        mButtonRight->Update(dt);
        mLabel->Update(dt);
    }
    bool Ready(void) const
    {
        return mButtonLeft->Ready() &&
               mButtonRight->Ready() &&
               mLabel->Ready();
    }
};

class VerticalScrollable : public MenuElement
{
private:
    GLfloat mY,
            mScrollSpeed,
            mMinScrolledY,
            mMaxScrolledY;

    const std::shared_ptr<Pos2Provider> pTopLeft,
                                        pBottomRight;
public:
    ~VerticalScrollable(void);

    VerticalScrollable(const std::shared_ptr<Pos2Provider> pTopLeft,
                       const std::shared_ptr<Pos2Provider> pBottomRight,
                       const GLfloat scrolledYMin,
                       const GLfloat scrolledYMax);

    bool MouseOver(void) const;

    GLfloat GetDisplacement(void) const;

    void Scroll(GLfloat amount);

    void Update(const float dt);
    void Render(void);

    bool OnMouseWheel(const SDL_MouseWheelEvent &);
    bool Ready(void) const;
};

class VerticalScrollPos2Provider : public Pos2Provider
{
private:
    const VerticalScrollable *pScrollable;
    const std::shared_ptr<Pos2Provider> pPos2Provider;
public:
    VerticalScrollPos2Provider(const std::shared_ptr<Pos2Provider>,
                                   const VerticalScrollable *);

    vec2 GetValue(void) const;
};

typedef std::function<void (const SDL_KeyboardEvent &)> KeyResponseFunc;
typedef std::function<void (const SDL_MouseButtonEvent &)> ButtonResponseFunc;

class KeyListener : public MenuElement
{
private:
    KeyResponseFunc mFuncKey;
    ButtonResponseFunc mFuncButton;
protected:
    bool OnMouseButtonDown(const SDL_MouseButtonEvent &);
    bool OnKeyDown(const SDL_KeyboardEvent &);
public:
    KeyListener(void);

    void StartListening(KeyResponseFunc, ButtonResponseFunc);

    void Render(void);

    bool Ready(void) const;
};


typedef uint64_t MenuID;

typedef std::map <MenuID, Menu *> MenuMap;

class MainMenuBackground
{
private:
    Client *pClient;
    float mTime;
    ResourceReference<Texture> rBGTexture;
public:
    MainMenuBackground(Client *);

    void Render(void);
    void Update(const float dt);
};

class MenuCursor
{
private:
    Client *pClient;
    ResourceReference<Texture> rCursorTexture;
public:
    MenuCursor(Client *);

    void Render(void);
    void Update(const float dt);
};

void RenderMenuOverlay(const Client *, MenuCursor *, MenuMap &);

class MainMenuScene : public Scene
{
private:
    MainMenuBackground *mBackGround;

    MenuCursor *mCursor;

    MenuMap mMenus;
private:
    void InitMenus(void);
protected:
    bool OnEvent(const SDL_Event &);
    bool OnQuit(const SDL_QuitEvent &);
public:
    MainMenuScene(Client *);
    ~MainMenuScene(void);

    void Render(void);
    void Update(const float dt);

    void Reset(void);
};

void InitYesNoMenu(Menu *pMenu,
                   const std::shared_ptr<StringProvider> question,
                   MenuActionFunc onYes, MenuActionFunc onNo,
                   Client *pClient);

class PlayScene;
void InitEscapeMenu(Menu *pEscapeMenu,
                    Menu *pOptionsMenu,
                    PlayScene *,
                    Client *);

void InitVideoMenu(Menu *pVideoMenu,
                   Menu *pGoBackMenu,
                   Client *pClient);

void InitControlsMenu(Menu *pControlsMenu,
                      Menu *pGoBackMenu,
                      Client *pClient);

void InitLanguageMenu(Menu *pLanguageMenu,
                      Menu *pGoBackMenu,
                      Client *pClient);

void InitOptionsMenu(Menu *pMenuOptions,
                     Menu *pMenuVideo,
                     Menu *pMenuControls,
                     Menu *pMenuLanguage,
                     Menu *pMenuBack,
                     Client *pClient);

#endif  // MENU_H
