#ifndef ENEMY_MANAGER_HPP
#define ENEMY_MANAGER_HPP

#include <vector>
#include <memory>
#include <functional>
#include <string>
#include <unordered_map>
#include <glm/vec2.hpp>

#include "Enemy.hpp"
#include "Util/Image.hpp"
#include "Util/Animation.hpp"

class Player;
class Camera;

class EnemyManager {
public:
    EnemyManager();

    void Init(const std::string& resourceDir);
    void Reset();

    void Update(float dtMs, Player& player, const Camera& camera,
                const std::function<void(Enemy&)>& onEnemyKilled,
                bool& playerDied);
    void Draw(const Camera& camera);

    void PauseAnimations();
    void PlayAnimations();

    void OnEnemyKilled(Enemy& enemy);

    std::vector<Enemy*> GetActiveEnemies();

    int GetCurrentWave() const { return m_CurrentWave; }
    int GetCurrentStage() const { return m_CurrentStage; }
    int GetEnemiesDefeated() const { return m_EnemiesDefeated; }
    float GetEnemyWidthRatioToPlayer() const { return m_EnemyWidthRatioToPlayer; }

private:
    void SpawnRegularEnemies(const glm::vec2& playerWorldPos, float playerWidth);
    void SpawnBossWave(const glm::vec2& playerWorldPos, float playerWidth);

    std::vector<Enemy> m_Enemies;
    int m_MaxEnemies = 5000;

    // Assets
    std::shared_ptr<Util::Image> m_Enemy1Image;
    std::shared_ptr<Util::Image> m_Enemy1HurtImage;
    std::shared_ptr<Util::Image> m_Enemy2Image;
    std::shared_ptr<Util::Image> m_Enemy2HurtImage;
    std::shared_ptr<Util::Image> m_Enemy3Image;
    std::shared_ptr<Util::Image> m_Enemy3HurtImage;
    std::shared_ptr<Util::Image> m_Enemy4Image;
    std::shared_ptr<Util::Image> m_Enemy4HurtImage;

    std::shared_ptr<Util::Animation> m_BossLeftAnimation;
    std::shared_ptr<Util::Animation> m_BossRightAnimation;
    std::shared_ptr<Util::Image> m_BossHurtLeftImage;
    std::shared_ptr<Util::Image> m_BossHurtRightImage;

    // Wave parameters
    int m_EnemiesDefeated = 0;
    int m_CurrentWave = 1;
    int m_CurrentStage = 1;
    int m_KillsToNextWave = 10;
    int m_LastSpecialWaveTriggered = 0;

    float m_EnemySpawnTimerMs = 0.0f;
    float m_EnemySpawnIntervalMs = 1200.0f;
    float m_EnemySpawnMinDistance = 850.0f;
    float m_EnemySpawnMaxDistance = 1200.0f;
    float m_EnemyWidthRatioToPlayer = 0.85f;
};

#endif // ENEMY_MANAGER_HPP
