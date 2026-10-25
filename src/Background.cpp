#include "Background.hpp"
#include "Camera.hpp"
#include "config.hpp"
#include <cmath>

Background::Background() = default;

void Background::Init(const std::string& groundPath) {
    m_GroundPath = groundPath;
    m_BackgroundTiles.clear();

    auto tileObject = std::make_shared<Util::GameObject>();
    auto tileImage = std::make_shared<Util::Image>(m_GroundPath);
    tileObject->SetDrawable(tileImage);
    tileObject->SetZIndex(-10.0f);
    m_BackgroundTiles.push_back({tileObject, tileImage});
    m_BackgroundTileSize = m_BackgroundTiles.front().object->GetScaledSize();
}

void Background::Update(const Camera& camera) {
    const float tileWidth = m_BackgroundTileSize.x;
    const float tileHeight = m_BackgroundTileSize.y;
    const float halfWindowWidth = static_cast<float>(WINDOW_WIDTH) * 0.5f;
    const float halfWindowHeight = static_cast<float>(WINDOW_HEIGHT) * 0.5f;

    const glm::vec2& camPos = camera.GetPosition();

    const int minTileX = static_cast<int>(std::floor((camPos.x - halfWindowWidth) / tileWidth)) - 1;
    const int maxTileX = static_cast<int>(std::floor((camPos.x + halfWindowWidth) / tileWidth)) + 1;
    const int minTileY = static_cast<int>(std::floor((camPos.y - halfWindowHeight) / tileHeight)) - 1;
    const int maxTileY = static_cast<int>(std::floor((camPos.y + halfWindowHeight) / tileHeight)) + 1;

    const int requiredTiles = (maxTileX - minTileX + 1) * (maxTileY - minTileY + 1);
    while (static_cast<int>(m_BackgroundTiles.size()) < requiredTiles) {
        auto extraTileObject = std::make_shared<Util::GameObject>();
        auto extraTileImage = std::make_shared<Util::Image>(m_GroundPath);
        extraTileObject->SetDrawable(extraTileImage);
        extraTileObject->SetZIndex(-10.0f);
        m_BackgroundTiles.push_back({extraTileObject, extraTileImage});
    }

    int tileIndex = 0;
    for (int worldTileY = minTileY; worldTileY <= maxTileY; ++worldTileY) {
        for (int worldTileX = minTileX; worldTileX <= maxTileX; ++worldTileX) {
            const float worldPosX = (static_cast<float>(worldTileX) + 0.5f) * tileWidth;
            const float worldPosY = (static_cast<float>(worldTileY) + 0.5f) * tileHeight;

            auto& tile = m_BackgroundTiles[static_cast<size_t>(tileIndex)];
            tile.object->m_Transform.translation = camera.WorldToScreen({worldPosX, worldPosY});
            ++tileIndex;
        }
    }
}

void Background::Draw() {
    for (auto& tile : m_BackgroundTiles) {
        tile.object->Draw();
    }
}
