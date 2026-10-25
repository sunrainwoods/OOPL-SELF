#include "WeaponManager.hpp"

WeaponManager::WeaponManager() {
    m_Whip = std::make_shared<Whip>();
    m_Knife = std::make_shared<KnifeWeapon>();
    m_Runetracer = std::make_shared<RunetracerWeapon>();
}

void WeaponManager::Init(const std::string& resourceDir, const Player& player) {
    m_Whip->Init(resourceDir, player);
    m_Knife->Init(resourceDir, player);
    m_Runetracer->Init(resourceDir, player);
}

void WeaponManager::Reset() {
    m_Whip->Reset();
    m_Knife->Reset();
    m_Runetracer->Reset();
}

void WeaponManager::Update(float dtMs, Player& player, const Camera& camera,
                           EnemyManager& enemyManager,
                           const std::function<void(Enemy&)>& onEnemyKilled) {
    m_Whip->Update(dtMs, player, camera, enemyManager, onEnemyKilled);
    m_Knife->Update(dtMs, player, camera, enemyManager, onEnemyKilled);
    m_Runetracer->Update(dtMs, player, camera, enemyManager, onEnemyKilled);
}

void WeaponManager::Draw(const Camera& camera) {
    m_Whip->Draw(camera);
    m_Knife->Draw(camera);
    m_Runetracer->Draw(camera);
}

bool WeaponManager::CanUpgrade(UpgradeType type) const {
    return m_Whip->CanUpgrade(type) ||
           m_Knife->CanUpgrade(type) ||
           m_Runetracer->CanUpgrade(type);
}

void WeaponManager::ApplyUpgrade(UpgradeType type, const Player& player) {
    if (m_Whip->CanUpgrade(type)) {
        m_Whip->ApplyUpgrade(type, player);
    } else if (m_Knife->CanUpgrade(type)) {
        m_Knife->ApplyUpgrade(type, player);
    } else if (m_Runetracer->CanUpgrade(type)) {
        m_Runetracer->ApplyUpgrade(type, player);
    }
}
