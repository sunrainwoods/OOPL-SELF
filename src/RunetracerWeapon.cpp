#include "RunetracerWeapon.hpp"
#include "Player.hpp"
#include "Camera.hpp"
#include "EnemyManager.hpp"
#include <cmath>
#include <random>

RunetracerWeapon::RunetracerWeapon() = default;

void RunetracerWeapon::Init(const std::string& resourceDir, const Player& player) {
    m_RunetracerImage = std::make_shared<Util::Image>(resourceDir + "/Runetracer.png");
    m_RunetracerImage65 = std::make_shared<Util::Image>(resourceDir + "/Runetracer65.png");

    m_Runetracers.clear();
    m_Runetracers.reserve(m_MaxRunetracers);

    const glm::vec2 playerSize = player.GetScaledSize();
    const float targetRuneWidth = playerSize.x * 0.5f;
    m_BaseScale = targetRuneWidth / m_RunetracerImage->GetSize().x;

    for (int i = 0; i < m_MaxRunetracers; ++i) {
        RunetracerProjectile rune;
        rune.object = std::make_shared<Util::GameObject>();
        rune.object->SetDrawable(m_RunetracerImage);
        rune.object->SetZIndex(4.5f);
        rune.object->SetVisible(false);
        rune.object->m_Transform.scale = {m_BaseScale, m_BaseScale};
        rune.active = false;
        rune.maxHistory = 40;
        m_Runetracers.push_back(rune);
    }
}

void RunetracerWeapon::Reset() {
    m_Unlocked = false;
    m_AttackIntervalMs = 1500.0f;
    m_AttackTimerMs = 0.0f;
    m_Speed = 400.0f;
    m_Damage = 35.0f;
    m_RunetracerCount = 1;

    m_CountLevel = 1;
    m_DamageLevel = 1;
    m_CooldownLevel = 1;

    for (auto& rune : m_Runetracers) {
        rune.active = false;
        rune.historyPositions.clear();
        if (rune.object) {
            rune.object->SetVisible(false);
        }
    }
}

void RunetracerWeapon::Update(float dtMs, Player& player, const Camera& camera,
                              EnemyManager& enemyManager,
                              const std::function<void(Enemy&)>& onEnemyKilled) {
    if (!m_Unlocked) return;

    m_AttackTimerMs += dtMs;
    if (m_AttackTimerMs >= m_AttackIntervalMs) {
        m_AttackTimerMs = 0.0f;

        int spawned = 0;
        static std::mt19937 runeRng(std::random_device{}());
        std::uniform_real_distribution<float> angleDist(0.0f, 6.2831853f);

        for (auto& rune : m_Runetracers) {
            if (!rune.active) {
                rune.active = true;
                rune.worldPosition = player.GetWorldPosition();

                float randomAngle = angleDist(runeRng);
                rune.velocity = glm::vec2(std::cos(randomAngle), std::sin(randomAngle)) * m_Speed;
                rune.timeToLiveMs = 4000.0f;
                rune.angle = randomAngle;
                rune.historyPositions.clear();

                rune.object->m_Transform.rotation = rune.angle;
                rune.object->SetVisible(true);

                spawned++;
                if (spawned >= m_RunetracerCount) break;
            }
        }
    }

    const float dtSec = dtMs / 1000.0f;
    float leftBound, rightBound, bottomBound, topBound;
    camera.GetViewportBounds(leftBound, rightBound, bottomBound, topBound);

    const float runeHitRadius = player.GetScaledSize().x * 0.5f * 0.5f;
    std::vector<Enemy*> activeEnemies = enemyManager.GetActiveEnemies();

    for (auto& rune : m_Runetracers) {
        if (rune.active) {
            // Track history
            rune.historyPositions.insert(rune.historyPositions.begin(), rune.worldPosition);
            if (static_cast<int>(rune.historyPositions.size()) > rune.maxHistory) {
                rune.historyPositions.pop_back();
            }

            rune.worldPosition += rune.velocity * dtSec;

            // Bounce against camera viewport bounds
            if (rune.worldPosition.x < leftBound) {
                rune.worldPosition.x = leftBound;
                rune.velocity.x *= -1.0f;
            } else if (rune.worldPosition.x > rightBound) {
                rune.worldPosition.x = rightBound;
                rune.velocity.x *= -1.0f;
            }

            if (rune.worldPosition.y < bottomBound) {
                rune.worldPosition.y = bottomBound;
                rune.velocity.y *= -1.0f;
            } else if (rune.worldPosition.y > topBound) {
                rune.worldPosition.y = topBound;
                rune.velocity.y *= -1.0f;
            }

            rune.angle = std::atan2(rune.velocity.y, rune.velocity.x);
            rune.object->m_Transform.rotation = rune.angle;
            rune.object->m_Transform.translation = camera.WorldToScreen(rune.worldPosition);

            rune.timeToLiveMs -= dtMs;
            if (rune.timeToLiveMs <= 0.0f) {
                rune.active = false;
                rune.object->SetVisible(false);
                continue;
            }

            // Piercing collision with enemies
            for (auto* enemyPtr : activeEnemies) {
                auto& enemy = *enemyPtr;
                if (enemy.IsActive() && enemy.GetHitCooldownTimerMs() <= 0.0f) {
                    float hitRadius = runeHitRadius + enemy.GetCollisionRadius();
                    glm::vec2 diff = enemy.GetWorldPosition() - rune.worldPosition;
                    float distSq = diff.x * diff.x + diff.y * diff.y;

                    if (distSq <= (hitRadius * hitRadius)) {
                        enemy.TakeDamage(m_Damage);
                        player.ApplyLifesteal(m_Damage);

                        if (enemy.IsDead()) {
                            onEnemyKilled(enemy);
                        }
                    }
                }
            }
        }
    }
}

void RunetracerWeapon::Draw(const Camera& camera) {
    if (!m_Unlocked) return;

    for (auto& rune : m_Runetracers) {
        if (rune.active) {
            float scaleStep = m_BaseScale / rune.maxHistory;

            // Draw ghosts from oldest to newest
            for (int i = static_cast<int>(rune.historyPositions.size()) - 1; i >= 0; --i) {
                float trailScale = m_BaseScale - (i * scaleStep);

                rune.object->SetDrawable(m_RunetracerImage65);
                rune.object->SetZIndex(4.4f - (i * 0.01f));
                rune.object->m_Transform.translation = camera.WorldToScreen(rune.historyPositions[i]);
                rune.object->m_Transform.scale = {trailScale, trailScale};
                rune.object->Draw();
            }

            // Restore body
            rune.object->SetDrawable(m_RunetracerImage);
            rune.object->SetZIndex(4.5f);

            if (camera.IsInView(rune.worldPosition)) {
                rune.object->m_Transform.translation = camera.WorldToScreen(rune.worldPosition);
                rune.object->m_Transform.scale = {m_BaseScale, m_BaseScale};
                rune.object->Draw();
            }
        }
    }
}

bool RunetracerWeapon::CanUpgrade(UpgradeType type) const {
    if (type == UpgradeType::UNLOCK_RUNETRACER) return !m_Unlocked;
    if (!m_Unlocked) return false;
    if (type == UpgradeType::RUNETRACER_COUNT) return m_CountLevel < 15;
    if (type == UpgradeType::RUNETRACER_DAMAGE) return m_DamageLevel < 15;
    if (type == UpgradeType::RUNETRACER_COOLDOWN) return m_CooldownLevel < 15;
    return false;
}

void RunetracerWeapon::ApplyUpgrade(UpgradeType type, const Player&) {
    if (type == UpgradeType::UNLOCK_RUNETRACER) {
        m_Unlocked = true;
    } else if (type == UpgradeType::RUNETRACER_COUNT) {
        m_CountLevel++;
        m_RunetracerCount++;
    } else if (type == UpgradeType::RUNETRACER_DAMAGE) {
        m_DamageLevel++;
        m_Damage += 15.0f;
    } else if (type == UpgradeType::RUNETRACER_COOLDOWN) {
        m_CooldownLevel++;
        m_AttackIntervalMs *= 0.80f;
    }
}
