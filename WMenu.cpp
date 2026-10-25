#include "WMenu.hpp"
#include "Settings.hpp"
#include <Geode/ui/BasedButtonSprite.hpp>

using namespace geode::prelude;

namespace {
    struct Row {
        char const* key;
        char const* name;
    };

    // the 4 "boring" features
    constexpr Row ROWS[] = {
        {w::K_FRAMECOUNT, "Frame Counter"},
        {w::K_CLICKCOUNT, "Click Counter"},
        {w::K_DIM, "Dim Screen"},
        {w::K_FLASH, "Click Flash"},
    };

    constexpr int CBFH_TAG = 100;

    CCScale9Sprite* panel(float width, float height, ccColor3B color, GLubyte opacity) {
        auto s = CCScale9Sprite::create("square02b_001.png", {0, 0, 80, 80});
        s->setContentSize({width, height});
        s->setColor(color);
        s->setOpacity(opacity);
        return s;
    }
}

CCNode* makeWIcon(float scale) {
    auto letter = CCLabelBMFont::create("W", "bigFont.fnt");
    auto spr = CircleButtonSprite::create(letter, CircleBaseColor::Green, CircleBaseSize::Medium);
    spr->setScale(scale);
    return spr;
}

WMenu* WMenu::create() {
    auto ret = new WMenu();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool WMenu::init() {
    if (!Popup::init(320.f, 290.f)) return false;
    this->setTitle("W Menu");

    auto sz = m_mainLayer->getContentSize();
    float top = sz.height;
    float cx = sz.width / 2;

    // ---- CBFH card (the main feature) ----
    auto card = panel(sz.width - 30, 104, {255, 140, 40}, 120);
    m_mainLayer->addChildAtPosition(card, Anchor::BottomLeft, {cx, top - 90});

    auto name = CCLabelBMFont::create("CBFH", "goldFont.fnt");
    name->setScale(0.8f);
    name->setAnchorPoint({0, 0.5f});
    m_mainLayer->addChildAtPosition(name, Anchor::BottomLeft, {28, top - 55});

    auto sub = CCLabelBMFont::create("Auto frame-perfect clicks", "bigFont.fnt");
    sub->setScale(0.32f);
    sub->setAnchorPoint({0, 0.5f});
    sub->setOpacity(200);
    m_mainLayer->addChildAtPosition(sub, Anchor::BottomLeft, {28, top - 75});

    auto cbfhToggle = CCMenuItemToggler::createWithStandardSprites(
        this, menu_selector(WMenu::onToggle), 0.8f
    );
    cbfhToggle->setTag(CBFH_TAG);
    cbfhToggle->toggle(w::on(w::K_CBFH));
    m_buttonMenu->addChildAtPosition(cbfhToggle, Anchor::BottomLeft, {sz.width - 42, top - 58});

    // frame window stepper: [<]  N frames  [>]
    auto leftSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
    leftSpr->setScale(0.8f);
    auto leftBtn = CCMenuItemSpriteExtra::create(leftSpr, this, menu_selector(WMenu::onStep));
    leftBtn->setTag(-1);
    m_buttonMenu->addChildAtPosition(leftBtn, Anchor::BottomLeft, {cx - 80, top - 108});

    auto rightSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
    rightSpr->setFlipX(true);
    rightSpr->setScale(0.8f);
    auto rightBtn = CCMenuItemSpriteExtra::create(rightSpr, this, menu_selector(WMenu::onStep));
    rightBtn->setTag(1);
    m_buttonMenu->addChildAtPosition(rightBtn, Anchor::BottomLeft, {cx + 80, top - 108});

    m_framesLabel = CCLabelBMFont::create("", "bigFont.fnt");
    m_framesLabel->setScale(0.6f);
    m_mainLayer->addChildAtPosition(m_framesLabel, Anchor::BottomLeft, {cx, top - 108});
    this->refreshFrames();

    // ---- the 4 boring toggles ----
    for (int i = 0; i < 4; ++i) {
        float y = top - 168.f - i * 30.f;

        m_mainLayer->addChildAtPosition(
            panel(sz.width - 30, 26, {0, 0, 0}, 90), Anchor::BottomLeft, {cx, y}
        );

        auto label = CCLabelBMFont::create(ROWS[i].name, "bigFont.fnt");
        label->setScale(0.45f);
        label->setAnchorPoint({0, 0.5f});
        m_mainLayer->addChildAtPosition(label, Anchor::BottomLeft, {28, y});

        auto toggle = CCMenuItemToggler::createWithStandardSprites(
            this, menu_selector(WMenu::onToggle), 0.6f
        );
        toggle->setTag(i);
        toggle->toggle(w::on(ROWS[i].key));
        m_buttonMenu->addChildAtPosition(toggle, Anchor::BottomLeft, {sz.width - 42, y});
    }

    return true;
}

void WMenu::onToggle(CCObject* sender) {
    auto toggle = static_cast<CCMenuItemToggler*>(sender);
    // isToggled() still holds the old state inside the callback
    bool newState = !toggle->isToggled();
    int tag = toggle->getTag();
    if (tag == CBFH_TAG) {
        w::set(w::K_CBFH, newState);
    } else if (tag >= 0 && tag < 4) {
        w::set(ROWS[tag].key, newState);
    }
}

void WMenu::onStep(CCObject* sender) {
    w::setFrames(w::frames() + static_cast<CCNode*>(sender)->getTag());
    this->refreshFrames();
}

void WMenu::refreshFrames() {
    int f = w::frames();
    m_framesLabel->setString(fmt::format("{} frame{}", f, f == 1 ? "" : "s").c_str());
}

void openWMenu() {
    if (auto popup = WMenu::create()) popup->show();
}
