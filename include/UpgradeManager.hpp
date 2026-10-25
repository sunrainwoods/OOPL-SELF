#ifndef UPGRADE_MANAGER_HPP
#define UPGRADE_MANAGER_HPP

#include <vector>
#include "Types.hpp"

class Player;
class WeaponManager;

class UpgradeManager {
public:
    UpgradeManager();

    void Reset();
    void GenerateUpgradeOptions(const Player& player, const WeaponManager& weaponManager);
    void ApplyUpgrade(UpgradeType type, Player& player, WeaponManager& weaponManager);

    const std::vector<UpgradeType>& GetCurrentOptions() const { return m_CurrentOptions; }
    int GetPendingLevelUps() const { return m_PendingLevelUps; }
    void AddPendingLevelUps(int count) { m_PendingLevelUps += count; }
    bool ConsumePendingLevelUp();

private:
    std::vector<UpgradeType> m_CurrentOptions;
    int m_PendingLevelUps = 0;
};

#endif // UPGRADE_MANAGER_HPP
