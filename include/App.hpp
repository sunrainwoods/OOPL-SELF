#ifndef APP_HPP
#define APP_HPP

#include "pch.hpp" // IWYU pragma: export
#include "Player.hpp"
#include "Util/Animation.hpp"
#include "Util/GameObject.hpp"
#include "Util/Image.hpp"

struct BackgroundTile {
    std::shared_ptr<Util::GameObject> object;
    std::shared_ptr<Util::Image> image;
};

struct WeaponEffect {
    std::shared_ptr<Util::GameObject> object;
    std::shared_ptr<Util::Animation> rightAnimation;
    std::shared_ptr<Util::Animation> leftAnimation;
    std::shared_ptr<Util::Animation> activeAnimation;
};

struct EnemyUnit {
    std::shared_ptr<Util::GameObject> object;
    glm::vec2 worldPosition = {0.0f, 0.0f};
    bool active = false; // Add active flag for Object Pool
    float health = 100.0f; // 怪物血量
    float maxHealth = 100.0f;
    float hitCooldownTimerMs = 0.0f; // 被打到的無敵冷卻時間
};

struct ExpGem {
    std::shared_ptr<Util::GameObject> object;
    glm::vec2 worldPosition = {0.0f, 0.0f};
    bool active = false;
    int expValue = 1;
    float pickupCooldownTimerMs = 0.0f; // 避免剛掉落就被吃掉的冷卻時間
};

class App {
public:
    enum class State {
        TITLE, // 新增標題狀態
        START,
        UPDATE,
        LEVEL_UP,
        GAME_OVER,
        PAUSED,
        END,
    };

    State GetCurrentState() const { return m_CurrentState; }

    void Init(); // 真正初始化資源的地方
    void UpdateTitle(); // 處理標題畫面
    void Start();

    void Update();
    void UpdateLevelUp();
    void UpdateGameOver();
    void UpdatePaused();
    void DrawGameObjects();

    void End(); // NOLINT(readability-convert-member-functions-to-static)

private:
    void ValidTask();

private:
    State m_CurrentState = State::TITLE;

    std::shared_ptr<Util::GameObject> m_TitleScreen;
    std::shared_ptr<Util::Image> m_TitleImage;

    std::shared_ptr<Player> m_Player;
    glm::vec2 m_PlayerWorldPosition = {0.0f, 0.0f};
    glm::vec2 m_CameraPosition = {0.0f, 0.0f};

    // 玩家狀態
    float m_PlayerMaxHealth = 100.0f;
    float m_PlayerHealth = 100.0f;
    float m_PlayerHitCooldownTimerMs = 0.0f;
    int m_PlayerLevel = 1;
    int m_PlayerExp = 0;
    int m_PlayerExpNext = 10;
    
    std::vector<BackgroundTile> m_BackgroundTiles;
    std::string m_GroundPath;
    glm::vec2 m_BackgroundTileSize = {1.0f, 1.0f};

    WeaponEffect m_WeaponEffect;
    bool m_IsFacingLeft = false;
    float m_WeaponAttackTimerMs = 0.0f;
    float m_WeaponAttackIntervalMs = 1000.0f;
    float m_PlayerScale = 0.7f;
    float m_WeaponWidthRatioToPlayer = 1.5f;

    std::size_t m_WeaponHitStartFrame = 2;
    std::size_t m_WeaponHitEndFrame = 4;
    float m_WeaponHitRadiusRatioToPlayer = 1.0f;

    std::vector<EnemyUnit> m_Enemies;
    std::string m_EnemyPath;
    float m_EnemySpawnTimerMs = 0.0f;
    float m_EnemySpawnIntervalMs = 1200.0f;
    int m_MaxEnemies = 30;
    float m_EnemySpawnMinDistance = 450.0f;
    float m_EnemySpawnMaxDistance = 850.0f;
    float m_EnemyMoveSpeed = 0.12f;
    float m_EnemyWidthRatioToPlayer = 0.85f;
    float m_WeaponDamage = 35.0f; // 武器傷害
    float m_ExpGemSizeRatioToPlayer = 0.6f; // 寶石大小比例

    int m_EnemiesDefeated = 0; // 記錄擊殺數
    int m_CurrentWave = 1;     // 波次系統
    
    std::vector<ExpGem> m_ExpGems;
    int m_MaxExpGems = 100;
};

#endif
