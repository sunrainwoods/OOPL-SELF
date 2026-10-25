#include "UpgradeManager.hpp"
#include "Player.hpp"
#include "WeaponManager.hpp"
#include <random>
#include <algorithm>

UpgradeManager::UpgradeManager() = default;

void UpgradeManager::Reset() {
    m_CurrentOptions.clear();
    m_PendingLevelUps = 0;
}

bool UpgradeManager::ConsumePendingLevelUp() {
    if (m_PendingLevelUps > 0) {
        m_PendingLevelUps--;
        return true;
    }
    return false;
}

void UpgradeManager::GenerateUpgradeOptions(const Player& player, const WeaponManager& weaponManager) {
    m_CurrentOptions.clear();
    std::vector<UpgradeType> validPool;

    auto whip = weaponManager.GetWhip();
    auto knife = weaponManager.GetKnife();
    auto rune = weaponManager.GetRunetracer();

    if (!knife->IsUnlocked()) validPool.push_back(UpgradeType::UNLOCK_KNIFE);
    if (!rune->IsUnlocked()) validPool.push_back(UpgradeType::UNLOCK_RUNETRACER);

    if (whip->GetRangeLevel() < 15) validPool.push_back(UpgradeType::WHIP_RANGE);
    if (whip->GetDamageLevel() < 15) validPool.push_back(UpgradeType::WHIP_DAMAGE);
    if (whip->GetCooldownLevel() < 15) validPool.push_back(UpgradeType::WHIP_COOLDOWN);

    if (knife->IsUnlocked()) {
        if (knife->GetCountLevel() < 15) validPool.push_back(UpgradeType::KNIFE_COUNT);
        if (knife->GetDamageLevel() < 15) validPool.push_back(UpgradeType::KNIFE_DAMAGE);
        if (knife->GetCooldownLevel() < 15) validPool.push_back(UpgradeType::KNIFE_COOLDOWN);
    }

    if (rune->IsUnlocked()) {
        if (rune->GetCountLevel() < 15) validPool.push_back(UpgradeType::RUNETRACER_COUNT);
        if (rune->GetDamageLevel() < 15) validPool.push_back(UpgradeType::RUNETRACER_DAMAGE);
        if (rune->GetCooldownLevel() < 15) validPool.push_back(UpgradeType::RUNETRACER_COOLDOWN);
    }

    if (player.GetMaxHealth() < 1500.0f) validPool.push_back(UpgradeType::MAX_HEALTH);

    if (!player.IsArmorUnlocked()) {
        validPool.push_back(UpgradeType::UNLOCK_ARMOR);
    } else if (player.GetArmorLevel() < 15) {
        validPool.push_back(UpgradeType::ARMOR);
    }

    if (!player.IsVampirismUnlocked()) {
        validPool.push_back(UpgradeType::UNLOCK_VAMPIRISM);
    } else if (player.GetVampirismLevel() < 15) {
        validPool.push_back(UpgradeType::VAMPIRISM);
    }

    if (validPool.empty()) {
        validPool.push_back(UpgradeType::MAX_HEALTH);
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(validPool.begin(), validPool.end(), g);

    int count = std::min(static_cast<int>(validPool.size()), 3);
    for (int i = 0; i < count; ++i) {
        m_CurrentOptions.push_back(validPool[i]);
    }
}

void UpgradeManager::ApplyUpgrade(UpgradeType type, Player& player, WeaponManager& weaponManager) {
    switch (type) {
        case UpgradeType::UNLOCK_KNIFE:
        case UpgradeType::UNLOCK_RUNETRACER:
        case UpgradeType::WHIP_RANGE:
        case UpgradeType::WHIP_DAMAGE:
        case UpgradeType::WHIP_COOLDOWN:
        case UpgradeType::KNIFE_COUNT:
        case UpgradeType::KNIFE_DAMAGE:
        case UpgradeType::KNIFE_COOLDOWN:
        case UpgradeType::RUNETRACER_COUNT:
        case UpgradeType::RUNETRACER_DAMAGE:
        case UpgradeType::RUNETRACER_COOLDOWN:
            weaponManager.ApplyUpgrade(type, player);
            break;

        case UpgradeType::UNLOCK_ARMOR:
            player.UnlockArmor();
            break;
        case UpgradeType::ARMOR:
            player.UpgradeArmor();
            break;
        case UpgradeType::UNLOCK_VAMPIRISM:
            player.UnlockVampirism();
            break;
        case UpgradeType::VAMPIRISM:
            player.UpgradeVampirism();
            break;
        case UpgradeType::MAX_HEALTH:
            player.UpgradeMaxHealth();
            break;
    }
}
