#ifndef WEAPON_HPP
#define WEAPON_HPP

#include <string>
#include <functional>
#include "Types.hpp"

class Player;
class Camera;
class EnemyManager;
class Enemy;

class Weapon {
public:
    virtual ~Weapon() = default;

    virtual void Reset() = 0;
    virtual void Update(float dtMs, Player& player, const Camera& camera,
                        EnemyManager& enemyManager,
                        const std::function<void(Enemy&)>& onEnemyKilled) = 0;
    virtual void Draw(const Camera& camera) = 0;

    virtual bool IsUnlocked() const = 0;
    virtual void SetUnlocked(bool unlocked) = 0;

    virtual bool CanUpgrade(UpgradeType type) const = 0;
    virtual void ApplyUpgrade(UpgradeType type, const Player& player) = 0;
};

#endif // WEAPON_HPP
