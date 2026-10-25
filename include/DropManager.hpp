#ifndef DROP_MANAGER_HPP
#define DROP_MANAGER_HPP

#include <vector>
#include <memory>
#include <string>
#include "DropItem.hpp"
#include "Util/Image.hpp"

class Player;
class Camera;
class Enemy;

class DropManager {
public:
    DropManager();

    void Init(const std::string& resourceDir, const Player& player);
    void Reset();

    void HandleEnemyDeath(const Enemy& enemy, const Player& player);
    void Update(float dtMs, Player& player, const Camera& camera,
                int& pendingLevelUps, bool& triggerLevelUp);
    void Draw(const Camera& camera);

private:
    std::shared_ptr<Util::Image> m_Gem1Image;
    std::shared_ptr<Util::Image> m_Gem2Image;
    std::shared_ptr<Util::Image> m_Gem3Image;
    std::shared_ptr<Util::Image> m_HealthImage;
    std::shared_ptr<Util::Image> m_MagnetImage;
    std::shared_ptr<Util::Image> m_RewardImage;

    std::vector<ExpGem> m_ExpGems;
    int m_MaxExpGems = 100;

    std::vector<HealthItem> m_HealthItems;
    int m_MaxHealthItems = 20;

    std::vector<MagnetItem> m_MagnetItems;
    int m_MaxMagnetItems = 20;
    float m_MagnetTimerMs = 0.0f;

    std::vector<RewardItem> m_RewardItems;
    int m_MaxRewardItems = 10;

    float m_ExpGemSizeRatioToPlayer = 0.6f;
};

#endif // DROP_MANAGER_HPP
