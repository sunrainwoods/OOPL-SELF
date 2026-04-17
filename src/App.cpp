#include "App.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

#include <imgui.h>

#include "Util/Image.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"
#include "Util/Time.hpp"

#include "config.hpp"

void App::UpdateTitle() {
    if (!m_TitleScreen) {
        // 第一幀，載入標題畫面圖片
        m_TitleScreen = std::make_shared<Util::GameObject>();
        m_TitleImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/Vampire_Survivors_start.jpg");
        m_TitleScreen->SetDrawable(m_TitleImage);
        m_TitleScreen->SetZIndex(99.0f);
        
        // 依照圖片與視窗比例縮放填滿 (或自訂大小)
        const glm::vec2 imageSize = m_TitleImage->GetSize();
        m_TitleScreen->m_Transform.scale = {
            static_cast<float>(WINDOW_WIDTH) / imageSize.x, 
            static_cast<float>(WINDOW_HEIGHT) / imageSize.y
        };
    }

    // 讓標題畫面跟著攝影機放在正中央
    m_TitleScreen->m_Transform.translation = glm::vec2(0.0f, 0.0f);
    m_TitleScreen->Draw();

    // 在畫面上用 ImGui 顯示閃爍的 "PRESS TO START" 或任意鍵提示
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT), ImGuiCond_Always);
    ImGui::Begin("TitleScreenUI", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove);

    // 每秒內閃動 0.5 秒
    if (static_cast<int>(Util::Time::GetElapsedTimeMs() / 500.0f) % 2 == 0) {
        ImVec2 textSize = ImGui::CalcTextSize("PRESS ANY KEY TO START");
        ImGui::SetCursorPos(ImVec2((WINDOW_WIDTH - textSize.x) * 0.5f, WINDOW_HEIGHT * 0.75f));
        ImGui::TextColored(ImVec4(1, 1, 1, 1), "PRESS ANY KEY TO START");
    }

    ImGui::End();

    // 如果按下鍵盤任意鍵
    if (Util::Input::IfAnyKeyPressed()) {
        m_CurrentState = State::START;
    }
}

void App::Start() {
    LOG_TRACE("Start");

    // 重設數值，避免 Restart 疊加
    m_WeaponAttackIntervalMs = 1000.0f;
    m_WeaponWidthRatioToPlayer = 1.5f;
    m_WeaponHitRadiusRatioToPlayer = 1.0f;
    m_WeaponDamage = 35.0f;
    m_PlayerMaxHealth = 100.0f;
    m_PlayerHealth = 100.0f;
    m_PlayerLevel = 1;
    m_PlayerExp = 0;
    m_PlayerExpNext = 10;
    m_PlayerHitCooldownTimerMs = 0.0f;
    m_EnemiesDefeated = 0;
    m_CurrentWave = 1;
    m_CurrentStage = 1;
    m_GameTimeMs = 0.0f;
    m_EnemySpawnIntervalMs = 1200.0f;

    m_GroundPath = std::string(RESOURCE_DIR) + "/ground.png";
    m_EnemyPath = std::string(RESOURCE_DIR) + "/enemy1.png";

    m_BackgroundTiles.clear();
    auto tileObject = std::make_shared<Util::GameObject>();
    auto tileImage = std::make_shared<Util::Image>(m_GroundPath);
    tileObject->SetDrawable(tileImage);
    tileObject->SetZIndex(-10.0f);
    m_BackgroundTiles.push_back({tileObject, tileImage});
    m_BackgroundTileSize = m_BackgroundTiles.front().object->GetScaledSize();

    m_Player = std::make_shared<Player>();
    m_Player->m_Transform.scale = {m_PlayerScale, m_PlayerScale};

    m_PlayerWorldPosition = {0.0f, 0.0f};
    m_CameraPosition = m_PlayerWorldPosition;
    m_Player->m_Transform.translation = {0.0f, 0.0f};

    std::vector<std::string> weaponRightFrames;
    std::vector<std::string> weaponLeftFrames;
    weaponRightFrames.reserve(5);
    weaponLeftFrames.reserve(5);
    for (int i = 1; i <= 5; ++i) {
        weaponRightFrames.push_back(std::string(RESOURCE_DIR) + "/right_slash" +
                                    std::to_string(i) + ".png");
        weaponLeftFrames.push_back(std::string(RESOURCE_DIR) +
                                   "/left_slash" +
                                   std::to_string(i) + ".png");
    }

    m_WeaponEffect.rightAnimation =
        std::make_shared<Util::Animation>(weaponRightFrames, false, 50, false, 0);
    m_WeaponEffect.leftAnimation =
        std::make_shared<Util::Animation>(weaponLeftFrames, false, 50, false, 0);
    m_WeaponEffect.activeAnimation = m_WeaponEffect.rightAnimation;

    m_WeaponEffect.object = std::make_shared<Util::GameObject>();
    m_WeaponEffect.object->SetDrawable(m_WeaponEffect.activeAnimation);
    m_WeaponEffect.object->SetZIndex(7.0f);
    m_WeaponEffect.object->SetVisible(false);

    const glm::vec2 playerSize = m_Player->GetScaledSize();
    const glm::vec2 weaponNativeSize = m_WeaponEffect.activeAnimation->GetSize();
    const float targetWeaponWidth = playerSize.x * m_WeaponWidthRatioToPlayer;
    const float weaponScale = targetWeaponWidth / weaponNativeSize.x;
    m_WeaponEffect.object->m_Transform.scale = {weaponScale, weaponScale};

    // Initialize enemy object pool
    m_Enemies.clear();
    m_Enemies.reserve(m_MaxEnemies);
    
    // 預先載入兩張圖片，然後把指標指派給每隻怪物，可以省下很多記憶體
    auto sharedEnemyImage = std::make_shared<Util::Image>(m_EnemyPath);
    auto sharedEnemyHurtImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/hurt_enemy1.png");

    for (int i = 0; i < m_MaxEnemies; ++i) {
        EnemyUnit enemy;
        enemy.object = std::make_shared<Util::GameObject>();
        enemy.defaultImage = sharedEnemyImage;
        enemy.hurtImage = sharedEnemyHurtImage;
        
        enemy.object->SetDrawable(enemy.defaultImage);
        enemy.object->SetZIndex(4.0f);
        enemy.object->SetVisible(false);
        const float targetEnemyWidth = playerSize.x * m_EnemyWidthRatioToPlayer;
        const float enemyScale = targetEnemyWidth / sharedEnemyImage->GetSize().x;
        enemy.object->m_Transform.scale = {enemyScale, enemyScale};
        enemy.active = false;
        enemy.health = enemy.maxHealth;
        enemy.hitCooldownTimerMs = 0.0f;
        m_Enemies.push_back(enemy);
    }

    m_EnemySpawnTimerMs = 0.0f;

    // 載入寶石與道具共用圖片
    m_Gem1Image = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/Experience_Gem1.png");
    m_Gem2Image = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/Experience_Gem2.png");
    m_Gem3Image = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/Experience_Gem3.png");
    m_HealthImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/check.png");

    // Initialize EXP Gem Object Pool
    m_ExpGems.clear();
    m_ExpGems.reserve(m_MaxExpGems);
    for (int i = 0; i < m_MaxExpGems; ++i) {
        ExpGem gem;
        gem.object = std::make_shared<Util::GameObject>();
        gem.object->SetDrawable(m_Gem1Image);
        gem.object->SetZIndex(3.0f); // 繪製在敵人下面，背景上面
        gem.object->SetVisible(false);
        gem.active = false;
        gem.expValue = 1;
        gem.pickupCooldownTimerMs = 0.0f;
        m_ExpGems.push_back(gem);
    }

    // Initialize Health Potion Object Pool
    m_HealthItems.clear();
    m_HealthItems.reserve(m_MaxHealthItems);
    for (int i = 0; i < m_MaxHealthItems; ++i) {
        HealthItem potion;
        potion.object = std::make_shared<Util::GameObject>();
        potion.object->SetDrawable(m_HealthImage);
        potion.object->SetZIndex(3.0f);
        potion.object->SetVisible(false);
        
        const float targetWidth = playerSize.x * m_ExpGemSizeRatioToPlayer;
        const float itemScale = targetWidth / m_HealthImage->GetSize().x;
        potion.object->m_Transform.scale = {itemScale, itemScale};
        potion.active = false;
        potion.pickupCooldownTimerMs = 0.0f;
        m_HealthItems.push_back(potion);
    }

    // Reset player states
    m_PlayerHealth = m_PlayerMaxHealth;
    m_PlayerLevel = 1;
    m_PlayerExp = 0;
    m_PlayerExpNext = 10;
    m_PlayerHitCooldownTimerMs = 0.0f;

    m_CurrentState = State::UPDATE;
}

void App::Update() {
    glm::vec2 moveDir = {0.0f, 0.0f};
    if (Util::Input::IsKeyPressed(Util::Keycode::W)) {
        moveDir.y += 1.0f;
    }
    if (Util::Input::IsKeyPressed(Util::Keycode::S)) {
        moveDir.y -= 1.0f;
    }
    if (Util::Input::IsKeyPressed(Util::Keycode::A)) {
        moveDir.x -= 1.0f;
    }
    if (Util::Input::IsKeyPressed(Util::Keycode::D)) {
        moveDir.x += 1.0f;
    }

    if (moveDir.x != 0.0f || moveDir.y != 0.0f) {
        constexpr float kMoveSpeed = 0.35f; // pixels per millisecond
        moveDir = glm::normalize(moveDir);
        m_PlayerWorldPosition +=
            moveDir * kMoveSpeed * Util::Time::GetDeltaTimeMs();
    }

    if (moveDir.x < 0.0f) {
        m_IsFacingLeft = true;
    } else if (moveDir.x > 0.0f) {
        m_IsFacingLeft = false;
    }
    // 玩家總無敵時間是 500ms，只要在 > 300ms 期間內顯示一次受擊白圖 (約亮白 200ms) 即可，避免像燈泡一樣閃爍
    m_Player->SetState(m_IsFacingLeft, m_PlayerHitCooldownTimerMs > 300.0f);

    // 解決武器動畫播到一半轉向造成的素材錯誤
    if (m_WeaponEffect.activeAnimation->GetState() == Util::Animation::State::PLAY) {
        auto correctAnimation = m_IsFacingLeft ? m_WeaponEffect.leftAnimation : m_WeaponEffect.rightAnimation;
        if (m_WeaponEffect.activeAnimation != correctAnimation) {
            std::size_t currentFrame = m_WeaponEffect.activeAnimation->GetCurrentFrameIndex();
            m_WeaponEffect.activeAnimation->Pause();
            
            m_WeaponEffect.activeAnimation = correctAnimation;
            m_WeaponEffect.object->SetDrawable(m_WeaponEffect.activeAnimation);
            m_WeaponEffect.activeAnimation->SetCurrentFrame(currentFrame);
            m_WeaponEffect.activeAnimation->Play();
        }
    }

    m_CameraPosition = m_PlayerWorldPosition;
    m_Player->m_Transform.translation = m_PlayerWorldPosition - m_CameraPosition;

    m_WeaponAttackTimerMs += Util::Time::GetDeltaTimeMs();
    if (m_WeaponAttackTimerMs >= m_WeaponAttackIntervalMs) {
        m_WeaponAttackTimerMs = 0.0f;
        m_WeaponEffect.activeAnimation =
            m_IsFacingLeft ? m_WeaponEffect.leftAnimation
                           : m_WeaponEffect.rightAnimation;
        m_WeaponEffect.object->SetDrawable(m_WeaponEffect.activeAnimation);
        m_WeaponEffect.activeAnimation->SetCurrentFrame(0);
        m_WeaponEffect.activeAnimation->Play();
        m_WeaponEffect.object->SetVisible(true);
    }

    if (m_WeaponEffect.activeAnimation->GetState() ==
        Util::Animation::State::ENDED) {
        m_WeaponEffect.object->SetVisible(false);
    }

    // Determine simple offset right next to the player
    const glm::vec2 playerSize = m_Player->GetScaledSize();
    const float sideOffset = playerSize.x * 0.7f; // simple side offset
    const glm::vec2 weaponOffset = m_IsFacingLeft ? glm::vec2{-sideOffset, 0.0f}
                                                  : glm::vec2{sideOffset, 0.0f};

    m_WeaponEffect.object->m_Transform.translation =
        (m_PlayerWorldPosition + weaponOffset) - m_CameraPosition;

    const float deltaTimeMs = Util::Time::GetDeltaTimeMs();
    m_GameTimeMs += deltaTimeMs; // 推進遊戲總時間
    m_EnemySpawnTimerMs += deltaTimeMs;

    // Object Pool: Count active enemies
    int activeEnemyCount = 0;
    for (const auto &enemy : m_Enemies) {
        if (enemy.active) {
            activeEnemyCount++;
        }
    }

    if (m_EnemySpawnTimerMs >= m_EnemySpawnIntervalMs &&
        activeEnemyCount < m_MaxEnemies) {
        m_EnemySpawnTimerMs = 0.0f;

        static std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> angleDist(0.0f,
                                                        glm::two_pi<float>());
        std::uniform_real_distribution<float> distanceDist(
            m_EnemySpawnMinDistance, m_EnemySpawnMaxDistance);

        const float angle = angleDist(rng);
        const float distance = distanceDist(rng);
        const glm::vec2 spawnOffset =
            glm::vec2{std::cos(angle), std::sin(angle)} * distance;

        // Find available enemy in pool
        for (auto &enemy : m_Enemies) {
            if (!enemy.active) {
                enemy.active = true;
                enemy.worldPosition = m_PlayerWorldPosition + spawnOffset;
                // 波次系統：依照波次疊加血量
                enemy.maxHealth = 100.0f + static_cast<float>(m_CurrentWave - 1) * 20.0f;
                enemy.health = enemy.maxHealth;
                enemy.hitCooldownTimerMs = 0.0f;
                enemy.object->SetVisible(true);
                break;
            }
        }
    }

    // 更新玩家無敵時間
    if (m_PlayerHitCooldownTimerMs > 0.0f) {
        m_PlayerHitCooldownTimerMs -= deltaTimeMs;
    }

    for (auto &enemy : m_Enemies) {
        if (!enemy.active) {
            continue;
        }

        // 減少敵人無敵時間
        if (enemy.hitCooldownTimerMs > 0.0f) {
            enemy.hitCooldownTimerMs -= deltaTimeMs;
        }

        const glm::vec2 toPlayer = m_PlayerWorldPosition - enemy.worldPosition;
        const float distanceToPlayer = glm::length(toPlayer);

        // 如果距離太近，玩家扣血
        if (distanceToPlayer < (playerSize.x * 0.5f) && m_PlayerHitCooldownTimerMs <= 0.0f) {
            m_PlayerHealth -= 10.0f; // 敵人傷害
            m_PlayerHitCooldownTimerMs = 500.0f; // 玩家 0.5秒無敵時間
            if (m_PlayerHealth <= 0.0f) {
                m_PlayerHealth = 0.0f;
                m_CurrentState = State::GAME_OVER; // 玩家死亡，切換狀態
                return;
            }
        }

        if (distanceToPlayer > 0.1f) {
            const glm::vec2 direction = toPlayer / distanceToPlayer;
            
            // 加入敵人之間彼此推擠(排斥)的碰撞體積 (簡單的分離力)
            glm::vec2 separation(0.0f);
            for (const auto &otherEnemy : m_Enemies) {
                if (!otherEnemy.active || &enemy == &otherEnemy) continue;
                
                glm::vec2 toOther = enemy.worldPosition - otherEnemy.worldPosition;
                float dist = glm::length(toOther);
                float minSeparation = playerSize.x * 0.5f; // 以主角體型一半作為怪物碰撞半徑

                // 如果距離太近，產生與之反向的分離力
                if (dist > 0.0f && dist < minSeparation) {
                    separation += (toOther / dist) * (minSeparation - dist) * 0.05f; // 大幅降低推擠力道，避免瞬移
                }
            }

            // 結合追逐與推擠力，同時限制分離力大小不要超過移動速度太多
            if (glm::length(separation) > m_EnemyMoveSpeed * 1.5f) {
                separation = glm::normalize(separation) * (m_EnemyMoveSpeed * 1.5f);
            }

            enemy.worldPosition += (direction * m_EnemyMoveSpeed + separation) * deltaTimeMs;
        }

        enemy.object->m_Transform.translation =
            enemy.worldPosition - m_CameraPosition;
    }

    bool weaponCanHit = false;
    if (m_WeaponEffect.activeAnimation->GetState() == Util::Animation::State::PLAY) {
        const std::size_t currentFrame =
            m_WeaponEffect.activeAnimation->GetCurrentFrameIndex();
        weaponCanHit = currentFrame >= m_WeaponHitStartFrame &&
                       currentFrame <= m_WeaponHitEndFrame;
    }

    if (weaponCanHit) {
        const glm::vec2 weaponHitCenter = m_PlayerWorldPosition + weaponOffset;
        const float hitRadius =
            playerSize.x * m_WeaponHitRadiusRatioToPlayer;

        for (auto &enemy : m_Enemies) {
            if (enemy.active && enemy.hitCooldownTimerMs <= 0.0f) {
                if (glm::distance(enemy.worldPosition, weaponHitCenter) <= hitRadius) {
                    enemy.hitCooldownTimerMs = 300.0f; // 武器冷卻：同一次揮擊只受傷一次
                    enemy.health -= m_WeaponDamage;
                    if (enemy.health <= 0.0f) {
                        enemy.active = false;
                        enemy.object->SetVisible(false);

                        // 擊殺統計與波次更新
                        m_EnemiesDefeated++;
                        m_CurrentWave = (m_EnemiesDefeated / 15) + 1;
                        m_CurrentStage = (m_CurrentWave - 1) / 5 + 1; // 每 5 波 (75隻敵人) 推進一關
                        m_EnemySpawnIntervalMs = std::max(200.0f, 1200.0f - static_cast<float>(m_CurrentWave - 1) * 100.0f); // 生成速度隨波次加快

                        static std::mt19937 dropRng(std::random_device{}());
                        std::uniform_real_distribution<float> dropDist(0.0f, 100.0f);
                        float dropRoll = dropDist(dropRng);

                        // 掉落經驗值寶石 (依機率決定掉落哪一種)
                        for (auto &gem : m_ExpGems) {
                            if (!gem.active) {
                                gem.active = true;
                                gem.worldPosition = enemy.worldPosition;
                                gem.pickupCooldownTimerMs = 800.0f; // 0.8 秒掉落冷卻
                                
                                std::shared_ptr<Util::Image> selectedImage;
                                if (dropRoll < 5.0f) {
                                    // 5% 機率掉落 Experience_Gem3 (最高經驗值)
                                    gem.expValue = 10;
                                    selectedImage = m_Gem3Image;
                                } else if (dropRoll < 25.0f) {
                                    // 20% 機率掉落 Experience_Gem2 (中等經驗值)
                                    gem.expValue = 3;
                                    selectedImage = m_Gem2Image;
                                } else {
                                    // 75% 機率掉落 Experience_Gem1 (最低經驗值)
                                    gem.expValue = 1;
                                    selectedImage = m_Gem1Image;
                                }
                                
                                gem.object->SetDrawable(selectedImage);
                                
                                // 依據所選圖片與玩家比例重新設定大小
                                const float targetWidth = playerSize.x * m_ExpGemSizeRatioToPlayer;
                                const float imgScale = targetWidth / selectedImage->GetSize().x;
                                gem.object->m_Transform.scale = {imgScale, imgScale};
                                gem.object->SetVisible(true);
                                break;
                            }
                        }

                        // 3% 機率掉落補血道具 (check.png)
                        if (dropDist(dropRng) < 3.0f) {
                            for (auto &potion : m_HealthItems) {
                                if (!potion.active) {
                                    potion.active = true;
                                    // 稍微往旁邊偏移一點，避免跟寶石完全重疊
                                    potion.worldPosition = enemy.worldPosition + glm::vec2(20.0f, 20.0f);
                                    potion.pickupCooldownTimerMs = 800.0f;
                                    potion.object->SetVisible(true);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 處理寶石吸收系統
    const float pickupRadius = playerSize.x * 0.7f; // 將吸收範圍改為 0.7 倍，不讓範圍過大
    for (auto &gem : m_ExpGems) {
        if (gem.active) {
            // 計算寶石冷卻，讓玩家有時間看到寶石掉落
            if (gem.pickupCooldownTimerMs > 0.0f) {
                gem.pickupCooldownTimerMs -= deltaTimeMs;
            } else {
                float dist = glm::distance(m_PlayerWorldPosition, gem.worldPosition);
                if (dist <= pickupRadius) {
                    // 給玩家經驗值
                    m_PlayerExp += gem.expValue;
                    gem.active = false;
                    gem.object->SetVisible(false);

                    // 簡單的升級邏輯
                    if (m_PlayerExp >= m_PlayerExpNext) {
                        m_PlayerExp -= m_PlayerExpNext;
                        m_PlayerLevel++;
                        m_PlayerExpNext = static_cast<int>(m_PlayerExpNext * 1.5f); // 升級需求遞增
                        m_PlayerMaxHealth += 20.0f; // 升級回血並加血量上限
                        m_PlayerHealth = m_PlayerMaxHealth;
                        m_CurrentState = State::LEVEL_UP; // 切換到升級暫停介面
                    }
                }
            }
            // 更新寶石位置
            gem.object->m_Transform.translation = gem.worldPosition - m_CameraPosition;
        }
    }

    // 處理補血道具的掉落冷卻與拾取
    for (auto &potion : m_HealthItems) {
        if (potion.active) {
            if (potion.pickupCooldownTimerMs > 0.0f) {
                potion.pickupCooldownTimerMs -= deltaTimeMs;
            } else {
                float dist = glm::distance(m_PlayerWorldPosition, potion.worldPosition);
                if (dist <= pickupRadius) {
                    m_PlayerHealth = m_PlayerMaxHealth;
                    potion.active = false;
                    potion.object->SetVisible(false);
                }
            }
            potion.object->m_Transform.translation = potion.worldPosition - m_CameraPosition;
        }
    }

    const float tileWidth = m_BackgroundTileSize.x;
    const float tileHeight = m_BackgroundTileSize.y;
    const float halfWindowWidth = static_cast<float>(WINDOW_WIDTH) * 0.5f;
    const float halfWindowHeight = static_cast<float>(WINDOW_HEIGHT) * 0.5f;

    const int minTileX =
        static_cast<int>(std::floor((m_CameraPosition.x - halfWindowWidth) / tileWidth)) - 1;
    const int maxTileX =
        static_cast<int>(std::floor((m_CameraPosition.x + halfWindowWidth) / tileWidth)) + 1;
    const int minTileY =
        static_cast<int>(std::floor((m_CameraPosition.y - halfWindowHeight) / tileHeight)) - 1;
    const int maxTileY =
        static_cast<int>(std::floor((m_CameraPosition.y + halfWindowHeight) / tileHeight)) + 1;

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

            auto &tile = m_BackgroundTiles[static_cast<size_t>(tileIndex)];
            tile.object->m_Transform.translation =
                glm::vec2{worldPosX, worldPosY} - m_CameraPosition;

            ++tileIndex;
        }
    }

    DrawGameObjects();

    // =============== UI 繪製區 ===============
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(500, 100), ImGuiCond_Always); // 增加寬度以避免時間被裁切
    ImGui::Begin("Player Status", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                 ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove);

    // 玩家進度條
    // 計算血條在畫面中「玩家下方」的位置
    glm::vec2 screenPlayerPos = m_Player->m_Transform.translation; 
    screenPlayerPos.x += (WINDOW_WIDTH / 2.0f);
    screenPlayerPos.y += (WINDOW_HEIGHT / 2.0f); // 置中

    ImVec2 healthBarSize(playerSize.x * 1.0f, 10.0f); // 與角色等同寬度比例
    // 置換在角色下方
    ImVec2 healthBarPos(screenPlayerPos.x - healthBarSize.x / 2.0f,
                        WINDOW_HEIGHT - screenPlayerPos.y + playerSize.y / 2.0f + 10.0f);
    
    // 取得 ImGui 畫布
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    // 繪製背景 (灰色)
    drawList->AddRectFilled(healthBarPos,
                            ImVec2(healthBarPos.x + healthBarSize.x, healthBarPos.y + healthBarSize.y),
                            IM_COL32(80, 80, 80, 255));
    // 繪製前景 (血量比例，紅色)
    float healthRatio = std::clamp(m_PlayerHealth / m_PlayerMaxHealth, 0.0f, 1.0f);
    drawList->AddRectFilled(healthBarPos,
                            ImVec2(healthBarPos.x + (healthBarSize.x * healthRatio), healthBarPos.y + healthBarSize.y),
                            IM_COL32(230, 40, 40, 255));


    // 格式化遊戲時間 (時:分:秒)
    int timeHours = static_cast<int>(m_GameTimeMs / 3600000.0f);
    int timeMinutes = static_cast<int>(m_GameTimeMs / 60000.0f) % 60;
    int timeSeconds = static_cast<int>(m_GameTimeMs / 1000.0f) % 60;

    // 在畫面正上方畫經驗條與等級波次
    ImGui::SetCursorPos(ImVec2(10, 10));
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Lv %d", m_PlayerLevel);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), " | Stage %d", m_CurrentStage);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1, 1, 1, 1), " | Wave %d", m_CurrentWave);
    ImGui::SameLine();
    
    // 一律顯示 時:分:秒
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), " | Time %02d:%02d:%02d", timeHours, timeMinutes, timeSeconds);

    ImGui::ProgressBar((float)m_PlayerExp / m_PlayerExpNext, ImVec2(280, 20), "EXP");

    // 右上角暫停按鈕
    ImGui::SetCursorPos(ImVec2(WINDOW_WIDTH - 100, 10));
    if (Util::Input::IsKeyUp(Util::Keycode::P)) {
        m_CurrentState = State::PAUSED;
    }

    ImGui::End();

    // 獨立的暫停按鈕視窗 (避免被 NoInputs 阻擋或被 Size 裁切)
    ImGui::SetNextWindowPos(ImVec2(WINDOW_WIDTH - 90, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(80, 40), ImGuiCond_Always);
    ImGui::Begin("PauseUI", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    if (ImGui::Button("Pause", ImVec2(70, 30))) {
        m_CurrentState = State::PAUSED;
    }
    ImGui::End();
    // =========================================

    /*
     * Do not touch the code below as they serve the purpose for
     * closing the window.
     */
    if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) ||
        Util::Input::IfExit()) {
        m_CurrentState = State::END;
    }
}

void App::DrawGameObjects() {
    // 優化渲染效能：視錐體剔除 (Frustum Culling) - 不渲染畫面外物品
    const float cullDistX = WINDOW_WIDTH * 0.5f + 100.0f; // 給予 100 像素緩衝區
    const float cullDistY = WINDOW_HEIGHT * 0.5f + 100.0f;

    for (auto &tile : m_BackgroundTiles) {
        tile.object->Draw();
    }
    for (auto &gem : m_ExpGems) {
        if (gem.active) {
            if (std::abs(gem.worldPosition.x - m_CameraPosition.x) < cullDistX &&
                std::abs(gem.worldPosition.y - m_CameraPosition.y) < cullDistY) {
                gem.object->Draw();
            }
        }
    }
    for (auto &potion : m_HealthItems) {
        if (potion.active) {
            if (std::abs(potion.worldPosition.x - m_CameraPosition.x) < cullDistX &&
                std::abs(potion.worldPosition.y - m_CameraPosition.y) < cullDistY) {
                potion.object->Draw();
            }
        }
    }
    for (auto &enemy : m_Enemies) {
        if (enemy.active) {
            if (std::abs(enemy.worldPosition.x - m_CameraPosition.x) < cullDistX &&
                std::abs(enemy.worldPosition.y - m_CameraPosition.y) < cullDistY) {
                
                // 受擊閃爍特效改為「實體白化」: 只要還處於冷卻時間的前半段 (大於 150ms，因為總共是 300ms) 就顯示白圖，給出單次閃爍的感覺
                if (enemy.hitCooldownTimerMs > 150.0f) {
                    enemy.object->SetDrawable(enemy.hurtImage);
                } else {
                    enemy.object->SetDrawable(enemy.defaultImage);
                }
                enemy.object->Draw();
            }
        }
    }

    // 玩家受擊特效已在 UpdateStart() 透過 SetState() 切換圖片，這裡一律畫出來
    m_Player->Draw();
    
    m_WeaponEffect.object->Draw();
}

void App::UpdatePaused() {
    DrawGameObjects(); // 畫出底層但不會更新他們的邏輯，形成暫停效果

    ImGui::SetNextWindowPos(ImVec2(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Always);
    ImGui::Begin("Paused", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::Text("Game is Paused");
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 20));

    // 按下 Resume 按鈕或是再按一次 P 都可以繼續
    if (ImGui::Button("Resume", ImVec2(280, 50)) || Util::Input::IsKeyUp(Util::Keycode::P)) {
        m_CurrentState = State::UPDATE;
    }

    if (ImGui::Button("Exit", ImVec2(280, 50))) {
        m_CurrentState = State::END;
    }

    ImGui::End();

    if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) || Util::Input::IfExit()) {
        m_CurrentState = State::END;
    }
}

void App::UpdateLevelUp() {
    DrawGameObjects();

    ImGui::SetNextWindowPos(ImVec2(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Always);
    ImGui::Begin("Level Up!", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Congratulations! You reached Level %d", m_PlayerLevel);
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 10));

    if (ImGui::Button("Decrease Weapon Cooldown (-10%)", ImVec2(380, 50))) {
        m_WeaponAttackIntervalMs *= 0.9f;
        m_CurrentState = State::UPDATE;
    }
    
    ImGui::Dummy(ImVec2(0, 5));
    if (ImGui::Button("Increase Weapon Damage (+10)", ImVec2(380, 50))) {
        m_WeaponDamage += 10.0f;
        m_CurrentState = State::UPDATE;
    }

    ImGui::Dummy(ImVec2(0, 5));
    if (ImGui::Button("Increase Weapon Range (+10%)", ImVec2(380, 50))) {
        m_WeaponHitRadiusRatioToPlayer *= 1.1f;
        const glm::vec2 weaponNativeSize = m_WeaponEffect.activeAnimation->GetSize();
        const float targetWeaponWidth = m_Player->GetScaledSize().x * m_WeaponWidthRatioToPlayer * (m_WeaponHitRadiusRatioToPlayer / 1.0f); // 配合打擊範圍放大特效
        const float weaponScale = targetWeaponWidth / weaponNativeSize.x;
        m_WeaponEffect.object->m_Transform.scale = {weaponScale, weaponScale};
        m_CurrentState = State::UPDATE;
    }

    ImGui::End();

    if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) || Util::Input::IfExit()) {
        m_CurrentState = State::END;
    }
}

void App::UpdateGameOver() {
    DrawGameObjects();

    ImGui::SetNextWindowPos(ImVec2(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Always);
    ImGui::Begin("Game Over", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "YOU DIED");
    ImGui::Text("Level Reached:     %d", m_PlayerLevel);
    ImGui::Text("Stage Reached:     %d", m_CurrentStage);
    ImGui::Text("Enemies Defeated:  %d", m_EnemiesDefeated);
    ImGui::Text("Waves Survived:    %d", m_CurrentWave);

    int timeHours = static_cast<int>(m_GameTimeMs / 3600000.0f);
    int timeMinutes = static_cast<int>(m_GameTimeMs / 60000.0f) % 60;
    int timeSeconds = static_cast<int>(m_GameTimeMs / 1000.0f) % 60;
    
    // 一律顯示 時:分:秒
    ImGui::Text("Time Survived:     %02d:%02d:%02d", timeHours, timeMinutes, timeSeconds);
    
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 20));

    if (ImGui::Button("Restart", ImVec2(280, 50))) {
        m_CurrentState = State::START;
    }

    if (ImGui::Button("Exit", ImVec2(280, 50))) {
        m_CurrentState = State::END;
    }

    ImGui::End();

    if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) || Util::Input::IfExit()) {
        m_CurrentState = State::END;
    }
}

void App::End() { // NOLINT(this method will mutate members in the future)
    LOG_TRACE("End");
}
