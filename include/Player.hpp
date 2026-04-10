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
        SetDrawable(m_RightImage);
        SetZIndex(5.0f);
        m_Transform.scale = {1.0f, 1.0f};
    }

    void SetFacingLeft(bool isFacingLeft) {
        if (isFacingLeft) {
            SetDrawable(m_LeftImage);
        } else {
            SetDrawable(m_RightImage);
        }
    }

private:
    std::shared_ptr<Util::Image> m_RightImage;
    std::shared_ptr<Util::Image> m_LeftImage;
};

#endif