#ifndef ENEMY_HPP
#define ENEMY_HPP

#include <memory>
#include <glm/vec2.hpp>

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include "Util/Animation.hpp"

class Camera;

class Enemy {
public:
    Enemy();

    void InitObject();
    void Spawn(const glm::vec2& pos, float health, float speed, float damage,
               float scale, float zIndex,
               std::shared_ptr<Util::Image> defaultImage,
               std::shared_ptr<Util::Image> hurtImage,
               std::shared_ptr<Util::Animation> defaultAnim = nullptr,
               bool isBoss = false, bool isPercentDamage = false);

    void UpdateHurtState(float dtMs);
    void UpdateMovement(float dtMs, const glm::vec2& direction, const glm::vec2& separation);
    void SyncScreenPosition(const Camera& camera);

    void TakeDamage(float amount);
    bool IsDead() const;
    void Deactivate();

    float GetCollisionRadius() const;
    void Draw(const Camera& camera);

    bool IsActive() const { return m_Active; }
    void SetActive(bool active) { m_Active = active; }

    const glm::vec2& GetWorldPosition() const { return m_WorldPosition; }
    void SetWorldPosition(const glm::vec2& pos) { m_WorldPosition = pos; }

    float GetHealth() const { return m_Health; }
    float GetMaxHealth() const { return m_MaxHealth; }
    float GetSpeed() const { return m_Speed; }
    float GetDamage() const { return m_Damage; }
    bool IsPercentDamage() const { return m_IsPercentDamage; }
    bool IsBoss() const { return m_IsBoss; }

    float GetHitCooldownTimerMs() const { return m_HitCooldownTimerMs; }
    void SetHitCooldownTimerMs(float timerMs) { m_HitCooldownTimerMs = timerMs; }

    void SetBossVisuals(std::shared_ptr<Util::Animation> anim, std::shared_ptr<Util::Image> hurtImg);

    std::shared_ptr<Util::GameObject> GetGameObject() const { return m_Object; }

private:
    std::shared_ptr<Util::GameObject> m_Object;
    glm::vec2 m_WorldPosition = {0.0f, 0.0f};
    bool m_Active = false;
    float m_Health = 100.0f;
    float m_MaxHealth = 100.0f;
    float m_Speed = 100.0f;
    float m_Damage = 10.0f;
    float m_HitCooldownTimerMs = 0.0f;

    std::shared_ptr<Util::Image> m_DefaultImage;
    std::shared_ptr<Util::Animation> m_DefaultAnimation;
    std::shared_ptr<Util::Image> m_HurtImage;

    bool m_IsBoss = false;
    bool m_IsHurtImageSet = false;
    bool m_IsPercentDamage = false;
};

#endif // ENEMY_HPP
