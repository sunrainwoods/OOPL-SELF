#ifndef UI_MANAGER_HPP
#define UI_MANAGER_HPP

#include <memory>
#include <string>
#include <vector>
#include <functional>

#include "Types.hpp"
#include "Util/Image.hpp"
#include "Util/GameObject.hpp"

class Player;
class WeaponManager;

class UIManager {
public:
    UIManager();

    void Init(const std::string& resourceDir);

    void DrawTitleScreen(std::shared_ptr<Util::GameObject>& titleScreen,
                         std::shared_ptr<Util::Image>& titleImage,
                         bool& startRequested);

    void DrawHUD(const Player& player, int defeated, float gameTimeMs, bool& pauseRequested);

    void DrawPausedMenu(const Player& player, const WeaponManager& weaponManager,
                        int defeated, int wave, int stage, float gameTimeMs,
                        bool& continueRequested, bool& exitRequested);

    void DrawGameOverMenu(const Player& player, const WeaponManager& weaponManager,
                          int defeated, int wave, int stage, float gameTimeMs,
                          bool& restartRequested, bool& exitRequested);

    void DrawLevelUpMenu(const std::vector<UpgradeType>& options,
                         const Player& player, const WeaponManager& weaponManager,
                         const std::function<void(UpgradeType)>& onSelectUpgrade);

private:
    void DrawStatsTables(const Player& player, const WeaponManager& weaponManager,
                         int defeated, int wave, int stage, float gameTimeMs,
                         float leftY, float rightY, float panelHeight);

    std::shared_ptr<Util::Image> m_PauseIconImage;
    std::shared_ptr<Util::Image> m_EnemyCountIconImage;
    std::shared_ptr<Util::Image> m_GameOverImage;
    std::shared_ptr<Util::Image> m_LevelUpImage;
    std::shared_ptr<Util::GameObject> m_LevelUpObject;
};

#endif // UI_MANAGER_HPP
