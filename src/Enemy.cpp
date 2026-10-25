#include "Enemy.hpp"
#include "Camera.hpp"
#include <algorithm>

Enemy::Enemy() {
    InitObject();
}

void Enemy::InitObject() {
    if (!m_Object) {
        m_Object = std::make_shared<Util::GameObject>();
        m_Object->SetVisible(false);
    }
}

void Enemy::Spawn(const glm::vec2& pos, float health, float speed, float damage,
                  float scale, float zIndex,
                  std::shared_ptr<Util::Image> defaultImage,
                  std::shared_ptr<Util::Image> hurtImage,
                  std::shared_ptr<Util::Animation> defaultAnim,
                  bool isBoss, bool isPercentDamage) {
    m_WorldPosition = pos;
    m_Health = health;
    m_MaxHealth = health;
    m_Speed = speed;
    m_Damage = damage;
    m_DefaultImage = defaultImage;
    m_HurtImage = hurtImage;
    m_DefaultAnimation = defaultAnim;
    m_IsBoss = isBoss;
    m_IsPercentDamage = isPercentDamage;
    m_HitCooldownTimerMs = 0.0f;
    m_IsHurtImageSet = false;

    if (m_DefaultAnimation) {
        m_Object->SetDrawable(m_DefaultAnimation);
    } else {
        m_Object->SetDrawable(m_DefaultImage);
    }

    m_Object->SetZIndex(zIndex);
    m_Object->m_Transform.scale = {scale, scale};
    m_Object->SetVisible(true);
    m_Active = true;
}

void Enemy::UpdateHurtState(float dtMs) {
    if (m_HitCooldownTimerMs > 0.0f) {
        m_HitCooldownTimerMs = std::max(0.0f, m_HitCooldownTimerMs - dtMs);
        if (!m_IsHurtImageSet) {
            if (m_HurtImage) m_Object->SetDrawable(m_HurtImage);
            m_IsHurtImageSet = true;
        }
    } else {
        if (m_IsHurtImageSet) {
            if (!m_IsBoss) {
                if (m_DefaultAnimation) {
                    m_Object->SetDrawable(m_DefaultAnimation);
                } else if (m_DefaultImage) {
                    m_Object->SetDrawable(m_DefaultImage);
                }
            }
            m_IsHurtImageSet = false;
        }
    }
}

void Enemy::UpdateMovement(float dtMs, const glm::vec2& direction, const glm::vec2& separation) {
    m_WorldPosition += (direction * (m_Speed / 1000.0f)) * dtMs + separation;
}

void Enemy::SyncScreenPosition(const Camera& camera) {
    m_Object->m_Transform.translation = camera.WorldToScreen(m_WorldPosition);
}

void Enemy::TakeDamage(float amount) {
    m_HitCooldownTimerMs = 150.0f;
    m_Health -= amount;
    if (!m_IsHurtImageSet && m_HurtImage) {
        m_Object->SetDrawable(m_HurtImage);
        m_IsHurtImageSet = true;
    }
}

bool Enemy::IsDead() const {
    return m_Health <= 0.0f;
}

void Enemy::Deactivate() {
    m_Active = false;
    if (m_Object) {
        m_Object->SetVisible(false);
    }
}

float Enemy::GetCollisionRadius() const {
    return m_Object ? (m_Object->GetScaledSize().x * 0.35f) : 0.0f;
}

void Enemy::SetBossVisuals(std::shared_ptr<Util::Animation> anim, std::shared_ptr<Util::Image> hurtImg) {
    m_DefaultAnimation = anim;
    m_HurtImage = hurtImg;
    if (m_HitCooldownTimerMs <= 0.0f && m_DefaultAnimation) {
        m_Object->SetDrawable(m_DefaultAnimation);
    }
}

void Enemy::Draw(const Camera& camera) {
    if (!m_Active || !m_Object) return;

    if (camera.IsInView(m_WorldPosition)) {
        if (m_HitCooldownTimerMs > 75.0f && m_HurtImage) {
            m_Object->SetDrawable(m_HurtImage);
        } else {
            if (m_DefaultAnimation) {
                m_Object->SetDrawable(m_DefaultAnimation);
            } else if (m_DefaultImage) {
                m_Object->SetDrawable(m_DefaultImage);
            }
        }
        m_Object->Draw();
    }
}
