#ifndef WHIP_HPP
#define WHIP_HPP

#include <memory>
#include <string>
#include <vector>
#include "Weapon.hpp"
#include "Util/GameObject.hpp"
#include "Util/Animation.hpp"

class Whip : public Weapon {
public:
    Whip();

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

    int GetRangeLevel() const { return m_RangeLevel; }
    int GetDamageLevel() const { return m_DamageLevel; }
    int GetCooldownLevel() const { return m_CooldownLevel; }
    float GetDamage() const { return m_Damage; }
    float GetAttackIntervalMs() const { return m_AttackIntervalMs; }
    float GetHitRadiusRatio() const { return m_HitRadiusRatioToPlayer; }

private:
    void UpdateScale(const Player& player);

    bool m_Unlocked = true;
    std::shared_ptr<Util::GameObject> m_Object;
    std::shared_ptr<Util::Animation> m_RightAnimation;
    std::shared_ptr<Util::Animation> m_LeftAnimation;
    std::shared_ptr<Util::Animation> m_ActiveAnimation;

    float m_AttackTimerMs = 0.0f;
    float m_AttackIntervalMs = 600.0f;
    float m_Damage = 100.0f;
    float m_HitRadiusRatioToPlayer = 1.0f;
    float m_WidthRatioToPlayer = 1.5f;

    std::size_t m_HitStartFrame = 2;
    std::size_t m_HitEndFrame = 4;

    int m_RangeLevel = 1;
    int m_DamageLevel = 1;
    int m_CooldownLevel = 1;
};

#endif // WHIP_HPP
