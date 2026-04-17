#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include "config.hpp"

class Player : public Util::GameObject {
public:
    Player() {
        m_RightImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/right_main1.png");
        m_LeftImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/left_main1.png");
        m_RightHurtImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/hurt_right_main1.png");
        m_LeftHurtImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/hurt_left_main1.png");
        SetDrawable(m_RightImage);
        SetZIndex(5.0f);
        m_Transform.scale = {1.0f, 1.0f};
    }

    void SetState(bool isFacingLeft, bool isHurt) {
        if (isFacingLeft) {
            SetDrawable(isHurt ? m_LeftHurtImage : m_LeftImage);
        } else {
            SetDrawable(isHurt ? m_RightHurtImage : m_RightImage);
        }
    }

private:
    std::shared_ptr<Util::Image> m_RightImage;
    std::shared_ptr<Util::Image> m_LeftImage;
    std::shared_ptr<Util::Image> m_RightHurtImage;
    std::shared_ptr<Util::Image> m_LeftHurtImage;
};

#endif