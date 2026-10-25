#include "EnemyManager.hpp"
#include "Player.hpp"
#include "Camera.hpp"
#include <algorithm>
#include <cmath>
#include <random>
#include <glm/gtc/constants.hpp>

EnemyManager::EnemyManager() = default;

void EnemyManager::Init(const std::string& resourceDir) {
    m_Enemy1Image = std::make_shared<Util::Image>(resourceDir + "/enemy1.png");
    m_Enemy1HurtImage = std::make_shared<Util::Image>(resourceDir + "/hurt_enemy1.png");
    m_Enemy2Image = std::make_shared<Util::Image>(resourceDir + "/enemy2.png");
    m_Enemy2HurtImage = std::make_shared<Util::Image>(resourceDir + "/hurt_enemy2.png");
    m_Enemy3Image = std::make_shared<Util::Image>(resourceDir + "/enemy3.png");
    m_Enemy3HurtImage = std::make_shared<Util::Image>(resourceDir + "/hurt_enemy3.png");
    m_Enemy4Image = std::make_shared<Util::Image>(resourceDir + "/enemy4.png");
    m_Enemy4HurtImage = std::make_shared<Util::Image>(resourceDir + "/hurt_enemy4.png");

    std::vector<std::string> bossLeftFrames = {
        resourceDir + "/boss_left1.png", resourceDir + "/boss_left2.png",
        resourceDir + "/boss_left3.png", resourceDir + "/boss_left4.png"};
    std::vector<std::string> bossRightFrames = {
        resourceDir + "/boss_right1.png", resourceDir + "/boss_right2.png",
        resourceDir + "/boss_right3.png", resourceDir + "/boss_right4.png"};
    m_BossLeftAnimation = std::make_shared<Util::Animation>(bossLeftFrames, true, 100, true, 0);
    m_BossRightAnimation = std::make_shared<Util::Animation>(bossRightFrames, true, 100, true, 0);

    m_BossHurtLeftImage = std::make_shared<Util::Image>(resourceDir + "/hurt_boss_left1.png");
    m_BossHurtRightImage = std::make_shared<Util::Image>(resourceDir + "/hurt_boss_right1.png");

    m_Enemies.clear();
    m_Enemies.resize(m_MaxEnemies);
    for (auto& enemy : m_Enemies) {
        enemy.InitObject();
        enemy.Deactivate();
    }
}

void EnemyManager::Reset() {
    for (auto& enemy : m_Enemies) {
        enemy.Deactivate();
    }
    m_EnemiesDefeated = 0;
    m_CurrentWave = 1;
    m_CurrentStage = 1;
    m_KillsToNextWave = 10;
    m_LastSpecialWaveTriggered = 0;
    m_EnemySpawnTimerMs = 0.0f;
    m_EnemySpawnIntervalMs = 1200.0f;
}

void EnemyManager::PauseAnimations() {
    if (m_BossLeftAnimation) m_BossLeftAnimation->Pause();
    if (m_BossRightAnimation) m_BossRightAnimation->Pause();
}

void EnemyManager::PlayAnimations() {
    if (m_BossLeftAnimation) m_BossLeftAnimation->Play();
    if (m_BossRightAnimation) m_BossRightAnimation->Play();
}

void EnemyManager::OnEnemyKilled(Enemy& enemy) {
    enemy.Deactivate();
    m_EnemiesDefeated++;
    if (m_EnemiesDefeated >= m_KillsToNextWave) {
        m_CurrentWave++;
        int nextRequirement = 10 + (m_CurrentWave - 1) * 5;
        m_KillsToNextWave += nextRequirement;
        m_CurrentStage = (m_CurrentWave - 1) / 5 + 1;
        m_EnemySpawnIntervalMs = std::max(50.0f, 1200.0f - static_cast<float>(m_CurrentWave - 1) * 150.0f);
    }
}

std::vector<Enemy*> EnemyManager::GetActiveEnemies() {
    std::vector<Enemy*> active;
    active.reserve(m_Enemies.size());
    for (auto& enemy : m_Enemies) {
        if (enemy.IsActive()) {
            active.push_back(&enemy);
        }
    }
    return active;
}

void EnemyManager::SpawnBossWave(const glm::vec2& playerWorldPos, float playerWidth) {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> angleDist(0.0f, glm::two_pi<float>());

    bool bossSpawned = false;
    for (auto& enemy : m_Enemies) {
        if (!enemy.IsActive()) {
            float angle = angleDist(rng);
            float bossDist = m_EnemySpawnMaxDistance * 0.8f;
            glm::vec2 spawnPos = playerWorldPos + glm::vec2{std::cos(angle), std::sin(angle)} * bossDist;

            float baselineHP = 150.0f + static_cast<float>(m_CurrentWave - 1) * 150.0f;
            float baselineDmg = 15.0f + static_cast<float>(m_CurrentWave - 1) * 4.5f;
            float bossHP = baselineHP * 8.0f;
            float bossDmg = baselineDmg * 2.0f;
            float targetWidth = playerWidth * 3.0f;
            float bossScale = targetWidth / m_BossLeftAnimation->GetSize().x;

            enemy.Spawn(spawnPos, bossHP, 65.0f, bossDmg, bossScale, 4.2f,
                        nullptr, m_BossHurtLeftImage, m_BossLeftAnimation, true, false);
            bossSpawned = true;
            break;
        }
    }

    if (bossSpawned) {
        int specialEnemyCount = 60;
        float circleRadius = m_EnemySpawnMaxDistance * 1.5f;
        for (int i = 0; i < specialEnemyCount; ++i) {
            for (auto& enemy : m_Enemies) {
                if (!enemy.IsActive()) {
                    float angle = (glm::two_pi<float>() / specialEnemyCount) * i;
                    glm::vec2 spawnPos = playerWorldPos + glm::vec2{std::cos(angle), std::sin(angle)} * circleRadius;
                    float baselineHP = 150.0f + static_cast<float>(m_CurrentWave - 1) * 45.0f;
                    float enemyHP = baselineHP * 3.0f;
                    float targetWidth = playerWidth * m_EnemyWidthRatioToPlayer;
                    float enemyScale = targetWidth / m_Enemy4Image->GetSize().x;

                    enemy.Spawn(spawnPos, enemyHP, 45.0f, 0.0f, enemyScale, 4.1f,
                                m_Enemy4Image, m_Enemy4HurtImage, nullptr, false, true);
                    break;
                }
            }
        }
    }
}

void EnemyManager::SpawnRegularEnemies(const glm::vec2& playerWorldPos, float playerWidth) {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> angleDist(0.0f, glm::two_pi<float>());
    std::uniform_real_distribution<float> distanceDist(m_EnemySpawnMinDistance, m_EnemySpawnMaxDistance);
    std::uniform_real_distribution<float> typeDist(0.0f, 100.0f);

    float baselineHP = 90.0f + static_cast<float>(m_CurrentWave - 1) * 52.5f;
    float baselineDmg = 9.0f + static_cast<float>(m_CurrentWave - 1) * 2.25f;

    int spawnCount = 1;
    if (m_CurrentWave >= 16) {
        spawnCount = 3;
    } else if (m_CurrentWave >= 7) {
        spawnCount = 2;
    }

    for (int k = 0; k < spawnCount; ++k) {
        const float angle = angleDist(rng);
        const float distance = distanceDist(rng);
        const glm::vec2 spawnOffset = glm::vec2{std::cos(angle), std::sin(angle)} * distance;
        const glm::vec2 spawnPos = playerWorldPos + spawnOffset;

        for (auto& enemy : m_Enemies) {
            if (!enemy.IsActive()) {
                float typeRoll = typeDist(rng);
                std::shared_ptr<Util::Image> defImg;
                std::shared_ptr<Util::Image> hurtImg;
                float speed = 0.0f;
                float dmg = 0.0f;
                float hp = 0.0f;
                float scaleMult = 1.0f;

                if (typeRoll < 20.0f) {
                    defImg = m_Enemy3Image;
                    hurtImg = m_Enemy3HurtImage;
                    speed = 55.0f;
                    dmg = baselineDmg * 2.0f;
                    hp = baselineHP * 2.0f;
                    scaleMult = 1.5f;
                } else if (typeRoll < 50.0f) {
                    defImg = m_Enemy2Image;
                    hurtImg = m_Enemy2HurtImage;
                    speed = 95.0f;
                    dmg = baselineDmg;
                    hp = baselineHP;
                    scaleMult = 1.0f;
                } else {
                    defImg = m_Enemy1Image;
                    hurtImg = m_Enemy1HurtImage;
                    speed = 170.0f;
                    dmg = baselineDmg * 0.5f;
                    hp = baselineHP * 0.5f;
                    scaleMult = 0.7f;
                }

                const float targetWidth = playerWidth * m_EnemyWidthRatioToPlayer * 1.2f;
                const float baseScale = targetWidth / defImg->GetSize().x;
                const float finalScale = baseScale * scaleMult;

                enemy.Spawn(spawnPos, hp, speed, dmg, finalScale, 4.0f,
                            defImg, hurtImg, nullptr, false, false);
                break;
            }
        }
    }
}

void EnemyManager::Update(float dtMs, Player& player, const Camera& camera,
                          const std::function<void(Enemy&)>& onEnemyKilled,
                          bool& playerDied) {
    playerDied = false;
    m_EnemySpawnTimerMs += dtMs;

    std::vector<Enemy*> activeEnemies = GetActiveEnemies();
    int activeEnemyCount = static_cast<int>(activeEnemies.size());

    // Spatial hash grid
    const float CELL_SIZE = 150.0f;
    std::unordered_map<int64_t, std::vector<Enemy*>> grid;
    auto getGridKey = [CELL_SIZE](const glm::vec2& pos) -> int64_t {
        int cx = static_cast<int>(std::floor(pos.x / CELL_SIZE));
        int cy = static_cast<int>(std::floor(pos.y / CELL_SIZE));
        return (static_cast<int64_t>(cx) << 32) | (cy & 0xFFFFFFFF);
    };

    for (auto* enemyPtr : activeEnemies) {
        grid[getGridKey(enemyPtr->GetWorldPosition())].push_back(enemyPtr);
    }

    // Spawn check
    const glm::vec2& playerWorldPos = player.GetWorldPosition();
    const float playerWidth = player.GetScaledSize().x;

    if (m_EnemySpawnTimerMs >= m_EnemySpawnIntervalMs && activeEnemyCount < m_MaxEnemies) {
        m_EnemySpawnTimerMs = 0.0f;

        if (m_CurrentWave > 0 && m_CurrentWave % 5 == 0 && m_LastSpecialWaveTriggered != m_CurrentWave) {
            m_LastSpecialWaveTriggered = m_CurrentWave;
            SpawnBossWave(playerWorldPos, playerWidth);
        }

        SpawnRegularEnemies(playerWorldPos, playerWidth);
    }

    // Re-collect active enemies if any were spawned
    activeEnemies = GetActiveEnemies();

    const float playerCollisionRadius = player.GetCollisionRadius();

    for (auto* enemyPtr : activeEnemies) {
        auto& enemy = *enemyPtr;
        if (!enemy.IsActive()) continue;

        enemy.UpdateHurtState(dtMs);

        const glm::vec2 toPlayer = playerWorldPos - enemy.GetWorldPosition();
        const float distanceToPlayer = glm::length(toPlayer);
        const float enemyCollisionRadius = enemy.GetCollisionRadius();
        const float damageHitRadius = playerCollisionRadius + enemyCollisionRadius;

        // Player damage check
        if (distanceToPlayer < damageHitRadius && player.GetHitCooldownTimerMs() <= 0.0f) {
            float actualDamage = enemy.GetDamage();
            if (enemy.IsPercentDamage()) {
                float percent = 0.15f + static_cast<float>(m_CurrentWave - 1) * 0.015f;
                actualDamage = player.GetMaxHealth() * percent;
            }

            if (player.TakeDamage(actualDamage)) {
                playerDied = true;
                return;
            }
        }

        if (distanceToPlayer > 0.1f) {
            const glm::vec2 direction = toPlayer / distanceToPlayer;
            glm::vec2 separation(0.0f);

            int cx = static_cast<int>(std::floor(enemy.GetWorldPosition().x / CELL_SIZE));
            int cy = static_cast<int>(std::floor(enemy.GetWorldPosition().y / CELL_SIZE));

            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    int64_t key = (static_cast<int64_t>(cx + dx) << 32) | ((cy + dy) & 0xFFFFFFFF);
                    auto it = grid.find(key);
                    if (it != grid.end()) {
                        for (const auto* otherEnemyPtr : it->second) {
                            const auto& otherEnemy = *otherEnemyPtr;
                            if (!otherEnemy.IsActive() || &enemy == &otherEnemy) continue;

                            glm::vec2 toOther = enemy.GetWorldPosition() - otherEnemy.GetWorldPosition();
                            float otherRadius = otherEnemy.GetCollisionRadius();
                            float minSeparation = enemyCollisionRadius + otherRadius;

                            if (std::abs(toOther.x) > minSeparation || std::abs(toOther.y) > minSeparation) {
                                continue;
                            }

                            float dist = glm::length(toOther);
                            if (dist > 0.0f && dist < minSeparation) {
                                separation += (toOther / dist) * (minSeparation - dist) * 0.4f;
                            }
                        }
                    }
                }
            }

            float playerThickSeparation = playerCollisionRadius + enemyCollisionRadius;
            if (distanceToPlayer > 0.0f && distanceToPlayer < playerThickSeparation) {
                float overlap = playerThickSeparation - distanceToPlayer;
                separation -= (toPlayer / distanceToPlayer) * overlap * 0.6f;
                player.OffsetWorldPosition(-(toPlayer / distanceToPlayer) * overlap * 0.1f);
            }

            if (glm::length(separation) > 10.0f) {
                separation = glm::normalize(separation) * 10.0f;
            }

            if (enemy.IsBoss()) {
                if (direction.x < 0.0f) {
                    enemy.SetBossVisuals(m_BossLeftAnimation, m_BossHurtLeftImage);
                } else {
                    enemy.SetBossVisuals(m_BossRightAnimation, m_BossHurtRightImage);
                }
            }

            enemy.UpdateMovement(dtMs, direction, separation);
        }

        enemy.SyncScreenPosition(camera);
    }
}

void EnemyManager::Draw(const Camera& camera) {
    for (auto& enemy : m_Enemies) {
        if (enemy.IsActive()) {
            enemy.Draw(camera);
        }
    }
}
