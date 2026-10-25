#ifndef APP_HPP
#define APP_HPP

#include <memory>
#include <string>

#include "Types.hpp"
#include "Camera.hpp"
#include "Player.hpp"
#include "Background.hpp"
#include "EnemyManager.hpp"
#include "WeaponManager.hpp"
#include "DropManager.hpp"
#include "UpgradeManager.hpp"
#include "UIManager.hpp"

#include "Util/BGM.hpp"
#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include "pch.hpp"

class App {
public:
    enum class State {
        TITLE,
        START,
        UPDATE,
        LEVEL_UP,
        GAME_OVER,
        PAUSED,
        END,
    };

    State GetCurrentState() const { return m_CurrentState; }

    void Init();
    void UpdateTitle();
    void Start();
    void Update();
    void UpdateLevelUp();
    void UpdateGameOver();
    void UpdatePaused();
    void End();

private:
    void DrawGameObjects();
    void HandleEnemyDeath(Enemy& enemy);

    State m_CurrentState = State::TITLE;

    std::shared_ptr<Util::GameObject> m_TitleScreen;
    std::shared_ptr<Util::Image> m_TitleImage;

    std::shared_ptr<Camera> m_Camera;
    std::shared_ptr<Player> m_Player;
    std::shared_ptr<Background> m_Background;
    std::shared_ptr<EnemyManager> m_EnemyManager;
    std::shared_ptr<WeaponManager> m_WeaponManager;
    std::shared_ptr<DropManager> m_DropManager;
    std::shared_ptr<UpgradeManager> m_UpgradeManager;
    std::shared_ptr<UIManager> m_UIManager;

    std::shared_ptr<Util::BGM> m_BGM;
    float m_GameTimeMs = 0.0f;
    bool m_Initialized = false;
};

#endif // APP_HPP
