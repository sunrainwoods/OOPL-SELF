#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <memory>
#include <string>
#include <vector>
#include <glm/vec2.hpp>

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include "Util/Animation.hpp"

class Camera;

class Player : public Util::GameObject {
public:
    Player();

    void Reset();
    void Update(float dtMs, const Camera& camera);
    void UpdateMovement(float dtMs, const glm::vec2& moveDir);

    bool TakeDamage(float damage);
    void Heal(float amount);
    void FullHeal();
    void ApplyLifesteal(float damageDealt);
    bool AddExp(int amount);

    void OffsetWorldPosition(const glm::vec2& offset);
    void SetWorldPosition(const glm::vec2& pos);
    const glm::vec2& GetWorldPosition() const;
    const glm::vec2& GetLastMoveDir() const;
    bool IsFacingLeft() const;
    float GetCollisionRadius() const;

    float GetHealth() const;
    float GetMaxHealth() const;
    void SetMaxHealth(float maxHealth);
    void UpgradeMaxHealth();

    float GetArmor() const;
    bool IsArmorUnlocked() const;
    int GetArmorLevel() const;
    void UnlockArmor();
    void UpgradeArmor();

    float GetVampirism() const;
    bool IsVampirismUnlocked() const;
    int GetVampirismLevel() const;
    void UnlockVampirism();
    void UpgradeVampirism();

    int GetLevel() const;
    int GetExp() const;
    int GetExpNext() const;

    float GetHitCooldownTimerMs() const;

    void PauseAnimation();
    void PlayAnimation();

private:
    void SetVisualState(bool isFacingLeft, bool isMoving, bool isHurt);

    // Visual assets
    std::shared_ptr<Util::Image> m_RightImage;
    std::shared_ptr<Util::Image> m_LeftImage;
    std::shared_ptr<Util::Image> m_RightHurtImage;
    std::shared_ptr<Util::Image> m_LeftHurtImage;
    std::shared_ptr<Util::Animation> m_RightWalkAnimation;
    std::shared_ptr<Util::Animation> m_LeftWalkAnimation;

    // Transform and movement
    glm::vec2 m_WorldPosition = {0.0f, 0.0f};
    glm::vec2 m_LastMoveDir = {1.0f, 0.0f};
    bool m_IsFacingLeft = false;
    bool m_IsMoving = false;
    float m_PlayerScale = 0.7f;
    float m_MoveSpeed = 0.35f; // pixels per millisecond

    // Health & combat stats
    float m_MaxHealth = 100.0f;
    float m_Health = 100.0f;
    float m_HitCooldownTimerMs = 0.0f;
    float m_Armor = 0.0f;
    float m_Vampirism = 0.0f;
    bool m_ArmorUnlocked = false;
    bool m_VampirismUnlocked = false;

    // Levels
    int m_MaxHealthLevel = 1;
    int m_ArmorLevel = 1;
    int m_VampirismLevel = 1;
    int m_Level = 1;
    int m_Exp = 0;
    int m_ExpNext = 10;
};

#endif // PLAYER_HPP