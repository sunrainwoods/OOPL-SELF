#include "Whip.hpp"
#include "Player.hpp"
#include "Camera.hpp"
#include "EnemyManager.hpp"
#include <algorithm>

Whip::Whip() = default;

void Whip::Init(const std::string& resourceDir, const Player& player) {
    std::vector<std::string> weaponRightFrames;
    std::vector<std::string> weaponLeftFrames;
    weaponRightFrames.reserve(5);
    weaponLeftFrames.reserve(5);
    for (int i = 1; i <= 5; ++i) {
        weaponRightFrames.push_back(resourceDir + "/right_slash" + std::to_string(i) + ".png");
        weaponLeftFrames.push_back(resourceDir + "/left_slash" + std::to_string(i) + ".png");
    }

    m_RightAnimation = std::make_shared<Util::Animation>(weaponRightFrames, false, 50, false, 0);
    m_LeftAnimation = std::make_shared<Util::Animation>(weaponLeftFrames, false, 50, false, 0);
    m_ActiveAnimation = m_RightAnimation;

    m_Object = std::make_shared<Util::GameObject>();
    m_Object->SetDrawable(m_ActiveAnimation);
    m_Object->SetZIndex(7.0f);
    m_Object->SetVisible(false);

    UpdateScale(player);
}

void Whip::UpdateScale(const Player& player) {
    if (!m_ActiveAnimation || !m_Object) return;
    const glm::vec2 playerSize = player.GetScaledSize();
    const glm::vec2 weaponNativeSize = m_ActiveAnimation->GetSize();
    const float targetWeaponWidth = playerSize.x * m_WidthRatioToPlayer * (m_HitRadiusRatioToPlayer / 1.0f);
    const float weaponScale = targetWeaponWidth / weaponNativeSize.x;
    m_Object->m_Transform.scale = {weaponScale, weaponScale};
}

void Whip::Reset() {
    m_Unlocked = true;
    m_AttackIntervalMs = 600.0f;
    m_AttackTimerMs = 0.0f;
    m_Damage = 100.0f;
    m_HitRadiusRatioToPlayer = 1.0f;
    m_WidthRatioToPlayer = 1.5f;

    m_RangeLevel = 1;
    m_DamageLevel = 1;
    m_CooldownLevel = 1;

    if (m_Object) {
        m_Object->SetVisible(false);
    }
}

void Whip::Update(float dtMs, Player& player, const Camera& camera,
                  EnemyManager& enemyManager,
                  const std::function<void(Enemy&)>& onEnemyKilled) {
    if (!m_Unlocked) return;

    bool isFacingLeft = player.IsFacingLeft();

    // Direction swap synchronization during attack
    if (m_ActiveAnimation && m_ActiveAnimation->GetState() == Util::Animation::State::PLAY) {
        auto correctAnimation = isFacingLeft ? m_LeftAnimation : m_RightAnimation;
        if (m_ActiveAnimation != correctAnimation) {
            std::size_t currentFrame = m_ActiveAnimation->GetCurrentFrameIndex();
            m_ActiveAnimation->Pause();
            m_ActiveAnimation = correctAnimation;
            m_Object->SetDrawable(m_ActiveAnimation);
            m_ActiveAnimation->SetCurrentFrame(currentFrame);
            m_ActiveAnimation->Play();
        }
    }

    m_AttackTimerMs += dtMs;
    if (m_AttackTimerMs >= m_AttackIntervalMs) {
        m_AttackTimerMs = 0.0f;
        m_ActiveAnimation = isFacingLeft ? m_LeftAnimation : m_RightAnimation;
        m_Object->SetDrawable(m_ActiveAnimation);
        m_ActiveAnimation->SetCurrentFrame(0);
        m_ActiveAnimation->Play();
        m_Object->SetVisible(true);
    }

    if (m_ActiveAnimation && m_ActiveAnimation->GetState() == Util::Animation::State::ENDED) {
        m_Object->SetVisible(false);
    }

    // Translation offset
    const glm::vec2 playerSize = player.GetScaledSize();
    const float sideOffset = playerSize.x * 0.7f;
    const glm::vec2 weaponOffset = isFacingLeft ? glm::vec2{-sideOffset, 0.0f} : glm::vec2{sideOffset, 0.0f};

    m_Object->m_Transform.translation = camera.WorldToScreen(player.GetWorldPosition() + weaponOffset);

    // Hit detection
    bool weaponCanHit = false;
    if (m_ActiveAnimation && m_ActiveAnimation->GetState() == Util::Animation::State::PLAY) {
        const std::size_t currentFrame = m_ActiveAnimation->GetCurrentFrameIndex();
        weaponCanHit = (currentFrame >= m_HitStartFrame && currentFrame <= m_HitEndFrame);
    }

    if (weaponCanHit) {
        const glm::vec2 weaponHitCenter = player.GetWorldPosition() + weaponOffset;
        const float hitRadius = playerSize.x * m_HitRadiusRatioToPlayer;
        const float hitRadiusSq = hitRadius * hitRadius;

        std::vector<Enemy*> activeEnemies = enemyManager.GetActiveEnemies();
        for (auto* enemyPtr : activeEnemies) {
            auto& enemy = *enemyPtr;
            if (enemy.IsActive()) {
                glm::vec2 diff = enemy.GetWorldPosition() - weaponHitCenter;
                float distSq = diff.x * diff.x + diff.y * diff.y;

                if (distSq <= hitRadiusSq && enemy.GetHitCooldownTimerMs() <= 0.0f) {
                    enemy.TakeDamage(m_Damage);
                    if (enemy.IsDead()) {
                        player.Heal(player.GetMaxHealth() * player.GetVampirism());
                        onEnemyKilled(enemy);
                    }
                }
            }
        }
    }
}

void Whip::Draw(const Camera&) {
    if (m_Unlocked && m_Object) {
        m_Object->Draw();
    }
}

bool Whip::CanUpgrade(UpgradeType type) const {
    if (type == UpgradeType::WHIP_RANGE) return m_RangeLevel < 15;
    if (type == UpgradeType::WHIP_DAMAGE) return m_DamageLevel < 15;
    if (type == UpgradeType::WHIP_COOLDOWN) return m_CooldownLevel < 15;
    return false;
}

void Whip::ApplyUpgrade(UpgradeType type, const Player& player) {
    if (type == UpgradeType::WHIP_RANGE) {
        m_RangeLevel++;
        m_HitRadiusRatioToPlayer *= 1.20f;
        UpdateScale(player);
    } else if (type == UpgradeType::WHIP_DAMAGE) {
        m_DamageLevel++;
        m_Damage += 15.0f;
    } else if (type == UpgradeType::WHIP_COOLDOWN) {
        m_CooldownLevel++;
        m_AttackIntervalMs *= 0.80f;
    }
}
