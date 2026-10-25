#include "KnifeWeapon.hpp"
#include "Player.hpp"
#include "Camera.hpp"
#include "EnemyManager.hpp"
#include <cmath>

KnifeWeapon::KnifeWeapon() = default;

void KnifeWeapon::Init(const std::string& resourceDir, const Player& player) {
    m_KnifeImage = std::make_shared<Util::Image>(resourceDir + "/knife.png");
    m_Knives.clear();
    m_Knives.reserve(m_MaxKnives);

    const glm::vec2 playerSize = player.GetScaledSize();
    const float targetKnifeWidth = playerSize.x * 0.65f;
    const float knifeScale = targetKnifeWidth / m_KnifeImage->GetSize().x;

    for (int i = 0; i < m_MaxKnives; ++i) {
        KnifeProjectile knife;
        knife.object = std::make_shared<Util::GameObject>();
        knife.object->SetDrawable(m_KnifeImage);
        knife.object->SetZIndex(5.0f);
        knife.object->SetVisible(false);
        knife.object->m_Transform.scale = {knifeScale, knifeScale};
        knife.active = false;
        m_Knives.push_back(knife);
    }
}

void KnifeWeapon::Reset() {
    m_Unlocked = false;
    m_AttackIntervalMs = 800.0f;
    m_AttackTimerMs = 0.0f;
    m_Speed = 600.0f;
    m_Damage = 80.0f;
    m_KnifeCount = 1;

    m_CountLevel = 1;
    m_DamageLevel = 1;
    m_CooldownLevel = 1;

    for (auto& knife : m_Knives) {
        knife.active = false;
        if (knife.object) {
            knife.object->SetVisible(false);
        }
    }
}

void KnifeWeapon::Update(float dtMs, Player& player, const Camera& camera,
                         EnemyManager& enemyManager,
                         const std::function<void(Enemy&)>& onEnemyKilled) {
    if (!m_Unlocked) return;

    m_AttackTimerMs += dtMs;
    if (m_AttackTimerMs >= m_AttackIntervalMs) {
        m_AttackTimerMs = 0.0f;

        int spawned = 0;
        glm::vec2 moveDir = player.GetLastMoveDir();
        float baseAngle = std::atan2(moveDir.y, moveDir.x);
        float angleStep = 0.15f;
        float startAngle = baseAngle - (m_KnifeCount - 1) * angleStep * 0.5f;

        for (auto& knife : m_Knives) {
            if (!knife.active) {
                knife.active = true;
                knife.worldPosition = player.GetWorldPosition();
                knife.angle = startAngle + spawned * angleStep;
                knife.velocity = glm::vec2(std::cos(knife.angle), std::sin(knife.angle)) * m_Speed;
                knife.timeToLiveMs = 2000.0f;

                knife.object->m_Transform.rotation = knife.angle;
                knife.object->SetVisible(true);

                spawned++;
                if (spawned >= m_KnifeCount) break;
            }
        }
    }

    // Movement & Collision
    const float dtSec = dtMs / 1000.0f;
    const float knifeHitRadius = player.GetScaledSize().x * 0.4f * 0.5f;
    std::vector<Enemy*> activeEnemies = enemyManager.GetActiveEnemies();

    for (auto& knife : m_Knives) {
        if (knife.active) {
            knife.worldPosition += knife.velocity * dtSec;
            knife.object->m_Transform.translation = camera.WorldToScreen(knife.worldPosition);

            knife.timeToLiveMs -= dtMs;
            if (knife.timeToLiveMs <= 0.0f) {
                knife.active = false;
                knife.object->SetVisible(false);
                continue;
            }

            // Collision against enemies
            for (auto* enemyPtr : activeEnemies) {
                auto& enemy = *enemyPtr;
                if (enemy.IsActive() && enemy.GetHitCooldownTimerMs() <= 0.0f) {
                    float hitRadius = knifeHitRadius + enemy.GetCollisionRadius();
                    glm::vec2 diff = enemy.GetWorldPosition() - knife.worldPosition;

                    if ((diff.x * diff.x + diff.y * diff.y) <= (hitRadius * hitRadius)) {
                        enemy.TakeDamage(m_Damage);
                        player.ApplyLifesteal(m_Damage);

                        knife.active = false;
                        knife.object->SetVisible(false);

                        if (enemy.IsDead()) {
                            onEnemyKilled(enemy);
                        }
                        break;
                    }
                }
            }
        }
    }
}

void KnifeWeapon::Draw(const Camera& camera) {
    if (!m_Unlocked) return;

    for (auto& knife : m_Knives) {
        if (knife.active && camera.IsInView(knife.worldPosition)) {
            knife.object->Draw();
        }
    }
}

bool KnifeWeapon::CanUpgrade(UpgradeType type) const {
    if (type == UpgradeType::UNLOCK_KNIFE) return !m_Unlocked;
    if (!m_Unlocked) return false;
    if (type == UpgradeType::KNIFE_COUNT) return m_CountLevel < 15;
    if (type == UpgradeType::KNIFE_DAMAGE) return m_DamageLevel < 15;
    if (type == UpgradeType::KNIFE_COOLDOWN) return m_CooldownLevel < 15;
    return false;
}

void KnifeWeapon::ApplyUpgrade(UpgradeType type, const Player&) {
    if (type == UpgradeType::UNLOCK_KNIFE) {
        m_Unlocked = true;
    } else if (type == UpgradeType::KNIFE_COUNT) {
        m_CountLevel++;
        m_KnifeCount += 2;
    } else if (type == UpgradeType::KNIFE_DAMAGE) {
        m_DamageLevel++;
        m_Damage += 15.0f;
    } else if (type == UpgradeType::KNIFE_COOLDOWN) {
        m_CooldownLevel++;
        m_AttackIntervalMs *= 0.80f;
    }
}
