#include "Player.hpp"
#include "Camera.hpp"
#include "config.hpp"
#include <algorithm>
#include <cmath>

Player::Player() {
    m_RightImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/right_main1.png");
    m_LeftImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/left_main1.png");
    m_RightHurtImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/hurt_right_main1.png");
    m_LeftHurtImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/hurt_left_main1.png");

    std::vector<std::string> rightWalkFrames;
    std::vector<std::string> leftWalkFrames;
    rightWalkFrames.reserve(3);
    leftWalkFrames.reserve(3);
    for (int i = 1; i <= 3; ++i) {
        rightWalkFrames.push_back(std::string(RESOURCE_DIR) + "/right_main_walk" + std::to_string(i) + ".png");
        leftWalkFrames.push_back(std::string(RESOURCE_DIR) + "/left_main_walk" + std::to_string(i) + ".png");
    }

    m_RightWalkAnimation = std::make_shared<Util::Animation>(rightWalkFrames, false, 150, true, 0);
    m_LeftWalkAnimation = std::make_shared<Util::Animation>(leftWalkFrames, false, 150, true, 0);

    SetDrawable(m_RightImage);
    SetZIndex(5.0f);
    m_Transform.scale = {m_PlayerScale, m_PlayerScale};
}

void Player::Reset() {
    m_WorldPosition = {0.0f, 0.0f};
    m_Transform.translation = {0.0f, 0.0f};
    m_Transform.scale = {m_PlayerScale, m_PlayerScale};
    m_LastMoveDir = {1.0f, 0.0f};
    m_IsFacingLeft = false;
    m_IsMoving = false;

    m_MaxHealth = 100.0f;
    m_Health = 100.0f;
    m_HitCooldownTimerMs = 0.0f;
    m_Armor = 0.0f;
    m_Vampirism = 0.0f;
    m_ArmorUnlocked = false;
    m_VampirismUnlocked = false;

    m_MaxHealthLevel = 1;
    m_ArmorLevel = 1;
    m_VampirismLevel = 1;

    m_Level = 1;
    m_Exp = 0;
    m_ExpNext = 10;

    SetVisualState(false, false, false);
}

void Player::Update(float dtMs, const Camera& camera) {
    if (m_HitCooldownTimerMs > 0.0f) {
        m_HitCooldownTimerMs = std::max(0.0f, m_HitCooldownTimerMs - dtMs);
    }

    SetVisualState(m_IsFacingLeft, m_IsMoving, m_HitCooldownTimerMs > 150.0f);
    m_Transform.translation = camera.WorldToScreen(m_WorldPosition);
}

void Player::UpdateMovement(float dtMs, const glm::vec2& moveDir) {
    m_IsMoving = (moveDir.x != 0.0f || moveDir.y != 0.0f);
    if (m_IsMoving) {
        glm::vec2 normDir = glm::normalize(moveDir);
        m_LastMoveDir = normDir;
        m_WorldPosition += normDir * m_MoveSpeed * dtMs;
    }

    if (moveDir.x < 0.0f) {
        m_IsFacingLeft = true;
    } else if (moveDir.x > 0.0f) {
        m_IsFacingLeft = false;
    }
}

bool Player::TakeDamage(float damage) {
    float actualDamage = damage * (1.0f - m_Armor);
    m_Health -= actualDamage;
    m_HitCooldownTimerMs = 250.0f;
    if (m_Health <= 0.0f) {
        m_Health = 0.0f;
        return true;
    }
    return false;
}

void Player::Heal(float amount) {
    m_Health = std::min(m_Health + amount, m_MaxHealth);
}

void Player::FullHeal() {
    m_Health = m_MaxHealth;
}

void Player::ApplyLifesteal(float damageDealt) {
    if (m_Vampirism > 0.0f) {
        Heal(damageDealt * m_Vampirism);
    }
}

bool Player::AddExp(int amount) {
    m_Exp += amount;
    if (m_Exp >= m_ExpNext) {
        m_Exp -= m_ExpNext;
        m_Level++;
        m_ExpNext = static_cast<int>(m_ExpNext * 1.15f);
        m_Health = m_MaxHealth; // Leveling up heals to full
        return true;
    }
    return false;
}

void Player::OffsetWorldPosition(const glm::vec2& offset) {
    m_WorldPosition += offset;
}

void Player::SetWorldPosition(const glm::vec2& pos) {
    m_WorldPosition = pos;
}

const glm::vec2& Player::GetWorldPosition() const {
    return m_WorldPosition;
}

const glm::vec2& Player::GetLastMoveDir() const {
    return m_LastMoveDir;
}

bool Player::IsFacingLeft() const {
    return m_IsFacingLeft;
}

float Player::GetCollisionRadius() const {
    return GetScaledSize().x * 0.35f;
}

float Player::GetHealth() const {
    return m_Health;
}

float Player::GetMaxHealth() const {
    return m_MaxHealth;
}

void Player::SetMaxHealth(float maxHealth) {
    m_MaxHealth = maxHealth;
}

void Player::UpgradeMaxHealth() {
    m_MaxHealthLevel++;
    m_MaxHealth += 100.0f;
    m_Health += 100.0f;
}

float Player::GetArmor() const {
    return m_Armor;
}

bool Player::IsArmorUnlocked() const {
    return m_ArmorUnlocked;
}

int Player::GetArmorLevel() const {
    return m_ArmorLevel;
}

void Player::UnlockArmor() {
    m_ArmorUnlocked = true;
    m_Armor = 0.05f;
}

void Player::UpgradeArmor() {
    m_ArmorLevel++;
    m_Armor += 0.05f;
}

float Player::GetVampirism() const {
    return m_Vampirism;
}

bool Player::IsVampirismUnlocked() const {
    return m_VampirismUnlocked;
}

int Player::GetVampirismLevel() const {
    return m_VampirismLevel;
}

void Player::UnlockVampirism() {
    m_VampirismUnlocked = true;
    m_Vampirism = 0.005f;
}

void Player::UpgradeVampirism() {
    m_VampirismLevel++;
    m_Vampirism += 0.005f;
}

int Player::GetLevel() const {
    return m_Level;
}

int Player::GetExp() const {
    return m_Exp;
}

int Player::GetExpNext() const {
    return m_ExpNext;
}

float Player::GetHitCooldownTimerMs() const {
    return m_HitCooldownTimerMs;
}

void Player::PauseAnimation() {
    if (m_RightWalkAnimation) m_RightWalkAnimation->Pause();
    if (m_LeftWalkAnimation) m_LeftWalkAnimation->Pause();
}

void Player::PlayAnimation() {
    if (m_RightWalkAnimation) m_RightWalkAnimation->Play();
    if (m_LeftWalkAnimation) m_LeftWalkAnimation->Play();
}

void Player::SetVisualState(bool isFacingLeft, bool isMoving, bool isHurt) {
    if (isHurt) {
        SetDrawable(isFacingLeft ? m_LeftHurtImage : m_RightHurtImage);
    } else if (isMoving) {
        auto anim = isFacingLeft ? m_LeftWalkAnimation : m_RightWalkAnimation;
        if (m_Drawable != anim) {
            SetDrawable(anim);
            anim->Play();
        }
    } else {
        SetDrawable(isFacingLeft ? m_LeftImage : m_RightImage);
    }
}
