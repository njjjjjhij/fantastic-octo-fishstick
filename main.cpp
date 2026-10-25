#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <algorithm>
#include <type_traits>
#include "Settings.hpp"
#include "WMenu.hpp"

using namespace geode::prelude;

namespace {
    // works whether m_touchingRings is a vector, a pointer to one, or a CCArray*
    template <class T>
    bool hasAny(T const& v) {
        if constexpr (std::is_pointer_v<T>) {
            if (!v) return false;
            if constexpr (requires { v->empty(); }) return !v->empty();
            else return v->count() > 0;
        } else {
            return !v.empty();
        }
    }

    // ship / ufo / wave / swing don't need a "can I jump right now" moment
    bool needsGround(PlayerObject* p) {
        return !(p->m_isShip || p->m_isBird || p->m_isDart || p->m_isSwing);
    }

    // a jump would actually register this step: on the ground or touching an orb
    bool canAct(PlayerObject* p) {
        return p->m_isOnGround || hasAny(p->m_touchingRings);
    }

    bool falling(PlayerObject* p) {
        double v = p->m_isUpsideDown ? -p->m_yVelocity : p->m_yVelocity;
        return v <= 0.0;
    }

    void flash() {
        if (!w::on(w::K_FLASH)) return;
        auto pl = PlayLayer::get();
        if (!pl || !pl->m_uiLayer) return;
        auto c = CCLayerColor::create({255, 255, 255, 70});
        c->setContentSize(CCDirector::get()->getWinSize());
        pl->m_uiLayer->addChild(c, -50);
        c->runAction(CCSequence::create(
            CCFadeOut::create(0.12f),
            CCCallFunc::create(c, callfunc_selector(CCNode::removeMeAndCleanup)),
            nullptr
        ));
    }
}

// ---------------- W button: main menu ----------------
// Use a text-based Geode ButtonSprite so the entry remains visible even if
// the custom circular icon fails to render on a particular mobile build.
class $modify(WMainMenu, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto sprite = ButtonSprite::create("W MENU");
        if (!sprite) {
            log::error("WMenu: could not create main-menu button sprite");
            return true;
        }

        auto btn = CCMenuItemSpriteExtra::create(
            sprite, this, menu_selector(WMainMenu::onW)
        );
        if (!btn) {
            log::error("WMenu: could not create main-menu button item");
            return true;
        }
        btn->setID("w-menu-button"_spr);

        auto win = CCDirector::get()->getWinSize();
        auto menu = CCMenu::create();
        if (!menu) return true;
        menu->setID("w-menu-container"_spr);
        menu->setPosition({0.f, 0.f});
        menu->setZOrder(9999);
        btn->setPosition({win.width - 62.f, win.height - 48.f});
        menu->addChild(btn);
        this->addChild(menu, 9999);

        log::info("WMenu: main-menu button added at {}, {}", win.width - 62.f, win.height - 48.f);
        return true;
    }

    void onW(CCObject*) { openWMenu(); }
};

// ---------------- W button: pause menu ----------------
class $modify(WPause, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto sprite = ButtonSprite::create("W MENU");
        if (!sprite) {
            log::error("WMenu: could not create pause-menu button sprite");
            return;
        }

        auto btn = CCMenuItemSpriteExtra::create(
            sprite, this, menu_selector(WPause::onW)
        );
        if (!btn) return;
        btn->setID("w-menu-button"_spr);

        auto win = CCDirector::get()->getWinSize();
        auto menu = CCMenu::create();
        if (!menu) return;
        menu->setID("w-menu-container"_spr);
        menu->setPosition({0.f, 0.f});
        menu->setZOrder(9999);
        btn->setPosition({win.width - 62.f, win.height - 48.f});
        menu->addChild(btn);
        this->addChild(menu, 9999);

        log::info("WMenu: pause-menu button added");
    }

    void onW(CCObject*) { openWMenu(); }
};

// ---------------- HUD + the 4 boring features ----------------
class $modify(WPlayLayer, PlayLayer) {
    struct Fields {
        CCLabelBMFont* hud = nullptr;
        CCLayerColor* dim = nullptr;
    };

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();

        auto win = CCDirector::get()->getWinSize();

        auto hud = CCLabelBMFont::create("", "bigFont.fnt");
        hud->setScale(0.4f);
        hud->setAnchorPoint({0.f, 1.f});
        hud->setPosition({8.f, win.height - 8.f});
        hud->setOpacity(170);
        m_uiLayer->addChild(hud, 100);
        m_fields->hud = hud;

        auto dim = CCLayerColor::create({0, 0, 0, 120});
        dim->setContentSize(win);
        m_uiLayer->addChild(dim, -100);
        m_fields->dim = dim;

        w::g_frames = 0;
        w::g_clicks = 0;
        w::g_fixed = 0;
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        if (m_fields->dim) m_fields->dim->setVisible(w::on(w::K_DIM));

        if (m_fields->hud) {
            std::string s;
            if (w::on(w::K_FRAMECOUNT)) s += fmt::format("Frame {}\n", w::g_frames);
            if (w::on(w::K_CLICKCOUNT)) s += fmt::format("Clicks {}\n", w::g_clicks);
            if (w::on(w::K_CBFH)) s += fmt::format("CBFH {}f  fixed {}", w::frames(), w::g_fixed);
            m_fields->hud->setString(s.c_str());
        }
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        w::g_frames = 0;
        w::g_clicks = 0;
        w::g_fixed = 0;
        w::g_reset = true;
    }
};

// ---------------- CBFH ----------------
//
// The player gets a window of N physics steps (1-20). When they click:
//  * already able to jump        -> pass the click straight through
//  * click came EARLY            -> hold it and fire on the first step the jump
//                                   can actually register (ground / orb), inside the window
//  * click came LATE (just left  -> best-effort: treat the player as still grounded
//    the ground, still falling)     if they walked off less than N steps ago
//
class $modify(CBFHLayer, GJBaseGameLayer) {
    struct Fields {
        bool pending = false;
        bool releaseQueued = false;
        bool firing = false;
        bool p1 = true;
        int left = 0;
        int releaseIn = 0;
        int sinceGround[2] = {999, 999};
    };

    void fireButton(bool down, bool isPlayer1) {
        m_fields->firing = true;
        this->handleButton(down, 1, isPlayer1);
        m_fields->firing = false;
    }

    void handleButton(bool down, int button, bool isPlayer1) {
        auto f = m_fields.self();

        if (f->firing || button != 1 || !PlayLayer::get()) {
            return GJBaseGameLayer::handleButton(down, button, isPlayer1);
        }

        if (down) {
            w::g_clicks++;
            flash();
        }

        if (!w::on(w::K_CBFH)) {
            return GJBaseGameLayer::handleButton(down, button, isPlayer1);
        }

        PlayerObject* p = isPlayer1 ? m_player1 : m_player2;

        if (down) {
            if (!p || !needsGround(p) || canAct(p)) {
                f->pending = false;
                return GJBaseGameLayer::handleButton(down, button, isPlayer1);
            }

            // late click: just walked off a ledge, still falling, not near an orb
            int sg = f->sinceGround[isPlayer1 ? 0 : 1];
            if (sg > 0 && sg <= w::frames() && falling(p)) {
                p->m_isOnGround = true;
                w::g_fixed++;
                return GJBaseGameLayer::handleButton(down, button, isPlayer1);
            }

            // early click: buffer it for up to N steps
            f->pending = true;
            f->releaseQueued = false;
            f->left = w::frames();
            f->p1 = isPlayer1;
            return;
        }

        // release while a click is still buffered: let it fire first, release after
        if (f->pending && f->p1 == isPlayer1) {
            f->releaseQueued = true;
            return;
        }

        return GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    void processCommands(float dt) {
        auto f = m_fields.self();

        if (w::g_reset) {
            w::g_reset = false;
            f->pending = false;
            f->releaseQueued = false;
            f->releaseIn = 0;
        }

        if (f->releaseIn > 0 && --f->releaseIn == 0) {
            this->fireButton(false, f->p1);
        }

        if (f->pending) {
            PlayerObject* p = f->p1 ? m_player1 : m_player2;
            bool ready = !p || canAct(p);
            if (ready || --f->left <= 0) {
                f->pending = false;
                if (ready) w::g_fixed++;
                this->fireButton(true, f->p1);
                if (f->releaseQueued) {
                    f->releaseQueued = false;
                    f->releaseIn = 2;
                }
            }
        }

        GJBaseGameLayer::processCommands(dt);

        w::g_frames++;

        for (int i = 0; i < 2; ++i) {
            PlayerObject* p = i == 0 ? m_player1 : m_player2;
            if (!p) continue;
            f->sinceGround[i] = p->m_isOnGround ? 0 : std::min(f->sinceGround[i] + 1, 999);
        }
    }
};
