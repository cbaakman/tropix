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
#include <sstream>

#include "menu.h"
#include "exception.h"
#include "matrix.h"
#include "sprite.h"
#include "log.h"
#include "play.h"


LanguageStringProvider::LanguageStringProvider(Client *pCl, const std::string &key):
    pClient(pCl), mKey(key)
{
}
std::string LanguageStringProvider::GetValue(void) const
{
    return pClient->GetLanguageString(mKey);
}
class VerticalMidMenuPos2Provider : public Pos2Provider
{
private:
    Client *pClient;
    GLfloat mY;
public:
    VerticalMidMenuPos2Provider(Client *pCl, const GLfloat y):
        pClient(pCl),
        mY(y)
    {
    }

    vec2 GetValue(void) const
    {
        ClientSettings settings;
        pClient->GetSettings(settings);

        GLfloat x = settings.display.resolution.width / 2;
        return {x, mY};
    }
};
class VerticalAwayMenuPos2Provider : public Pos2Provider
{
private:
    Client *pClient;
    const std::shared_ptr<Pos2Provider> pRestPos;
    GLfloat mAwayFromScreen;
public:
    VerticalAwayMenuPos2Provider(Client *pCl,
                                 const std::shared_ptr<Pos2Provider> pPP,
                                 const GLfloat away):
        pClient(pCl),
        pRestPos(pPP),
        mAwayFromScreen(away)
    {
    }
    vec2 GetValue(void) const
    {
        ClientSettings settings;
        pClient->GetSettings(settings);

        GLfloat x = pRestPos->GetValue().x + settings.display.resolution.width / 2 + mAwayFromScreen;
        return {x, pRestPos->GetValue().y};
    }
};
class FromTopScreenPos2Provider : public Pos2Provider
{
private:
    Client *pClient;
    vec2 mFromTopMid;
public:
    FromTopScreenPos2Provider(Client *pCl,
                              const vec2 &fromTopMid):
        pClient(pCl),
        mFromTopMid(fromTopMid)
    {
    }
    vec2 GetValue(void) const
    {
        ClientSettings settings;
        pClient->GetSettings(settings);

        vec2 pos = {settings.display.resolution.width / 2, 0.0f};
        return pos + mFromTopMid;
    }
};
class FromBottomScreenPos2Provider : public Pos2Provider
{
private:
    Client *pClient;
    vec2 mFromBottomMid;
public:
    FromBottomScreenPos2Provider(Client *pCl,
                                 const vec2 &fromBottomMid):
        pClient(pCl),
        mFromBottomMid(fromBottomMid)
    {
    }
    vec2 GetValue(void) const
    {
        ClientSettings settings;
        pClient->GetSettings(settings);

        vec2 pos = {settings.display.resolution.width / 2,
                    settings.display.resolution.height};
        return pos + mFromBottomMid;
    }
};
MenuElement::MenuElement():
    bActive(false),
    bDisabled(false)
{
}
void MenuElement::Leave(void)
{
    if (bActive)
        OnLeave();
    bActive = false;
}
void MenuElement::Enter(void)
{
    if (!bActive)
        OnEnter();
    bActive = true;
}
void MenuElement::Disable(void)
{
    bDisabled = true;
}
void MenuElement::Enable(void)
{
    bDisabled = false;
}
bool MenuElement::IsActive(void) const
{
    return bActive;
}
bool MenuElement::IsDisabled(void) const
{
    return bDisabled;
}
Menu::Menu(void):
    bActive(false),
    bElementsLeaving(true),
    mLeaveAction(NULL)
{
}
Menu::~Menu(void)
{
    for (MenuElement *pElement : mElements)
        delete pElement;
}
void Menu::AddElement(MenuElement *pElement)
{
    mElements.push_back(pElement);

    if (bActive)
        pElement->Enter();
}
bool Menu::Ready(void) const
{
    for (MenuElement *pElement : mElements)
    {
        if (!pElement->Ready())
        {
            return false;
        }
    }

    return true;
}
bool Menu::IsActive(void) const
{
    return bActive;
}
void Menu::DisableAll(void)
{
    for (MenuElement *pElement : mElements)
        pElement->Disable();
}
void Menu::EnableAll(void)
{
    for (MenuElement *pElement : mElements)
        pElement->Enable();
}
void Menu::Leave(MenuActionFunc leaveAction)
{
    bActive = false;
    bElementsLeaving = false;
    mLeaveAction = leaveAction;
}
void Menu::Enter(void)
{
    for (MenuElement *pElement : mElements)
        pElement->Enter();

    bActive = true;
    bElementsLeaving = false;
}
void Menu::Render(void)
{
    for (MenuElement *pElement : mElements)
        if (!bActive && bElementsLeaving && pElement->Ready())
            continue;
        else
            pElement->Render();
}
void Menu::Update(const float dt)
{
    bool allReady = true;
    for (MenuElement *pElement : mElements)
    {
        pElement->Update(dt);

        if (!pElement->Ready())
            allReady = false;
    }
    if (allReady && !bActive)
    {
        if (!bElementsLeaving)
        {
            for (MenuElement *pElement : mElements)
                pElement->Leave();
            bElementsLeaving = true;
        }
        else if (bElementsLeaving && mLeaveAction != NULL)
        {
            mLeaveAction();
            mLeaveAction = NULL;
        }
    }
}
bool Menu::OnEvent(const SDL_Event &event)
{
    if (bActive)
    {
        if (EventProcessor::OnEvent(event))
            return true;

        for (MenuElement *pElement : mElements)
        {
            if (!pElement->IsDisabled())
                if (pElement->OnEvent(event))
                    return true;
        }
    }
    return false;
}
class RenderDistanceStringProvider : public ValueStringProvider<int>
{
private:
    Client *pClient;
protected:
    std::string ToString(const int &distance) const
    {
        std::ostringstream ss;
        ss << pClient->GetLanguageString("menu.option.video.renderdistance") << ": "
           << distance;

        return ss.str();
    }
public:
    RenderDistanceStringProvider(Client *pCl, const int &distance):
        ValueStringProvider(distance),
        pClient(pCl)
    {
    }
};
class ResolutionStringProvider : public ValueStringProvider<ScreenResolution>
{
private:
    Client *pClient;
protected:
    std::string ToString(const ScreenResolution &resolution) const
    {
        std::ostringstream ss;

        ss << pClient->GetLanguageString("menu.option.video.resolution") << ": "
           << resolution.width << "x" << resolution.height;

        return ss.str();
    }
public:
    ResolutionStringProvider(Client *pCl, const ScreenResolution &res):
        ValueStringProvider(res),
        pClient(pCl)
    {
    }
};
class FullscreenStringProvider : public ValueStringProvider<bool>
{
private:
    Client *pClient;
protected:
    std::string ToString(const bool &fullscreen) const
    {
        std::ostringstream ss;

        ss << pClient->GetLanguageString("menu.option.video.fullscreen") << ": ";

        if (fullscreen)
            ss << pClient->GetLanguageString("menu.answer.yes");
        else
            ss << pClient->GetLanguageString("menu.answer.no");

        return ss.str();
    }
public:
    FullscreenStringProvider(Client *pCl, const bool &fullscreen):
        ValueStringProvider(fullscreen),
        pClient(pCl)
    {
    }
};
MainMenuBackground::MainMenuBackground(Client *pCl):
    pClient(pCl),
    mTime(0.0f),
    rBGTexture(pClient->GetResourceManager()->GetTexture("menu-bg"))
{
}
#define BG_SPEED 0.1f
void MainMenuBackground::Render(void)
{
    ClientSettings settings;
    pClient->GetSettings(settings);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(&MatOrtho(0, settings.display.resolution.width,
                            0, settings.display.resolution.height,
                            -1.0f, 1.0f));

    glViewport(0, 0, settings.display.resolution.width,
                     settings.display.resolution.height);

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(&MatID());

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBindTexture(GL_TEXTURE_2D, rBGTexture->tex);

    GLfloat tw = 256.0f,
            th = 256.0f,
            tfx0 = BG_SPEED * mTime,
            tfy0 = BG_SPEED * mTime,
            tfx1 = tfx0 + GLfloat(settings.display.resolution.width) / tw,
            tfy1 = tfy0 + GLfloat(settings.display.resolution.height) / th;

    glBegin(GL_QUADS);

    glTexCoord2f(tfx0, tfy1);
    glVertex2f(0.0f, 0.0f);

    glTexCoord2f(tfx1, tfy1);
    glVertex2f((GLfloat)settings.display.resolution.width, 0.0f);

    glTexCoord2f(tfx1, tfy0);
    glVertex2f((GLfloat)settings.display.resolution.width,
               (GLfloat)settings.display.resolution.height);

    glTexCoord2f(tfx0, tfy0);
    glVertex2f(0.0f, (GLfloat)settings.display.resolution.height);

    glEnd();
}
void MainMenuBackground::Update(const float dt)
{
    mTime += dt;
}
MenuCursor::MenuCursor(Client *pCl):
    pClient(pCl),
    rCursorTexture(pClient->GetResourceManager()->GetTexture("default-cursor"))
{
}
void MenuCursor::Render(void)
{
    int mouseX, mouseY;

    if (pClient->HasMouseFocus())
    {
        SDL_GetMouseState(&mouseX, &mouseY);

        GLfloat px = 6.0f,
                py = 6.0f,
                tw = 48.0f,
                th = 48.0f;

        RenderSprite(rCursorTexture,
                    (GLfloat)mouseX - px, (GLfloat)mouseY - py,
                    tw, th);
    }
}
void MenuCursor::Update(const float dt)
{
}
MainMenuScene::MainMenuScene(Client *pCl):
    Scene(pCl),
    mBackGround(NULL),
    mCursor(NULL)
{
    mInitJobs.push_back([&]()
    {
        mBackGround = new MainMenuBackground(pClient);
    });

    mInitJobs.push_back([&]()
    {
        mCursor = new MenuCursor(pClient);
    });

    mInitJobs.push_back([&]()
    {
        InitMenus();
    });
}
MainMenuScene::~MainMenuScene(void)
{
    delete mBackGround;

    delete mCursor;

    for (auto pair : mMenus)
        delete pair.second;
}

#define MENU_AWAY_OUTOFSCREEN 300.0f
void InitVideoMenu(Menu *pVideoMenu, Menu *pGoBackMenu, Client *pClient)
{
    HorizontalSelection<int> *pRenderDistanceSelection;
    HorizontalSelection<ScreenResolution> *pResolutionSelection;
    ToggleSelection<bool> *pFullscreenSelection;
    std::shared_ptr<Pos2Provider> pRestPos,
                                  pAwayPos;
    std::shared_ptr<ResolutionStringProvider> pResolution;
    std::shared_ptr<FullscreenStringProvider> pFullscreen;
    std::shared_ptr<RenderDistanceStringProvider> pRenderDistance;

    std::vector<ScreenResolution> resolutionOptions;
    pClient->GetResolutionOptions(resolutionOptions);

    std::vector<int> renderDistanceOptions;
    for (int rd = 100; rd <= 500; rd += 100)
        renderDistanceOptions.push_back(rd);

    ClientSettings settings;
    pClient->GetSettings(settings);

    pRestPos = std::make_shared<VerticalMidMenuPos2Provider>(pClient, 120.0f);
    pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);
    pRenderDistance = std::make_shared<RenderDistanceStringProvider>(pClient, settings.render.renderDistance);
    pRenderDistanceSelection = new HorizontalSelection<int>(
                                pClient,
                                pRestPos, pAwayPos, 420.0f,
                                renderDistanceOptions,
                                pRenderDistance);
    pVideoMenu->AddElement(pRenderDistanceSelection);

    pRestPos = std::make_shared<VerticalMidMenuPos2Provider>(pClient, 220.0f);
    pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);
    pResolution = std::make_shared<ResolutionStringProvider>(pClient, settings.display.resolution);
    pResolutionSelection = new HorizontalSelection<ScreenResolution>(
                                pClient,
                                pRestPos, pAwayPos, 420.0f,
                                resolutionOptions,
                                pResolution);
    pVideoMenu->AddElement(pResolutionSelection);

    pRestPos = std::make_shared<VerticalMidMenuPos2Provider>(pClient, 320.0f);
    pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);
    pFullscreen = std::make_shared<FullscreenStringProvider>(pClient, settings.display.fullscreen);
    std::vector<bool> fullscreenChoices;
    fullscreenChoices.push_back(true);
    fullscreenChoices.push_back(false);
    pFullscreenSelection = new ToggleSelection<bool>(pClient,
                                                     pRestPos, pAwayPos,
                                                     fullscreenChoices,
                                                     pFullscreen);
    pVideoMenu->AddElement(pFullscreenSelection);

    pRestPos = std::make_shared<VerticalMidMenuPos2Provider>(pClient, 420.0f);
    pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);
    pVideoMenu->AddElement(
        new AnimatedMenuButton424x96(pClient,
            [pVideoMenu, pClient, pRenderDistanceSelection,
             pResolutionSelection, pFullscreenSelection, pGoBackMenu](MenuButton *)
            {
                pVideoMenu->Leave([&]()
                {
                    ClientSettings oldSettings;
                    DisplaySettings newDisplaySettings = oldSettings.display;
                    RenderSettings newRenderSettings = oldSettings.render;

                    pClient->GetSettings(oldSettings);

                    newDisplaySettings.resolution = pResolutionSelection->GetChoice();
                    newDisplaySettings.fullscreen = pFullscreenSelection->GetChoice();

                    if (oldSettings.display != newDisplaySettings)
                    {
                        try
                        {
                            pClient->ChangeDisplaySettings(newDisplaySettings);
                        }
                        catch (std::exception &e)
                        {
                            pClient->MessageBoxError(e.what());
                            pClient->ShutDown();
                            return;
                        }
                    }

                    newRenderSettings.renderDistance = pRenderDistanceSelection->GetChoice();
                    pClient->SetRenderSettings(newRenderSettings);

                    pGoBackMenu->Enter();
                });
            },
            pRestPos, pAwayPos,
            std::make_shared<LanguageStringProvider>(pClient, "menu.option.done"), false)
    );
}
class ControlsStringProvider : public StringProvider
{
private:
    Client *pClient;
    bool bSet;
    KeyBinding mKeyBinding;
    KeyBindingValue mKeyBindingValue;
public:
    ControlsStringProvider(Client *pCl, const KeyBinding keyb):
        bSet(false),
        pClient(pCl),
        mKeyBinding(keyb)
    {
    }
    ControlsStringProvider(Client *pCl, const KeyBinding keyb, const KeyBindingValue &value):
        bSet(true),
        pClient(pCl),
        mKeyBinding(keyb),
        mKeyBindingValue(value)
    {
    }
    std::string GetValue(void) const
    {
        std::string description = pClient->GetLanguageString(GetKeyBindingDescription(mKeyBinding));

        if (bSet)
            return description + ": " + pClient->GetLanguageString(GetKeyBindingValueName(mKeyBindingValue));
        else
            return description + ": ???";
    }
};
#define SCROLLABLE_Y 30.0f
#define SCROLLABLE_X -50.0f
#define SCROLLABLE_W_2 210.0f
#define SCROLLARROW_X 290.0f
#define ARROW_SCROLL 250.0f
#define BUTTON_DISTANCE 100.0f
void InitControlsMenu(Menu *pControlsMenu, Menu *pGoBackMenu,
                      Client *pClient)
{
    ClientSettings settings;
    pClient->GetSettings(settings);

    std::shared_ptr<Pos2Provider> pTopLeft,
                                  pBottomRight,
                                  pRestPos,
                                  pAwayPos;
    int nControlsButtons;
    VerticalScrollable *pKeyBScroll;
    AnimatedMenuButton *pButton;
    KeyBinding keyb;
    KeyBindingValue kbValue;

    KeyListener *pListener = new KeyListener();
    pControlsMenu->AddElement(pListener);

    pTopLeft = std::make_shared<FromTopScreenPos2Provider>(pClient, vec2(SCROLLABLE_X - SCROLLABLE_W_2, SCROLLABLE_Y));
    pBottomRight = std::make_shared<FromBottomScreenPos2Provider>(pClient, vec2(SCROLLABLE_X + SCROLLABLE_W_2, -SCROLLABLE_Y));

    nControlsButtons = settings.controls.size();
    pKeyBScroll = new VerticalScrollable(pTopLeft, pBottomRight,
                                         BUTTON_DISTANCE * 0.5f,
                                         BUTTON_DISTANCE * (nControlsButtons + 0.5f));
    pControlsMenu->AddElement(pKeyBScroll);

    GLfloat y = BUTTON_DISTANCE;

    for (auto pair : settings.controls)
    {
        keyb = pair.first;
        kbValue = pair.second;

        pRestPos = std::make_shared<FromTopScreenPos2Provider>(pClient, vec2(SCROLLABLE_X, y));
        pRestPos = std::make_shared<VerticalScrollPos2Provider>(pRestPos, pKeyBScroll);
        pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);

        pButton = new AnimatedMenuButton424x96(pClient,
                [pClient, pControlsMenu, keyb, pListener](MenuButton *pThisButton)
                {
                    AnimatedMenuButton *pKeyBButtonActive = dynamic_cast<AnimatedMenuButton *>(pThisButton);

                    pKeyBButtonActive->SetTextProvider(std::make_shared<ControlsStringProvider>(pClient, keyb));

                    pControlsMenu->DisableAll();
                    pListener->Enable();

                    pListener->StartListening(
                    [pClient, pControlsMenu, keyb, pKeyBButtonActive](const SDL_KeyboardEvent &event)
                    {
                        KeyBindingValue value;
                        value.type = KEYTYPE_KEYBOARD;
                        value.key.code = event.keysym.sym;

                        pClient->SetKeyBinding(keyb, value);

                        pKeyBButtonActive->SetTextProvider(std::make_shared<ControlsStringProvider>(pClient, keyb, value));
                        pKeyBButtonActive->UnClick();

                        pControlsMenu->EnableAll();
                    },
                    [pClient, pControlsMenu, keyb, pKeyBButtonActive](const SDL_MouseButtonEvent &event)
                    {
                        KeyBindingValue value;
                        value.type = KEYTYPE_MOUSEBUTTON;
                        value.mouse.button = event.button;

                        pClient->SetKeyBinding(keyb, value);

                        pKeyBButtonActive->SetTextProvider(std::make_shared<ControlsStringProvider>(pClient, keyb, value));
                        pKeyBButtonActive->UnClick();

                        pControlsMenu->EnableAll();
                    });
                },
                pRestPos, pAwayPos,
                std::make_shared<ControlsStringProvider>(pClient, keyb, kbValue),
                false);
        pControlsMenu->AddElement(pButton);
        y += BUTTON_DISTANCE;
    }

    pRestPos = std::make_shared<FromTopScreenPos2Provider>(pClient, vec2(SCROLLARROW_X, 120.0f));
    pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);
    pControlsMenu->AddElement(
        new AnimatedMenuButtonArrowUp(pClient,
            [pKeyBScroll](MenuButton *)
            {
                pKeyBScroll->Scroll(ARROW_SCROLL);
            },
            pRestPos, pAwayPos, true)
    );
    pRestPos = std::make_shared<FromBottomScreenPos2Provider>(pClient, vec2(SCROLLARROW_X, -180.0f));
    pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);
    pControlsMenu->AddElement(
        new AnimatedMenuButtonArrowDown(pClient,
            [pKeyBScroll](MenuButton *)
            {
                pKeyBScroll->Scroll(-ARROW_SCROLL);
            },
            pRestPos, pAwayPos, true)
    );

    pRestPos = std::make_shared<FromBottomScreenPos2Provider>(pClient, vec2(SCROLLARROW_X, -80.0f));
    pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);

    pControlsMenu->AddElement(
        new AnimatedMenuButton164x96(pClient,
            [pControlsMenu, pGoBackMenu](MenuButton *)
            {
                pControlsMenu->Leave([&]()
                {
                    pGoBackMenu->Enter();
                });
            },
            pRestPos, pAwayPos,
            std::make_shared<LanguageStringProvider>(pClient, "menu.option.done"),
            false)
    );
}
class LanguageButtonManipulator : public MenuElement
{
private:
    Client *pClient;

    std::map<std::string, MenuButton *> pButtons;
public:
    LanguageButtonManipulator(Client *pCl):
        pClient(pCl)
    {
    }
    void Render(void)
    {
    }
    bool Ready(void) const
    {
        return true;
    }
    void Add(const std::string &language, MenuButton *pButton)
    {
        pButtons[language] = pButton;
    }
    void UpdateButtons(void)
    {
        ClientSettings settings;
        pClient->GetSettings(settings);

        for (auto pair : pButtons)
        {
            if (pair.first != settings.language)
            {
                pair.second->UnClick();
            }
        }
    }
    void OnEnter(void)
    {
        ClientSettings settings;
        pClient->GetSettings(settings);

        // Make sure the button for the current language is already clicked.
        if (pButtons.find(settings.language) != pButtons.end())
        {
            pButtons.at(settings.language)->SetClicked();
        }
    }
};
void InitLanguageMenu(Menu *pLanguageMenu, Menu *pGoBackMenu,
                      Client *pClient)
{
    ClientSettings settings;
    pClient->GetSettings(settings);

    std::list <std::string> languages;
    pClient->GetLanguageOptions(languages);

    std::shared_ptr <Pos2Provider> pTopLeft,
                                   pBottomRight,
                                   pRestPos,
                                   pAwayPos;
    LanguageButtonManipulator *pManipulator = new LanguageButtonManipulator(pClient);
    int nLanguages;
    VerticalScrollable *pKeyBScroll;
    AnimatedMenuButton *pButton;

    pTopLeft = std::make_shared<FromTopScreenPos2Provider>(pClient, vec2(SCROLLABLE_X - SCROLLABLE_W_2, SCROLLABLE_Y));
    pBottomRight = std::make_shared<FromBottomScreenPos2Provider>(pClient, vec2(SCROLLABLE_X + SCROLLABLE_W_2, -SCROLLABLE_Y));

    nLanguages = languages.size();
    pKeyBScroll = new VerticalScrollable(pTopLeft, pBottomRight,
                                         BUTTON_DISTANCE * 0.5f,
                                         BUTTON_DISTANCE * (nLanguages + 0.5f));
    pLanguageMenu->AddElement(pKeyBScroll);

    GLfloat y = BUTTON_DISTANCE;

    for (const std::string &language : languages)
    {
        pRestPos = std::make_shared<FromTopScreenPos2Provider>(pClient, vec2(SCROLLABLE_X, y));
        pRestPos = std::make_shared<VerticalScrollPos2Provider>(pRestPos, pKeyBScroll);
        pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);

        pButton = new AnimatedMenuButton424x96(pClient,
                [language, pClient, pManipulator](MenuButton *pThisButton)
                {
                    try
                    {
                        pClient->SetLanguage(language);
                    }
                    catch (std::exception &e)
                    {
                        pClient->GetMainRenderLoop()->ShowOnScreenWarning(e.what());
                    }

                    pManipulator->UpdateButtons();
                },
                pRestPos, pAwayPos,
                std::make_shared<ConstantStringProvider>(language),
                false);
        pLanguageMenu->AddElement(pButton);
        y += BUTTON_DISTANCE;

        pManipulator->Add(language, pButton);
    }

    /*
       The manipulator's 'OnEnter' must be called AFTER the button's.
       So add it AFTER the buttons:
     */
    pLanguageMenu->AddElement(pManipulator);

    pRestPos = std::make_shared<FromTopScreenPos2Provider>(pClient, vec2(SCROLLARROW_X, 120.0f));
    pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);
    pLanguageMenu->AddElement(
        new AnimatedMenuButtonArrowUp(pClient,
            [pKeyBScroll](MenuButton *)
            {
                pKeyBScroll->Scroll(ARROW_SCROLL);
            },
            pRestPos, pAwayPos, true)
    );
    pRestPos = std::make_shared<FromBottomScreenPos2Provider>(pClient, vec2(SCROLLARROW_X, -180.0f));
    pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);
    pLanguageMenu->AddElement(
        new AnimatedMenuButtonArrowDown(pClient,
            [pKeyBScroll](MenuButton *)
            {
                pKeyBScroll->Scroll(-ARROW_SCROLL);
            },
            pRestPos, pAwayPos, true)
    );

    pRestPos = std::make_shared<FromBottomScreenPos2Provider>(pClient, vec2(SCROLLARROW_X, -80.0f));
    pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);
    pLanguageMenu->AddElement(
        new AnimatedMenuButton164x96(pClient,
            [pLanguageMenu, pGoBackMenu](MenuButton *)
            {
                pLanguageMenu->Leave([&]()
                {
                    pGoBackMenu->Enter();
                });
            },
            pRestPos, pAwayPos,
            std::make_shared<LanguageStringProvider>(pClient, "menu.option.done"),
            false)
    );
}
void InitOptionsMenu(Menu *pMenuOptions,
                     Menu *pMenuVideo,
                     Menu *pMenuControls,
                     Menu *pMenuLanguage,
                     Menu *pMenuBack,
                     Client *pClient)
{
    std::shared_ptr<Pos2Provider> pRestPos,
                                  pAwayPos;

    pRestPos = std::make_shared<VerticalMidMenuPos2Provider>(pClient, 170.0f);
    pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);
    pMenuOptions->AddElement(
        new AnimatedMenuButton424x96(pClient,
            [pMenuOptions, pMenuVideo](MenuButton *)
            {
                pMenuOptions->Leave([&]()
                {
                    pMenuVideo->Enter();
                });
            },
            pRestPos, pAwayPos,
            std::make_shared<LanguageStringProvider>(pClient, "menu.option.video"),
            false)
    );
    pRestPos = std::make_shared<VerticalMidMenuPos2Provider>(pClient, 270.0f);
    pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);
    pMenuOptions->AddElement(
        new AnimatedMenuButton424x96(pClient,
            [pMenuOptions, pMenuControls](MenuButton *)
            {
                pMenuOptions->Leave([&]()
                {
                    pMenuControls->Enter();
                });
            },
            pRestPos, pAwayPos,
            std::make_shared<LanguageStringProvider>(pClient, "menu.option.controls"),
            false)
    );
    pRestPos = std::make_shared<VerticalMidMenuPos2Provider>(pClient, 370.0f);
    pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);
    pMenuOptions->AddElement(
        new AnimatedMenuButton424x96(pClient,
            [pMenuOptions, pMenuLanguage](MenuButton *)
            {
                pMenuOptions->Leave([&]()
                {
                    pMenuLanguage->Enter();
                });
            },
            pRestPos, pAwayPos,
            std::make_shared<LanguageStringProvider>(pClient, "menu.option.language"),
            false)
    );
    pRestPos = std::make_shared<VerticalMidMenuPos2Provider>(pClient, 470.0f);
    pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);
    pMenuOptions->AddElement(
        new AnimatedMenuButton424x96(pClient,
            [pMenuOptions, pMenuBack](MenuButton *)
            {
                pMenuOptions->Leave([&]()
                {
                    pMenuBack->Enter();
                });
            },
            pRestPos, pAwayPos,
            std::make_shared<LanguageStringProvider>(pClient, "menu.option.done"),
            false)
    );
}
void InitYesNoMenu(Menu *pMenu,
                   const std::shared_ptr<StringProvider> question,
                   MenuActionFunc onYes, MenuActionFunc onNo,
                   Client *pClient)
{
    std::shared_ptr<Pos2Provider> pRestPos,
                                  pAwayPos;

    pRestPos = std::make_shared<VerticalMidMenuPos2Provider>(pClient, 220.0f);
    pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);
    pMenu->AddElement(
        new AnimatedTextLabel(pClient, pRestPos, pAwayPos, question)
    );

    pRestPos = std::make_shared<FromTopScreenPos2Provider>(pClient, vec2(-100.0f, 320.0));
    pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);
    pMenu->AddElement(
        new AnimatedMenuButton164x96(pClient,
            [pMenu, onYes](MenuButton *)
            {
                pMenu->Leave(onYes);
            },
            pRestPos, pAwayPos,
            std::make_shared<LanguageStringProvider>(pClient, "menu.answer.yes"),
            false)
    );

    pRestPos = std::make_shared<FromTopScreenPos2Provider>(pClient, vec2(100.0f, 320.0f));
    pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);
    pMenu->AddElement(
        new AnimatedMenuButton164x96(pClient,
            [pMenu, onNo](MenuButton *)
            {
                pMenu->Leave(onNo);
            },
            pRestPos, pAwayPos,
            std::make_shared<LanguageStringProvider>(pClient, "menu.answer.no"),
            false)
    );
}
void InitMainMenu(Menu *pMenuMain, Menu *pMenuOptions, Menu *pMenuQuit,
                  Client *pClient)
{
    std::shared_ptr<Pos2Provider> pRestPos,
                                  pAwayPos;

    pRestPos = std::make_shared<VerticalMidMenuPos2Provider>(pClient, 220.0f);
    pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);
    pMenuMain->AddElement(
        new AnimatedMenuButton424x96(pClient,
            [pMenuMain, pClient](MenuButton *)
            {
                pMenuMain->Leave([&]()
                {
                    pClient->StartSinglePlayer();
                });
            },
            pRestPos, pAwayPos,
            std::make_shared<LanguageStringProvider>(pClient, "menu.option.singleplayer"),
            false)
    );
    pRestPos = std::make_shared<VerticalMidMenuPos2Provider>(pClient, 320.0f);
    pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);
    pMenuMain->AddElement(
        new AnimatedMenuButton424x96(pClient,
            [pMenuMain, pMenuOptions](MenuButton *)
            {
                pMenuMain->Leave([&]()
                {
                    pMenuOptions->Enter();
                });
            },
            pRestPos, pAwayPos,
            std::make_shared<LanguageStringProvider>(pClient, "menu.option.options"),
            false)
    );
    pRestPos = std::make_shared<VerticalMidMenuPos2Provider>(pClient, 420.0f);
    pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);
    pMenuMain->AddElement(
        new AnimatedMenuButton424x96(pClient,
            [pMenuMain, pMenuQuit](MenuButton *)
            {
                pMenuMain->Leave([&]()
                {
                    pMenuQuit->Enter();
                });
            },
            pRestPos, pAwayPos,
            std::make_shared<LanguageStringProvider>(pClient, "menu.option.quit"),
            false)
    );
}
void InitEscapeMenu(Menu *pMenuEscape, Menu *pMenuOptions,
                    PlayScene *pPlayScene, Client *pClient)
{
    std::shared_ptr<Pos2Provider> pRestPos,
                                  pAwayPos;

    pRestPos = std::make_shared<VerticalMidMenuPos2Provider>(pClient, 220.0f);
    pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);
    pMenuEscape->AddElement(
        new AnimatedMenuButton424x96(pClient,
            [pPlayScene](MenuButton *)
            {
                pPlayScene->UnescapeFromMenu();
            },
            pRestPos, pAwayPos,
            std::make_shared<LanguageStringProvider>(pClient, "menu.option.continue"),
            false)
    );
    pRestPos = std::make_shared<VerticalMidMenuPos2Provider>(pClient, 320.0f);
    pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);
    pMenuEscape->AddElement(
        new AnimatedMenuButton424x96(pClient,
            [pMenuEscape, pMenuOptions](MenuButton *)
            {
                pMenuEscape->Leave([&]()
                {
                    pMenuOptions->Enter();
                });
            },
            pRestPos, pAwayPos,
            std::make_shared<LanguageStringProvider>(pClient, "menu.option.options"),
            false)
    );
    pRestPos = std::make_shared<VerticalMidMenuPos2Provider>(pClient, 420.0f);
    pAwayPos = std::make_shared<VerticalAwayMenuPos2Provider>(pClient, pRestPos, MENU_AWAY_OUTOFSCREEN);
    pMenuEscape->AddElement(
        new AnimatedMenuButton424x96(pClient,
            [pMenuEscape, pClient](MenuButton *)
            {
                pClient->ReturnToMain();
            },
            pRestPos, pAwayPos,
            std::make_shared<LanguageStringProvider>(pClient, "menu.option.quittomain"),
            false)
    );
}
KeyListener::KeyListener(void):
    mFuncKey(NULL),
    mFuncButton(NULL)
{
}
void KeyListener::StartListening(KeyResponseFunc funcK, ButtonResponseFunc funcB)
{
    mFuncKey = funcK;
    mFuncButton = funcB;
}
bool KeyListener::OnKeyDown(const SDL_KeyboardEvent &event)
{
    bool b = false;

    if (mFuncKey)
    {
        mFuncKey(event);
        b = true;
    }

    mFuncButton = NULL;
    mFuncKey = NULL;

    return b;
}
bool KeyListener::OnMouseButtonDown(const SDL_MouseButtonEvent &event)
{
    bool b = false;

    if (mFuncButton)
    {
        mFuncButton(event);
        b = true;
    }

    mFuncButton = NULL;
    mFuncKey = NULL;

    return b;
}
void KeyListener::Render(void)
{
}
bool KeyListener::Ready(void) const
{
    return true;
}
#define MENUID_MAIN 1
#define MENUID_OPTIONS 2
#define MENUID_VIDEO 3
#define MENUID_CONTROLS 4
#define MENUID_LANGUAGE 5
#define MENUID_QUIT 6
void MainMenuScene::InitMenus(void)
{
    mMenus.emplace(MENUID_MAIN, new Menu());
    mMenus.emplace(MENUID_OPTIONS, new Menu());
    mMenus.emplace(MENUID_VIDEO, new Menu());
    mMenus.emplace(MENUID_CONTROLS, new Menu());
    mMenus.emplace(MENUID_LANGUAGE, new Menu());
    mMenus.emplace(MENUID_QUIT, new Menu());

    InitYesNoMenu(mMenus[MENUID_QUIT],
                  std::make_shared<LanguageStringProvider>(pClient, "menu.question.quit"),
                  [this]()
                  {
                      pClient->ShutDown();
                  },
                  [this]()
                  {
                      mMenus[MENUID_MAIN]->Enter();
                  },
                  pClient);

    InitMainMenu(mMenus[MENUID_MAIN], mMenus[MENUID_OPTIONS], mMenus[MENUID_QUIT], pClient);
    InitOptionsMenu(mMenus[MENUID_OPTIONS], mMenus[MENUID_VIDEO],
                    mMenus[MENUID_CONTROLS], mMenus[MENUID_LANGUAGE],
                    mMenus[MENUID_MAIN], pClient);
    InitVideoMenu(mMenus[MENUID_VIDEO], mMenus[MENUID_OPTIONS], pClient);
    InitControlsMenu(mMenus[MENUID_CONTROLS], mMenus[MENUID_OPTIONS], pClient);
    InitLanguageMenu(mMenus[MENUID_LANGUAGE], mMenus[MENUID_OPTIONS], pClient);

    mMenus[MENUID_MAIN]->Enter();
}
void MainMenuScene::Reset(void)
{
    for (auto pair : mMenus)
        pair.second->Leave();

    mMenus[MENUID_MAIN]->Enter();
}
bool MainMenuScene::OnEvent(const SDL_Event &event)
{
    if (EventProcessor::OnEvent(event))
        return true;

    for (auto pair : mMenus)
    {
        if (pair.second->OnEvent(event))
            return true;
    }
    return false;
}
bool MainMenuScene::OnQuit(const SDL_QuitEvent &)
{
    Menu *pActiveMenu = NULL;
    for (auto pair : mMenus)
        if (pair.second->IsActive())
            pActiveMenu = pair.second;

    if (pActiveMenu)
        pActiveMenu->Leave([this](){
            mMenus[MENUID_QUIT]->Enter();
        });
    else
        mMenus[MENUID_QUIT]->Enter();

    return true;
}
void MainMenuScene::Update(const float dt)
{
    mBackGround->Update(dt);
    mCursor->Update(dt);

    for (auto pair : mMenus)
        pair.second->Update(dt);
}
void RenderMenuOverlay(const Client *pClient,
                       MenuCursor *pCursor,
                       MenuMap &menus)
{
    ClientSettings settings;
    pClient->GetSettings(settings);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(&MatOrtho(0, settings.display.resolution.width,
                            0, settings.display.resolution.height,
                            -1.0f, 1.0f));

    glViewport(0, 0, settings.display.resolution.width,
                     settings.display.resolution.height);

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(&MatID());

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (auto pair : menus)
    {
        pair.second->Render();
    }

    pCursor->Render();
}
void MainMenuScene::Render(void)
{
    mBackGround->Render();

    RenderMenuOverlay(pClient, mCursor, mMenus);
}
MenuButton::MenuButton(Client *pCl, ButtonActionFunc func):
    pClient(pCl),
    mAction(func),
    bClicked(false),
    bTouched(false)
{
}
void MenuButton::OnLeave(void)
{
    bTouched = false;
}
void MenuButton::Update(const float dt)
{
    // A button might enter under the cursor.
    if (!bTouched && Clickable() && MouseOver())
    {
        OnTouch();

        bTouched = true;
    }
    else if (!MouseOver())
        bTouched = false;
}
bool MenuButton::OnMouseWheel(const SDL_MouseWheelEvent &event)
{
    // A button might be scrolled under the cursor.
    if (!bTouched && Clickable() && MouseOver())
    {
        OnTouch();

        bTouched = true;
    }
    else if (!MouseOver())
        bTouched = false;

    return false;
}
bool MenuButton::OnMouseMotion(const SDL_MouseMotionEvent &event)
{
    if (!bTouched && Clickable() && MouseOver())
    {
        OnTouch();

        bTouched = true;
    }
    else if (!MouseOver())
        bTouched = false;

    return false;
}
bool MenuButton::OnMouseButtonDown(const SDL_MouseButtonEvent &event)
{
    if (Clickable() && MouseOver() &&
        event.button == SDL_BUTTON_LEFT)
    {
        bClicked = true;
        OnClick();

        mAction(this);

        return true;
    }

    return false;
}
bool MenuButton::IsClicked(void)
{
    return bClicked;
}
void MenuButton::UnClick(void)
{
    bClicked = false;
}
void MenuButton::SetClicked(void)
{
    bClicked = true;
}
AnimatedMenuButton424x96::AnimatedMenuButton424x96(Client *pClient,
                                                   ButtonActionFunc func,
                                                   const std::shared_ptr<Pos2Provider> pRest,
                                                   const std::shared_ptr<Pos2Provider> pAway,
                                                   const std::shared_ptr<StringProvider> text,
                                                   const bool spammable):
    AnimatedMenuButton(pClient, func, pRest, pAway, text, spammable,
                       424.0f, 96.0f,
                       pClient->GetResourceManager()->GetTexture("button-424x96-plain"),
                       pClient->GetResourceManager()->GetTexture("button-424x96-touched"),
                       pClient->GetResourceManager()->GetTexture("button-424x96-clicked"),
                       pClient->GetResourceManager()->GetTexture("button-424x96-disabled"))
{
}
AnimatedMenuButton164x96::AnimatedMenuButton164x96(Client *pClient,
                                                   ButtonActionFunc func,
                                                   const std::shared_ptr<Pos2Provider> pRest,
                                                   const std::shared_ptr<Pos2Provider> pAway,
                                                   const std::shared_ptr<StringProvider> text,
                                                   const bool spammable):
    AnimatedMenuButton(pClient, func, pRest, pAway, text, spammable,
                       164.0f, 96.0f,
                       pClient->GetResourceManager()->GetTexture("button-164x96-plain"),
                       pClient->GetResourceManager()->GetTexture("button-164x96-touched"),
                       pClient->GetResourceManager()->GetTexture("button-164x96-clicked"),
                       pClient->GetResourceManager()->GetTexture("button-164x96-disabled"))
{
}
#define BUTTON_ARROW_WIDTH 58.0f
#define BUTTON_ARROW_HEIGHT 72.0f
AnimatedMenuButtonArrowRight::AnimatedMenuButtonArrowRight(Client *pClient,
                                                           ButtonActionFunc func,
                                                           const std::shared_ptr<Pos2Provider> pRest,
                                                           const std::shared_ptr<Pos2Provider> pAway,
                                                           const bool spammable):
    AnimatedMenuButton(pClient, func, pRest, pAway, std::make_shared<ConstantStringProvider>(""), spammable,
                       BUTTON_ARROW_WIDTH, BUTTON_ARROW_HEIGHT,
                       pClient->GetResourceManager()->GetTexture("button-arrowright-plain"),
                       pClient->GetResourceManager()->GetTexture("button-arrowright-touched"),
                       pClient->GetResourceManager()->GetTexture("button-arrowright-clicked"),
                       pClient->GetResourceManager()->GetTexture("button-arrowright-disabled"))
{
}
AnimatedMenuButtonArrowLeft::AnimatedMenuButtonArrowLeft(Client *pClient,
                                                         ButtonActionFunc func,
                                                         const std::shared_ptr<Pos2Provider> pRest,
                                                         const std::shared_ptr<Pos2Provider> pAway,
                                                         const bool spammable):
    AnimatedMenuButton(pClient, func, pRest, pAway, std::make_shared<ConstantStringProvider>(""), spammable,
                       BUTTON_ARROW_WIDTH, BUTTON_ARROW_HEIGHT,
                       pClient->GetResourceManager()->GetTexture("button-arrowleft-plain"),
                       pClient->GetResourceManager()->GetTexture("button-arrowleft-touched"),
                       pClient->GetResourceManager()->GetTexture("button-arrowleft-clicked"),
                       pClient->GetResourceManager()->GetTexture("button-arrowleft-disabled"))
{
}
AnimatedMenuButtonArrowUp::AnimatedMenuButtonArrowUp(Client *pClient,
                                                     ButtonActionFunc func,
                                                     const std::shared_ptr<Pos2Provider> pRest,
                                                     const std::shared_ptr<Pos2Provider> pAway,
                                                     const bool spammable):
    AnimatedMenuButton(pClient, func, pRest, pAway, std::make_shared<ConstantStringProvider>(""), spammable,
                       BUTTON_ARROW_HEIGHT, BUTTON_ARROW_WIDTH,
                       pClient->GetResourceManager()->GetTexture("button-arrowup-plain"),
                       pClient->GetResourceManager()->GetTexture("button-arrowup-touched"),
                       pClient->GetResourceManager()->GetTexture("button-arrowup-clicked"),
                       pClient->GetResourceManager()->GetTexture("button-arrowup-disabled"))
{
}
AnimatedMenuButtonArrowDown::AnimatedMenuButtonArrowDown(Client *pClient,
                                                         ButtonActionFunc func,
                                                         const std::shared_ptr<Pos2Provider> pRest,
                                                         const std::shared_ptr<Pos2Provider> pAway,
                                                         const bool spammable):
    AnimatedMenuButton(pClient, func, pRest, pAway, std::make_shared<ConstantStringProvider>(""), spammable,
                       BUTTON_ARROW_HEIGHT, BUTTON_ARROW_WIDTH,
                       pClient->GetResourceManager()->GetTexture("button-arrowdown-plain"),
                       pClient->GetResourceManager()->GetTexture("button-arrowdown-touched"),
                       pClient->GetResourceManager()->GetTexture("button-arrowdown-clicked"),
                       pClient->GetResourceManager()->GetTexture("button-arrowdown-disabled"))
{
}
#define BUTTON_ANIM_DURATION 0.5f
#define BUTTON_APPEAR_DURATION 0.3f
AnimatedMenuButton::AnimatedMenuButton(Client *pClient, ButtonActionFunc func,
                                       const std::shared_ptr<Pos2Provider> pRest,
                                       const std::shared_ptr<Pos2Provider> pAway,
                                       const std::shared_ptr<StringProvider> text,
                                       const bool spammable,
                                       const GLfloat width, const GLfloat height,
                                       ResourceReference<Texture> texPlain,
                                       ResourceReference<Texture> texTouched,
                                       ResourceReference<Texture> texClicked,
                                       ResourceReference<Texture> texDisabled):
    MenuButton(pClient, func),
    pRestPos2Provider(pRest),
    pAwayPos2Provider(pAway),
    pTextProvider(text),
    bSpammable(spammable),
    mAnimTime(BUTTON_ANIM_DURATION),
    mAppearTime(BUTTON_APPEAR_DURATION),
    mWidth(width),
    mHeight(height),
    rButtonFont(pClient->GetResourceManager()->GetFont("tiki-font-button")),
    rTexPlain(texPlain),
    rTexTouched(texTouched),
    rTexClicked(texClicked),
    rTexDisabled(texDisabled)
{
}
void AnimatedMenuButton::SetTextProvider(const std::shared_ptr<StringProvider> p)
{
    pTextProvider = p;
}
void AnimatedMenuButton::Render(void)
{
    ClientSettings settings;
    pClient->GetSettings(settings);
    ScreenResolution resolution = settings.display.resolution;

    GLfloat lh = GetLineSpacing(rButtonFont),
            amp,
            angle = 0.0f,
            fAppear = std::min(1.0f, mAppearTime / BUTTON_APPEAR_DURATION);
    bool mirrorX = mWidth < 0.0f,
         mirrorY = mHeight < 0.0f;

    if (IsClicked())
    {
        amp = 10.0f * std::max(0.0f, 1.0f - mAnimTime / BUTTON_ANIM_DURATION);
        angle = amp * sin(25.0f * mAnimTime);
    }
    else
    {
        amp = 3.0f * std::max(0.0f, 1.0f - mAnimTime / BUTTON_ANIM_DURATION);
        angle = amp * sin(25.0f * mAnimTime);
    }

    vec2 pos;
    if (IsActive())
    {
        pos = pRestPos2Provider->GetValue() * fAppear +
              pAwayPos2Provider->GetValue() * (1.0f - fAppear);
    }
    else
    {
        pos = pRestPos2Provider->GetValue() * (1.0f - fAppear) +
              pAwayPos2Provider->GetValue() * fAppear;
    }

    glTranslatef(pos.x, pos.y, 0.0f);
    glRotatef(angle, 0.0f, 0.0f, 1.0f);
    glScalef(mirrorX ? -1.0f : 1.0f, mirrorY ? -1.0f : 1.0f, 1.0f);

    ResourceReference<Texture> rTex;

    if (IsClicked())
        rTex = rTexClicked;
    else if (IsDisabled())
        rTex = rTexDisabled;
    else if (Clickable() && MouseOver())
        rTex = rTexTouched;
    else
        rTex = rTexPlain;

    RenderSprite(rTex, -rTex->w / 2, -rTex->h / 2, rTex->w, rTex->h);

    std::string text = pTextProvider->GetValue();
    if (text.size() > 0)
    {
        if (IsDisabled() && !IsClicked())
            glColor4f(0.5f, 0.5f, 1.0f, 1.0f);
        else
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        glTranslatef(0.0f, lh / 3, 0.0f);
        glRenderText(rButtonFont, text.c_str(), TEXTALIGN_MID);
        glTranslatef(0.0f, -lh / 3, 0.0f);

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }

    glScalef(mirrorX ? -1.0f : 1.0f, mirrorY ? -1.0f : 1.0f, 1.0f);
    glRotatef(-angle, 0.0f, 0.0f, 1.0f);
    glTranslatef(-pos.x, -pos.y, 0.0f);
}
void AnimatedMenuButton::OnClick(void)
{
    if (mAppearTime >= BUTTON_APPEAR_DURATION)
    {
        mAnimTime = 0.0f;
    }
}
void AnimatedMenuButton::OnTouch(void)
{
    if (mAppearTime >= BUTTON_APPEAR_DURATION)
    {
        mAnimTime = 0.0f;
    }
}
void AnimatedMenuButton::OnLeave(void)
{
    MenuButton::OnLeave();

    if (Ready())
    {
        mAppearTime = 0.0f;
    }
}
void AnimatedMenuButton::OnEnter(void)
{
    if (Ready())
    {
        mAppearTime = 0.0f;
    }

    UnClick();
}
bool AnimatedMenuButton::Clickable(void)
{
    if (!IsActive())
        return false;

    if (IsDisabled())
        return false;

    if (mAppearTime < BUTTON_APPEAR_DURATION)
        return false;

    if (!pClient->HasMouseFocus())
        return false;

    if (IsClicked() && !bSpammable)
        return false;

    return true;
}
bool AnimatedMenuButton::MouseOver(void) const
{
    ClientSettings settings;
    pClient->GetSettings(settings);
    ScreenResolution resolution = settings.display.resolution;

    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    GLfloat w = mWidth,
            h = mHeight;
    if (w < 0.0f)
        w = -w;
    if (h < 0.0f)
        h = -h;

    vec2 pos = pRestPos2Provider->GetValue();

    if (mouseX > (pos.x - w / 2) &&
        mouseX < (pos.x + w / 2) &&
        mouseY > (pos.y - h / 2) &&
        mouseY < (pos.y + h / 2))

        return true;

    return false;
}
void AnimatedMenuButton::Update(const float dt)
{
    MenuButton::Update(dt);

    mAnimTime += dt;
    mAppearTime += dt;

    if (bSpammable && mAnimTime >= BUTTON_ANIM_DURATION)
        UnClick();
}
bool AnimatedMenuButton::Ready(void) const
{
    return mAnimTime >= BUTTON_ANIM_DURATION &&
           mAppearTime >= BUTTON_APPEAR_DURATION;
}
MenuLabel::MenuLabel(Client *pCl):
    pClient(pCl)
{
}
bool MenuLabel::MouseOver(void) const
{
    return false;
}
AnimatedTextLabel::AnimatedTextLabel(Client *pCl,
                                     const std::shared_ptr<Pos2Provider> pRest,
                                     const std::shared_ptr<Pos2Provider> pAway,
                                     const std::shared_ptr<StringProvider> text):
    MenuLabel(pCl),
    pTextProvider(text),
    pRestPos2Provider(pRest),
    pAwayPos2Provider(pAway),
    mAnimTime(BUTTON_ANIM_DURATION),
    mAppearTime(BUTTON_APPEAR_DURATION),
    rButtonFont(pClient->GetResourceManager()->GetFont("tiki-font-button"))
{
}
void AnimatedTextLabel::OnLeave(void)
{
    if (Ready())
    {
        mAppearTime = 0.0f;
    }
}
void AnimatedTextLabel::OnEnter(void)
{
    if (Ready())
    {
        mAppearTime = 0.0f;
    }
}
void AnimatedTextLabel::Render(void)
{
    ClientSettings settings;
    pClient->GetSettings(settings);
    ScreenResolution resolution = settings.display.resolution;

    GLfloat lh = GetLineSpacing(rButtonFont),
            amp,
            angle = 0.0f,
            fAppear = std::min(1.0f, mAppearTime / BUTTON_APPEAR_DURATION);

    amp = 3.0f * std::max(0.0f, 1.0f - mAnimTime / BUTTON_ANIM_DURATION);
    angle = amp * sin(25.0f * mAnimTime);

    vec2 pos;
    if (IsActive())
    {
        pos = pRestPos2Provider->GetValue() * fAppear +
              pAwayPos2Provider->GetValue() * (1.0f - fAppear);
    }
    else
    {
        pos = pRestPos2Provider->GetValue() * (1.0f - fAppear) +
              pAwayPos2Provider->GetValue() * fAppear;
    }

    glTranslatef(pos.x, pos.y, 0.0f);
    glRotatef(angle, 0.0f, 0.0f, 1.0f);

    glTranslatef(0.0f, lh / 3, 0.0f);
    glRenderText(rButtonFont, pTextProvider->GetValue().c_str(), TEXTALIGN_MID);
    glTranslatef(0.0f, -lh / 3, 0.0f);

    glRotatef(-angle, 0.0f, 0.0f, 1.0f);
    glTranslatef(-pos.x, -pos.y, 0.0f);
}
void AnimatedTextLabel::Update(const float dt)
{
    mAnimTime += dt;
    mAppearTime += dt;
}
bool AnimatedTextLabel::Ready(void) const
{
    return mAnimTime >= BUTTON_ANIM_DURATION &&
           mAppearTime >= BUTTON_APPEAR_DURATION;
}
void AnimatedTextLabel::Animate(void)
{
    mAnimTime = 0.0f;
}
void AnimatedTextLabel::SetTextProvider(const std::shared_ptr<StringProvider> p)
{
    pTextProvider = p;
}
VerticalScrollPos2Provider::VerticalScrollPos2Provider(const std::shared_ptr<Pos2Provider> pProv,
                                                               const VerticalScrollable *pScroll):
    pScrollable(pScroll),
    pPos2Provider(pProv)
{
}
vec2 VerticalScrollPos2Provider::GetValue(void) const
{
    vec2 pos = pPos2Provider->GetValue();
    return {pos.x, pos.y + pScrollable->GetDisplacement()};
}
VerticalScrollable::~VerticalScrollable(void)
{
}
VerticalScrollable::VerticalScrollable(const std::shared_ptr<Pos2Provider> pTpLft,
                                       const std::shared_ptr<Pos2Provider> pBttmRght,
                                       const GLfloat scrolledYMin,
                                       const GLfloat scrolledYMax):
    mMinScrolledY(scrolledYMin),
    mMaxScrolledY(scrolledYMax),
    mY(0.0f),
    mScrollSpeed(0.0f),
    pTopLeft(pTpLft),
    pBottomRight(pBttmRght)
{
}
GLfloat VerticalScrollable::GetDisplacement(void) const
{
    return mY;
}
bool VerticalScrollable::MouseOver(void) const
{
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    vec2 topLeft = pTopLeft->GetValue(),
         bottomRight = pBottomRight->GetValue();

    if (mouseX > topLeft.x &&
        mouseX < bottomRight.x &&
        mouseY > topLeft.y &&
        mouseY < bottomRight.y)
        return true;

    return false;
}
void VerticalScrollable::Scroll(GLfloat amount)
{
    mScrollSpeed = amount;
}
#define SCROLL_SLOWDOWN 250.0f
void VerticalScrollable::Update(const float dt)
{
    GLfloat newDY = mY + mScrollSpeed * dt,
            scrolledHeight = fabs(mMaxScrolledY - mMinScrolledY),
            frameHeight = fabs(pBottomRight->GetValue().y - pTopLeft->GetValue().y);

    if (scrolledHeight < frameHeight)
    {
        mScrollSpeed = 0.0f;
        newDY = 0.0f;
    }
    else if ((mMaxScrolledY + newDY) < pBottomRight->GetValue().y)
    {
        mScrollSpeed = 0.0f;
        newDY = pBottomRight->GetValue().y - mMaxScrolledY;
    }
    else if ((mMinScrolledY + newDY) > pTopLeft->GetValue().y)
    {
        mScrollSpeed = 0.0f;
        newDY = pTopLeft->GetValue().y - mMinScrolledY;
    }

    mY = newDY;

    if (mScrollSpeed > 0.0f)
        mScrollSpeed = std::max(0.0f, mScrollSpeed - SCROLL_SLOWDOWN * dt);
    else
        mScrollSpeed = std::min(0.0f, mScrollSpeed + SCROLL_SLOWDOWN * dt);
}
void VerticalScrollable::Render(void)
{
}
bool VerticalScrollable::OnMouseWheel(const SDL_MouseWheelEvent &event)
{
    if (MouseOver())
    {
        Scroll(250 * event.y);
        return true;
    }
    return false;
}
bool VerticalScrollable::Ready(void) const
{
    return true;
}
