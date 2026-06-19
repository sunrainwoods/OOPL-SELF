#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include "Util/Animation.hpp"
#include "config.hpp"

class Player : public Util::GameObject {
public:
    Player() {
        m_RightImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/right_main1.png");
        m_LeftImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/left_main1.png");
        m_RightHurtImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/hurt_right_main1.png");
        m_LeftHurtImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/hurt_left_main1.png");
        
        std::vector<std::string> rightWalkFrames;
        std::vector<std::string> leftWalkFrames;
        for (int i = 1; i <= 3; ++i) {
            rightWalkFrames.push_back(std::string(RESOURCE_DIR) + "/right_main_walk" + std::to_string(i) + ".png");
            leftWalkFrames.push_back(std::string(RESOURCE_DIR) + "/left_main_walk" + std::to_string(i) + ".png");
        }
        
        m_RightWalkAnimation = std::make_shared<Util::Animation>(rightWalkFrames, false, 150, true, 0);
        m_LeftWalkAnimation = std::make_shared<Util::Animation>(leftWalkFrames, false, 150, true, 0);

        SetDrawable(m_RightImage);
        SetZIndex(5.0f);
        m_Transform.scale = {1.0f, 1.0f};
    }

    void SetState(bool isFacingLeft, bool isMoving, bool isHurt) {
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

    void PauseAnimation() {
        m_RightWalkAnimation->Pause();
        m_LeftWalkAnimation->Pause();
    }

    void PlayAnimation() {
        m_RightWalkAnimation->Play();
        m_LeftWalkAnimation->Play();
    }

private:
    std::shared_ptr<Util::Image> m_RightImage;
    std::shared_ptr<Util::Image> m_LeftImage;
    std::shared_ptr<Util::Image> m_RightHurtImage;
    std::shared_ptr<Util::Image> m_LeftHurtImage;
    std::shared_ptr<Util::Animation> m_RightWalkAnimation;
    std::shared_ptr<Util::Animation> m_LeftWalkAnimation;
};

#endif