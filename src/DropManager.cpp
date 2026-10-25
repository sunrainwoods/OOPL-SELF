#include "DropManager.hpp"
#include "Player.hpp"
#include "Camera.hpp"
#include "Enemy.hpp"
#include "config.hpp"
#include <random>
#include <algorithm>
#include <cmath>

DropManager::DropManager() = default;

void DropManager::Init(const std::string& resourceDir, const Player& player) {
    m_Gem1Image = std::make_shared<Util::Image>(resourceDir + "/Experience_Gem1.png");
    m_Gem2Image = std::make_shared<Util::Image>(resourceDir + "/Experience_Gem2.png");
    m_Gem3Image = std::make_shared<Util::Image>(resourceDir + "/Experience_Gem3.png");
    m_HealthImage = std::make_shared<Util::Image>(resourceDir + "/check.png");
    m_MagnetImage = std::make_shared<Util::Image>(resourceDir + "/magnet.png");
    m_RewardImage = std::make_shared<Util::Image>(resourceDir + "/reward.png");

    const glm::vec2 playerSize = player.GetScaledSize();

    // Exp Gems
    m_ExpGems.clear();
    m_ExpGems.reserve(m_MaxExpGems);
    for (int i = 0; i < m_MaxExpGems; ++i) {
        ExpGem gem;
        gem.object = std::make_shared<Util::GameObject>();
        gem.object->SetDrawable(m_Gem1Image);
        gem.object->SetZIndex(3.0f);
        gem.object->SetVisible(false);
        gem.active = false;
        gem.expValue = 1;
        gem.pickupCooldownTimerMs = 150.0f;
        m_ExpGems.push_back(gem);
    }

    // Health Potions
    m_HealthItems.clear();
    m_HealthItems.reserve(m_MaxHealthItems);
    const float potionWidth = playerSize.x * m_ExpGemSizeRatioToPlayer;
    const float potionScale = potionWidth / m_HealthImage->GetSize().x;
    for (int i = 0; i < m_MaxHealthItems; ++i) {
        HealthItem potion;
        potion.object = std::make_shared<Util::GameObject>();
        potion.object->SetDrawable(m_HealthImage);
        potion.object->SetZIndex(3.0f);
        potion.object->SetVisible(false);
        potion.object->m_Transform.scale = {potionScale, potionScale};
        potion.active = false;
        potion.pickupCooldownTimerMs = 150.0f;
        m_HealthItems.push_back(potion);
    }

    // Magnet Items
    m_MagnetItems.clear();
    m_MagnetItems.reserve(m_MaxMagnetItems);
    const float magnetWidth = playerSize.x * m_ExpGemSizeRatioToPlayer;
    const float magnetScale = magnetWidth / m_MagnetImage->GetSize().x;
    for (int i = 0; i < m_MaxMagnetItems; ++i) {
        MagnetItem magnet;
        magnet.object = std::make_shared<Util::GameObject>();
        magnet.object->SetDrawable(m_MagnetImage);
        magnet.object->SetZIndex(3.0f);
        magnet.object->SetVisible(false);
        magnet.object->m_Transform.scale = {magnetScale, magnetScale};
        magnet.active = false;
        magnet.pickupCooldownTimerMs = 150.0f;
        m_MagnetItems.push_back(magnet);
    }

    // Boss Rewards
    m_RewardItems.clear();
    m_RewardItems.reserve(m_MaxRewardItems);
    const float rewardWidth = playerSize.x * m_ExpGemSizeRatioToPlayer * 1.5f;
    const float rewardScale = rewardWidth / m_RewardImage->GetSize().x;
    for (int i = 0; i < m_MaxRewardItems; ++i) {
        RewardItem reward;
        reward.object = std::make_shared<Util::GameObject>();
        reward.object->SetDrawable(m_RewardImage);
        reward.object->SetZIndex(3.0f);
        reward.object->SetVisible(false);
        reward.object->m_Transform.scale = {rewardScale, rewardScale};
        reward.active = false;
        reward.pickupCooldownTimerMs = 150.0f;
        m_RewardItems.push_back(reward);
    }
}

void DropManager::Reset() {
    m_MagnetTimerMs = 0.0f;

    for (auto& gem : m_ExpGems) {
        gem.active = false;
        if (gem.object) gem.object->SetVisible(false);
    }
    for (auto& potion : m_HealthItems) {
        potion.active = false;
        if (potion.object) potion.object->SetVisible(false);
    }
    for (auto& magnet : m_MagnetItems) {
        magnet.active = false;
        if (magnet.object) magnet.object->SetVisible(false);
    }
    for (auto& reward : m_RewardItems) {
        reward.active = false;
        if (reward.object) reward.object->SetVisible(false);
    }
}

void DropManager::HandleEnemyDeath(const Enemy& enemy, const Player& player) {
    static std::mt19937 dropRng(std::random_device{}());
    std::uniform_real_distribution<float> dropDist(0.0f, 100.0f);
    float dropRoll = dropDist(dropRng);

    const glm::vec2& enemyPos = enemy.GetWorldPosition();
    const glm::vec2 playerSize = player.GetScaledSize();

    if (enemy.IsBoss()) {
        for (auto& reward : m_RewardItems) {
            if (!reward.active) {
                reward.active = true;
                reward.worldPosition = enemyPos;
                reward.pickupCooldownTimerMs = 150.0f;
                reward.object->SetVisible(true);
                break;
            }
        }
    }

    // Exp gem drop
    for (auto& gem : m_ExpGems) {
        if (!gem.active) {
            gem.active = true;
            gem.worldPosition = enemyPos;
            gem.pickupCooldownTimerMs = 150.0f;
            gem.isMagnetized = false;

            std::shared_ptr<Util::Image> selectedImage;
            if (dropRoll < 5.0f) {
                gem.expValue = 10;
                selectedImage = m_Gem3Image;
            } else if (dropRoll < 20.0f) {
                gem.expValue = 4;
                selectedImage = m_Gem2Image;
            } else {
                gem.expValue = 1;
                selectedImage = m_Gem1Image;
            }

            gem.object->SetDrawable(selectedImage);
            const float targetWidth = playerSize.x * m_ExpGemSizeRatioToPlayer;
            const float imgScale = targetWidth / selectedImage->GetSize().x;
            gem.object->m_Transform.scale = {imgScale, imgScale};
            gem.object->SetVisible(true);
            break;
        }
    }

    // 3% Health item drop
    if (dropDist(dropRng) < 3.0f) {
        for (auto& potion : m_HealthItems) {
            if (!potion.active) {
                potion.active = true;
                potion.worldPosition = enemyPos + glm::vec2(20.0f, 20.0f);
                potion.pickupCooldownTimerMs = 150.0f;
                potion.object->SetVisible(true);
                break;
            }
        }
    }

    // 3% Magnet item drop
    if (dropDist(dropRng) < 3.0f) {
        for (auto& magnet : m_MagnetItems) {
            if (!magnet.active) {
                magnet.active = true;
                magnet.worldPosition = enemyPos + glm::vec2(-20.0f, -20.0f);
                magnet.pickupCooldownTimerMs = 150.0f;
                magnet.object->SetVisible(true);
                break;
            }
        }
    }
}

void DropManager::Update(float dtMs, Player& player, const Camera& camera,
                         int& pendingLevelUps, bool& triggerLevelUp) {
    const glm::vec2& playerWorldPos = player.GetWorldPosition();
    const float pickupRadius = player.GetScaledSize().x * 0.7f;
    const float dtSec = dtMs / 1000.0f;

    // Magnet duration
    if (m_MagnetTimerMs > 0.0f) {
        m_MagnetTimerMs -= dtMs;
        const glm::vec2& camPos = camera.GetPosition();
        float cullDistX = static_cast<float>(WINDOW_WIDTH) * 0.6f;
        float cullDistY = static_cast<float>(WINDOW_HEIGHT) * 0.6f;

        for (auto& gem : m_ExpGems) {
            if (gem.active && !gem.isMagnetized) {
                float dx = std::abs(gem.worldPosition.x - camPos.x);
                float dy = std::abs(gem.worldPosition.y - camPos.y);
                if (dx <= cullDistX && dy <= cullDistY) {
                    gem.isMagnetized = true;
                }
            }
        }
    }

    // Exp Gems
    for (auto& gem : m_ExpGems) {
        if (gem.active) {
            if (gem.pickupCooldownTimerMs > 0.0f && !gem.isMagnetized) {
                gem.pickupCooldownTimerMs -= dtMs;
            } else {
                float dist = glm::distance(playerWorldPos, gem.worldPosition);

                if (gem.isMagnetized) {
                    float moveDist = 1200.0f * dtSec;
                    if (moveDist > dist) moveDist = dist;
                    glm::vec2 toPlayer = glm::normalize(playerWorldPos - gem.worldPosition);
                    gem.worldPosition += toPlayer * moveDist;
                    dist = glm::distance(playerWorldPos, gem.worldPosition);
                }

                if (dist <= pickupRadius) {
                    gem.active = false;
                    gem.object->SetVisible(false);
                    if (player.AddExp(gem.expValue)) {
                        triggerLevelUp = true;
                    }
                }
            }
            gem.object->m_Transform.translation = camera.WorldToScreen(gem.worldPosition);
        }
    }

    // Reward Items
    for (auto& reward : m_RewardItems) {
        if (reward.active) {
            if (reward.pickupCooldownTimerMs > 0.0f) {
                reward.pickupCooldownTimerMs -= dtMs;
            } else {
                float dist = glm::distance(playerWorldPos, reward.worldPosition);
                if (dist <= pickupRadius) {
                    reward.active = false;
                    reward.object->SetVisible(false);
                    player.FullHeal();
                    pendingLevelUps += 3;
                    triggerLevelUp = true;
                }
            }
            reward.object->m_Transform.translation = camera.WorldToScreen(reward.worldPosition);
        }
    }

    // Health Items
    for (auto& potion : m_HealthItems) {
        if (potion.active) {
            if (potion.pickupCooldownTimerMs > 0.0f) {
                potion.pickupCooldownTimerMs -= dtMs;
            } else {
                float dist = glm::distance(playerWorldPos, potion.worldPosition);
                if (dist <= pickupRadius) {
                    potion.active = false;
                    potion.object->SetVisible(false);
                    player.FullHeal();
                }
            }
            potion.object->m_Transform.translation = camera.WorldToScreen(potion.worldPosition);
        }
    }

    // Magnet Items
    for (auto& magnet : m_MagnetItems) {
        if (magnet.active) {
            if (magnet.pickupCooldownTimerMs > 0.0f) {
                magnet.pickupCooldownTimerMs -= dtMs;
            } else {
                float dist = glm::distance(playerWorldPos, magnet.worldPosition);
                if (dist <= pickupRadius) {
                    magnet.active = false;
                    magnet.object->SetVisible(false);
                    m_MagnetTimerMs = 5000.0f;
                }
            }
            magnet.object->m_Transform.translation = camera.WorldToScreen(magnet.worldPosition);
        }
    }
}

void DropManager::Draw(const Camera& camera) {
    for (auto& gem : m_ExpGems) {
        if (gem.active && camera.IsInView(gem.worldPosition)) {
            gem.object->Draw();
        }
    }
    for (auto& potion : m_HealthItems) {
        if (potion.active && camera.IsInView(potion.worldPosition)) {
            potion.object->Draw();
        }
    }
    for (auto& reward : m_RewardItems) {
        if (reward.active && camera.IsInView(reward.worldPosition)) {
            reward.object->Draw();
        }
    }
    for (auto& magnet : m_MagnetItems) {
        if (magnet.active && camera.IsInView(magnet.worldPosition)) {
            magnet.object->Draw();
        }
    }
}
