#ifndef BACKGROUND_HPP
#define BACKGROUND_HPP

#include <memory>
#include <vector>
#include <string>
#include <glm/vec2.hpp>

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"

class Camera;

struct BackgroundTile {
    std::shared_ptr<Util::GameObject> object;
    std::shared_ptr<Util::Image> image;
};

class Background {
public:
    Background();

    void Init(const std::string& groundPath);
    void Update(const Camera& camera);
    void Draw();

private:
    std::string m_GroundPath;
    glm::vec2 m_BackgroundTileSize = {1.0f, 1.0f};
    std::vector<BackgroundTile> m_BackgroundTiles;
};

#endif // BACKGROUND_HPP
