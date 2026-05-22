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
    float speed = 100.0f; // 怪物移速
    float damage = 10.0f; // 怪物攻擊力
    float hitCooldownTimerMs = 0.0f; // 被打到的無敵冷卻時間
    std::shared_ptr<Util::Image> defaultImage; // 原始怪物的圖片
    std::shared_ptr<Util::Image> hurtImage;    // 全白怪物的圖片
};

struct ExpGem {
    std::shared_ptr<Util::GameObject> object;
    glm::vec2 worldPosition = {0.0f, 0.0f};
    bool active = false;
    int expValue = 1;
    float pickupCooldownTimerMs = 0.0f; // 避免剛掉落就被吃掉的冷卻時間
};

struct HealthItem {
    std::shared_ptr<Util::GameObject> object;
    glm::vec2 worldPosition = {0.0f, 0.0f};
    bool active = false;
    float pickupCooldownTimerMs = 0.0f; // 避免剛掉落就被吃掉的冷卻時間
};

struct Knife {
    std::shared_ptr<Util::GameObject> object;
    glm::vec2 worldPosition = {0.0f, 0.0f};
    glm::vec2 velocity = {0.0f, 0.0f};
    bool active = false;
    float timeToLiveMs = 0.0f;
    float angle = 0.0f; // 用於旋轉圖片
};

struct Runetracer {
    std::shared_ptr<Util::GameObject> object;
    glm::vec2 worldPosition = {0.0f, 0.0f};
    glm::vec2 velocity = {0.0f, 0.0f};
    bool active = false;
    float timeToLiveMs = 0.0f;
    float angle = 0.0f;
    
    std::vector<glm::vec2> historyPositions;
    int maxHistory = 40; // 加長尾流
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
    
    // 飛刀武器相關變數
    std::vector<Knife> m_Knives;
    int m_MaxKnives = 50;
    std::shared_ptr<Util::Image> m_KnifeImage;
    float m_KnifeAttackIntervalMs = 800.0f;
    float m_KnifeAttackTimerMs = 0.0f;
    float m_KnifeSpeed = 600.0f;
    float m_KnifeDamage = 25.0f;
    glm::vec2 m_PlayerLastMoveDir = {1.0f, 0.0f};

    // 符文追蹤者武器相關變數
    std::vector<Runetracer> m_Runetracers;
    int m_MaxRunetracers = 30;
    std::shared_ptr<Util::Image> m_RunetracerImage;
    std::shared_ptr<Util::Image> m_RunetracerImage95;
    std::shared_ptr<Util::Image> m_RunetracerImage80;
    std::shared_ptr<Util::Image> m_RunetracerImage65;
    std::shared_ptr<Util::Image> m_RunetracerImage50;
    float m_RunetracerAttackIntervalMs = 1500.0f;
    float m_RunetracerAttackTimerMs = 0.0f;
    float m_RunetracerSpeed = 400.0f;
    float m_RunetracerDamage = 15.0f;

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
    int m_CurrentStage = 1;    // 關卡系統
    float m_GameTimeMs = 0.0f; // 遊戲經過時間

    std::vector<ExpGem> m_ExpGems;
    int m_MaxExpGems = 100;

    std::vector<HealthItem> m_HealthItems;
    int m_MaxHealthItems = 20;

    std::shared_ptr<Util::Image> m_Enemy1Image;
    std::shared_ptr<Util::Image> m_Enemy1HurtImage;
    std::shared_ptr<Util::Image> m_Enemy2Image;
    std::shared_ptr<Util::Image> m_Enemy2HurtImage;
    std::shared_ptr<Util::Image> m_Enemy3Image;
    std::shared_ptr<Util::Image> m_Enemy3HurtImage;

    std::shared_ptr<Util::Image> m_Gem1Image;
    std::shared_ptr<Util::Image> m_Gem2Image;
    std::shared_ptr<Util::Image> m_Gem3Image;
    std::shared_ptr<Util::Image> m_HealthImage;

    std::shared_ptr<Util::Image> m_LevelUpImage;
    std::shared_ptr<Util::GameObject> m_LevelUpObject;
};

#endif
