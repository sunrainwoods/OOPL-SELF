#ifndef KNIFE_WEAPON_HPP
#define KNIFE_WEAPON_HPP

#include <memory>
#include <vector>
#include <string>
#include <glm/vec2.hpp>
#include "Weapon.hpp"
#include "Util/GameObject.hpp"
#include "Util/Image.hpp"

struct KnifeProjectile {
    std::shared_ptr<Util::GameObject> object;
    glm::vec2 worldPosition = {0.0f, 0.0f};
    glm::vec2 velocity = {0.0f, 0.0f};
    bool active = false;
    float timeToLiveMs = 0.0f;
    float angle = 0.0f;
};

class KnifeWeapon : public Weapon {
public:
    KnifeWeapon();

    void Init(const std::string& resourceDir, const Player& player);
    void Reset() override;
    void Update(float dtMs, Player& player, const Camera& camera,
                EnemyManager& enemyManager,
                const std::function<void(Enemy&)>& onEnemyKilled) override;
    void Draw(const Camera& camera) override;

    bool IsUnlocked() const override { return m_Unlocked; }
    void SetUnlocked(bool unlocked) override { m_Unlocked = unlocked; }

    bool CanUpgrade(UpgradeType type) const override;
    void ApplyUpgrade(UpgradeType type, const Player& player) override;

    int GetCountLevel() const { return m_CountLevel; }
    int GetDamageLevel() const { return m_DamageLevel; }
    int GetCooldownLevel() const { return m_CooldownLevel; }
    int GetCount() const { return m_KnifeCount; }
    float GetDamage() const { return m_Damage; }
    float GetAttackIntervalMs() const { return m_AttackIntervalMs; }

private:
    bool m_Unlocked = false;
    std::vector<KnifeProjectile> m_Knives;
    int m_MaxKnives = 50;
    int m_KnifeCount = 1;
    std::shared_ptr<Util::Image> m_KnifeImage;

    float m_AttackIntervalMs = 800.0f;
    float m_AttackTimerMs = 0.0f;
    float m_Speed = 600.0f;
    float m_Damage = 80.0f;

    int m_CountLevel = 1;
    int m_DamageLevel = 1;
    int m_CooldownLevel = 1;
};

#endif // KNIFE_WEAPON_HPP
