#ifndef WEAPON_MANAGER_HPP
#define WEAPON_MANAGER_HPP

#include <memory>
#include <vector>
#include <string>
#include <functional>

#include "Whip.hpp"
#include "KnifeWeapon.hpp"
#include "RunetracerWeapon.hpp"
#include "Types.hpp"

class Player;
class Camera;
class EnemyManager;
class Enemy;

class WeaponManager {
public:
    WeaponManager();

    void Init(const std::string& resourceDir, const Player& player);
    void Reset();
    void Update(float dtMs, Player& player, const Camera& camera,
                EnemyManager& enemyManager,
                const std::function<void(Enemy&)>& onEnemyKilled);
    void Draw(const Camera& camera);

    bool CanUpgrade(UpgradeType type) const;
    void ApplyUpgrade(UpgradeType type, const Player& player);

    std::shared_ptr<Whip> GetWhip() const { return m_Whip; }
    std::shared_ptr<KnifeWeapon> GetKnife() const { return m_Knife; }
    std::shared_ptr<RunetracerWeapon> GetRunetracer() const { return m_Runetracer; }

private:
    std::shared_ptr<Whip> m_Whip;
    std::shared_ptr<KnifeWeapon> m_Knife;
    std::shared_ptr<RunetracerWeapon> m_Runetracer;
};

#endif // WEAPON_MANAGER_HPP
