#ifndef DROP_ITEM_HPP
#define DROP_ITEM_HPP

#include <memory>
#include <glm/vec2.hpp>
#include "Util/GameObject.hpp"

struct ExpGem {
    std::shared_ptr<Util::GameObject> object;
    glm::vec2 worldPosition = {0.0f, 0.0f};
    bool active = false;
    int expValue = 1;
    float pickupCooldownTimerMs = 0.0f;
    bool isMagnetized = false;
};

struct HealthItem {
    std::shared_ptr<Util::GameObject> object;
    glm::vec2 worldPosition = {0.0f, 0.0f};
    bool active = false;
    float pickupCooldownTimerMs = 0.0f;
};

struct MagnetItem {
    std::shared_ptr<Util::GameObject> object;
    glm::vec2 worldPosition = {0.0f, 0.0f};
    bool active = false;
    float pickupCooldownTimerMs = 0.0f;
};

struct RewardItem {
    std::shared_ptr<Util::GameObject> object;
    glm::vec2 worldPosition = {0.0f, 0.0f};
    bool active = false;
    float pickupCooldownTimerMs = 0.0f;
};

#endif // DROP_ITEM_HPP
