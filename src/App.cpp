#include "App.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

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
        m_TitleScreen->m_Transform.scale = {static_cast<float>(WINDOW_WIDTH) / imageSize.x,
                                            static_cast<float>(WINDOW_HEIGHT) / imageSize.y};
    }

    // 讓標題畫面跟著攝影機放在正中央
    m_TitleScreen->m_Transform.translation = glm::vec2(0.0f, 0.0f);
    m_TitleScreen->Draw();

    // 在畫面上用 ImGui 顯示閃爍的 "PRESS TO START" 或任意鍵提示
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT), ImGuiCond_Always);
    ImGui::Begin("TitleScreenUI", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs |
                     ImGuiWindowFlags_NoMove);

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
    m_KillsToNextWave = 10;
    m_MagnetTimerMs = 0.0f;
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
        weaponRightFrames.push_back(std::string(RESOURCE_DIR) + "/right_slash" + std::to_string(i) + ".png");
        weaponLeftFrames.push_back(std::string(RESOURCE_DIR) + "/left_slash" + std::to_string(i) + ".png");
    }

    m_WeaponEffect.rightAnimation = std::make_shared<Util::Animation>(weaponRightFrames, false, 50, false, 0);
    m_WeaponEffect.leftAnimation = std::make_shared<Util::Animation>(weaponLeftFrames, false, 50, false, 0);
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

    // 預先載入所有敵人圖片
    m_Enemy1Image = std::make_shared<Util::Image>(m_EnemyPath);
    m_Enemy1HurtImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/hurt_enemy1.png");
    m_Enemy2Image = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/enemy2.png");
    m_Enemy2HurtImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/hurt_enemy2.png");
    m_Enemy3Image = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/enemy3.png");
    m_Enemy3HurtImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/hurt_enemy3.png");
    m_Enemy4Image = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/enemy4.png");
    m_Enemy4HurtImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/hurt_enemy4.png");
    m_GameOverImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/game_over.png");

    std::vector<std::string> bossLeftFrames = {
        std::string(RESOURCE_DIR) + "/boss_left1.png", std::string(RESOURCE_DIR) + "/boss_left2.png",
        std::string(RESOURCE_DIR) + "/boss_left3.png", std::string(RESOURCE_DIR) + "/boss_left4.png"};
    std::vector<std::string> bossRightFrames = {
        std::string(RESOURCE_DIR) + "/boss_right1.png", std::string(RESOURCE_DIR) + "/boss_right2.png",
        std::string(RESOURCE_DIR) + "/boss_right3.png", std::string(RESOURCE_DIR) + "/boss_right4.png"};
    m_BossLeftAnimation = std::make_shared<Util::Animation>(bossLeftFrames, true, 100, true, 0);
    m_BossRightAnimation = std::make_shared<Util::Animation>(bossRightFrames, true, 100, true, 0);

    m_BossHurtLeftImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/hurt_boss_left1.png");
    m_BossHurtRightImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/hurt_boss_right1.png");

    for (int i = 0; i < m_MaxEnemies; ++i) {
        EnemyUnit enemy;
        enemy.object = std::make_shared<Util::GameObject>();
        // Default to enemy 1 initially, will be re-assigned on spawn
        enemy.defaultImage = m_Enemy1Image;
        enemy.hurtImage = m_Enemy1HurtImage;

        enemy.object->SetDrawable(enemy.defaultImage);
        enemy.object->SetZIndex(4.0f);
        enemy.object->SetVisible(false);
        const float targetEnemyWidth = playerSize.x * m_EnemyWidthRatioToPlayer;
        const float enemyScale = targetEnemyWidth / m_Enemy1Image->GetSize().x;
        enemy.object->m_Transform.scale = {enemyScale, enemyScale};
        enemy.active = false;
        enemy.health = enemy.maxHealth;
        enemy.speed = 100.0f;  // pixels per second
        enemy.damage = 10.0f;
        enemy.hitCooldownTimerMs = 0.0f;
        m_Enemies.push_back(enemy);
    }

    m_EnemySpawnTimerMs = 0.0f;

    // 載入寶石與道具共用圖片
    m_Gem1Image = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/Experience_Gem1.png");
    m_Gem2Image = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/Experience_Gem2.png");
    m_Gem3Image = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/Experience_Gem3.png");
    m_HealthImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/check.png");

    // 載入升級畫面背景並調整大小以符合螢幕
    m_LevelUpImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/levelup.png");
    m_LevelUpObject = std::make_shared<Util::GameObject>();
    m_LevelUpObject->SetDrawable(m_LevelUpImage);
    m_LevelUpObject->SetZIndex(100.0f);  // 蓋在最上層
    m_LevelUpObject->SetVisible(true);   // UpdateLevelUp() 時自行 draw()

    // 原圖片高度是 854，畫面高是 720，稍微縮小 0.8 倍以吻合高度，並加點餘離
    const float levelUpScale = static_cast<float>(WINDOW_HEIGHT) / m_LevelUpImage->GetSize().y * 0.9f;
    m_LevelUpObject->m_Transform.scale = {levelUpScale, levelUpScale};

    m_PauseIconImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/pause_icon.png");
    m_EnemyCountIconImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/enemy_count_icon.png");

    // 嘗試載入背景音樂 (可以隨意替換檔名)
    // 請將你要播放的音樂放到 Resources 資料夾，並叫做 bgm.mp3
    try {
        m_BGM = std::make_shared<Util::BGM>(std::string(RESOURCE_DIR) + "/bgm.mp3");
        m_BGM->SetVolume(50);  // 音量 0~128
        m_BGM->Play(-1);       // -1 表示無限循環
    } catch (...) {
        // 如果沒有音樂檔案則略過不播放，避免程式崩潰
    }

    // Initialize EXP Gem Object Pool
    m_ExpGems.clear();
    m_ExpGems.reserve(m_MaxExpGems);
    for (int i = 0; i < m_MaxExpGems; ++i) {
        ExpGem gem;
        gem.object = std::make_shared<Util::GameObject>();
        gem.object->SetDrawable(m_Gem1Image);
        gem.object->SetZIndex(3.0f);  // 繪製在敵人下面，背景上面
        gem.object->SetVisible(false);
        gem.active = false;
        gem.expValue = 1;
        gem.pickupCooldownTimerMs = 150.0f;
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
        potion.pickupCooldownTimerMs = 150.0f;
        m_HealthItems.push_back(potion);
    }

    // Initialize Magnet Item Object Pool
    m_MagnetImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/magnet.png");
    m_MagnetItems.clear();
    m_MagnetItems.reserve(m_MaxMagnetItems);
    for (int i = 0; i < m_MaxMagnetItems; ++i) {
        MagnetItem magnet;
        magnet.object = std::make_shared<Util::GameObject>();
        magnet.object->SetDrawable(m_MagnetImage);
        magnet.object->SetZIndex(3.0f);
        magnet.object->SetVisible(false);

        const float targetWidth = playerSize.x * m_ExpGemSizeRatioToPlayer;
        const float itemScale = targetWidth / m_MagnetImage->GetSize().x;
        magnet.object->m_Transform.scale = {itemScale, itemScale};
        magnet.active = false;
        magnet.pickupCooldownTimerMs = 150.0f;
        m_MagnetItems.push_back(magnet);
    }

    // Initialize Boss Reward Object Pool
    m_RewardImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/reward.png");
    m_RewardItems.clear();
    m_RewardItems.reserve(m_MaxRewardItems);
    for (int i = 0; i < m_MaxRewardItems; ++i) {
        RewardItem reward;
        reward.object = std::make_shared<Util::GameObject>();
        reward.object->SetDrawable(m_RewardImage);
        reward.object->SetZIndex(3.0f);
        reward.object->SetVisible(false);

        const float targetWidth = playerSize.x * m_ExpGemSizeRatioToPlayer * 1.5f;  // 稍微大一點
        const float itemScale = targetWidth / m_RewardImage->GetSize().x;
        reward.object->m_Transform.scale = {itemScale, itemScale};
        reward.active = false;
        reward.pickupCooldownTimerMs = 150.0f;
        m_RewardItems.push_back(reward);
    }

    // 載入飛刀圖片並初始化飛刀 Object Pool
    m_KnifeImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/knife.png");
    m_Knives.clear();
    m_Knives.reserve(m_MaxKnives);
    for (int i = 0; i < m_MaxKnives; ++i) {
        Knife knife;
        knife.object = std::make_shared<Util::GameObject>();
        knife.object->SetDrawable(m_KnifeImage);
        knife.object->SetZIndex(5.0f);  // 飛刀畫在上面一點
        knife.object->SetVisible(false);

        // 調整飛刀大小 (可依需求調整比例，這裡先設為玩家寬度的 0.65 倍)
        const float targetKnifeWidth = playerSize.x * 0.65f;
        const float knifeScale = targetKnifeWidth / m_KnifeImage->GetSize().x;
        knife.object->m_Transform.scale = {knifeScale, knifeScale};

        knife.active = false;
        m_Knives.push_back(knife);
    }

    // 載入符文追蹤者圖片並初始化 Object Pool
    m_RunetracerImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/Runetracer.png");
    m_RunetracerImage95 = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/Runetracer95.png");
    m_RunetracerImage80 = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/Runetracer80.png");
    m_RunetracerImage65 = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/Runetracer65.png");
    m_RunetracerImage50 = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/Runetracer50.png");
    m_Runetracers.clear();
    m_Runetracers.reserve(m_MaxRunetracers);
    for (int i = 0; i < m_MaxRunetracers; ++i) {
        Runetracer rune;
        rune.object = std::make_shared<Util::GameObject>();
        rune.object->SetDrawable(m_RunetracerImage);
        rune.object->SetZIndex(4.5f);  // 畫在怪物之上、UI之下
        rune.object->SetVisible(false);

        const float targetRuneWidth = playerSize.x * 0.5f;
        const float runeScale = targetRuneWidth / m_RunetracerImage->GetSize().x;
        rune.object->m_Transform.scale = {runeScale, runeScale};

        rune.active = false;
        rune.maxHistory = 40;  // 保留40個殘影，讓尾流更長
        m_Runetracers.push_back(rune);
    }

    // Reset player states
    m_WhipRangeLevel = 1;
    m_WhipDamageLevel = 1;
    m_WhipCooldownLevel = 1;
    m_KnifeCountLevel = 1;
    m_KnifeDamageLevel = 1;
    m_KnifeCooldownLevel = 1;
    m_RunetracerCountLevel = 1;
    m_RunetracerDamageLevel = 1;
    m_RunetracerCooldownLevel = 1;
    m_MaxHealthLevel = 1;
    m_ArmorLevel = 1;
    m_VampirismLevel = 1;
    m_KnifeUnlocked = false;
    m_RunetracerUnlocked = false;
    m_ArmorUnlocked = false;
    m_VampirismUnlocked = false;
    m_PlayerArmor = 0.0f;
    m_PlayerVampirism = 0.0f;
    m_WeaponAttackIntervalMs = 600.0f;       // 鞭子最快
    m_KnifeAttackIntervalMs = 800.0f;        // 小刀居中
    m_RunetracerAttackIntervalMs = 1500.0f;  // 符文追蹤者最慢
    m_WeaponHitRadiusRatioToPlayer = 1.0f;
    m_WeaponDamage = 100.0f;  // 鞭子最痛
    m_KnifeDamage = 80.0f;    // 小刀居中
    m_KnifeCount = 1;
    m_RunetracerDamage = 35.0f;
    m_RunetracerCount = 1;
    m_PlayerMaxHealth = 100.0f;
    m_PlayerHealth = m_PlayerMaxHealth;
    m_PlayerLevel = 1;
    m_PlayerExp = 0;
    m_PlayerExpNext = 10;
    m_PlayerHitCooldownTimerMs = 0.0f;
    m_CurrentWave = 1;
    m_CurrentStage = 1;
    m_EnemiesDefeated = 0;
    m_GameTimeMs = 0.0f;
    m_LastSpecialWaveTriggered = 0;

    m_CurrentState = State::UPDATE;
}

void App::Update() {
    if (m_BossLeftAnimation) m_BossLeftAnimation->Play();
    if (m_BossRightAnimation) m_BossRightAnimation->Play();
    if (m_Player) m_Player->PlayAnimation();
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

    bool isMoving = moveDir.x != 0.0f || moveDir.y != 0.0f;
    if (isMoving) {
        constexpr float kMoveSpeed = 0.35f;  // pixels per millisecond
        moveDir = glm::normalize(moveDir);
        m_PlayerLastMoveDir = moveDir;  // 更新飛行武器的方向
        m_PlayerWorldPosition += moveDir * kMoveSpeed * Util::Time::GetDeltaTimeMs();
    }

    if (moveDir.x < 0.0f) {
        m_IsFacingLeft = true;
    } else if (moveDir.x > 0.0f) {
        m_IsFacingLeft = false;
    }
    // 玩家總無敵時間是 500ms，只要在 > 300ms 期間內顯示一次受擊白圖 (約亮白 200ms) 即可，避免像燈泡一樣閃爍
    m_Player->SetState(m_IsFacingLeft, isMoving, m_PlayerHitCooldownTimerMs > 300.0f);

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

    // --- 近戰武器邏輯 ---
    m_WeaponAttackTimerMs += Util::Time::GetDeltaTimeMs();
    if (m_WeaponAttackTimerMs >= m_WeaponAttackIntervalMs) {
        m_WeaponAttackTimerMs = 0.0f;
        m_WeaponEffect.activeAnimation = m_IsFacingLeft ? m_WeaponEffect.leftAnimation : m_WeaponEffect.rightAnimation;
        m_WeaponEffect.object->SetDrawable(m_WeaponEffect.activeAnimation);
        m_WeaponEffect.activeAnimation->SetCurrentFrame(0);
        m_WeaponEffect.activeAnimation->Play();
        m_WeaponEffect.object->SetVisible(true);
    }

    if (m_WeaponEffect.activeAnimation->GetState() == Util::Animation::State::ENDED) {
        m_WeaponEffect.object->SetVisible(false);
    }

    // --- 飛刀武器邏輯 ---
    if (m_KnifeUnlocked) {
        m_KnifeAttackTimerMs += Util::Time::GetDeltaTimeMs();
        if (m_KnifeAttackTimerMs >= m_KnifeAttackIntervalMs) {
            m_KnifeAttackTimerMs = 0.0f;

            int spawned = 0;
            float baseAngle = atan2(m_PlayerLastMoveDir.y, m_PlayerLastMoveDir.x);
            float angleStep = 0.15f;  // 每把飛刀間隔 0.15 弧度
            float startAngle = baseAngle - (m_KnifeCount - 1) * angleStep * 0.5f;

            for (auto &knife : m_Knives) {
                if (!knife.active) {
                    knife.active = true;
                    knife.worldPosition = m_PlayerWorldPosition;

                    knife.angle = startAngle + spawned * angleStep;
                    knife.velocity = glm::vec2(std::cos(knife.angle), std::sin(knife.angle)) * m_KnifeSpeed;
                    knife.timeToLiveMs = 2000.0f;

                    knife.object->m_Transform.rotation = knife.angle;
                    knife.object->SetVisible(true);

                    spawned++;
                    if (spawned >= m_KnifeCount) break;
                }
            }
        }
    }

    // 更新飛刀位置與存活時間
    for (auto &knife : m_Knives) {
        if (knife.active) {
            knife.worldPosition += knife.velocity * static_cast<float>(Util::Time::GetDeltaTimeMs() / 1000.0f);
            knife.object->m_Transform.translation = knife.worldPosition - m_CameraPosition;

            knife.timeToLiveMs -= Util::Time::GetDeltaTimeMs();
            if (knife.timeToLiveMs <= 0.0f) {
                knife.active = false;
                knife.object->SetVisible(false);
            }
        }
    }

    // --- 符文追蹤者武器邏輯 ---
    if (m_RunetracerUnlocked) {
        m_RunetracerAttackTimerMs += Util::Time::GetDeltaTimeMs();
        if (m_RunetracerAttackTimerMs >= m_RunetracerAttackIntervalMs) {
            m_RunetracerAttackTimerMs = 0.0f;

            int spawned = 0;
            static std::mt19937 runeRng(std::random_device{}());
            std::uniform_real_distribution<float> angleDist(0.0f, 6.2831853f);  // 0 到 2*PI

            for (auto &rune : m_Runetracers) {
                if (!rune.active) {
                    rune.active = true;
                    rune.worldPosition = m_PlayerWorldPosition;

                    float randomAngle = angleDist(runeRng);

                    rune.velocity = glm::vec2(std::cos(randomAngle), std::sin(randomAngle)) * m_RunetracerSpeed;
                    rune.timeToLiveMs = 4000.0f;
                    rune.angle = randomAngle;
                    rune.historyPositions.clear();

                    rune.object->m_Transform.rotation = rune.angle;
                    rune.object->SetVisible(true);

                    spawned++;
                    if (spawned >= m_RunetracerCount) break;
                }
            }
        }
    }

    // 更新符文追蹤者位置與存活時間、動態邊緣反彈
    for (auto &rune : m_Runetracers) {
        if (rune.active) {
            // 紀錄歷史軌跡
            rune.historyPositions.insert(rune.historyPositions.begin(), rune.worldPosition);
            if (rune.historyPositions.size() > rune.maxHistory) {
                rune.historyPositions.pop_back();
            }

            float dt = static_cast<float>(Util::Time::GetDeltaTimeMs() / 1000.0f);
            rune.worldPosition += rune.velocity * dt;

            // 處理與螢幕邊界的反彈 (螢幕範圍是由 CameraPosition 決定)
            float leftBound = m_CameraPosition.x - WINDOW_WIDTH * 0.5f;
            float rightBound = m_CameraPosition.x + WINDOW_WIDTH * 0.5f;
            float bottomBound = m_CameraPosition.y - WINDOW_HEIGHT * 0.5f;
            float topBound = m_CameraPosition.y + WINDOW_HEIGHT * 0.5f;

            if (rune.worldPosition.x < leftBound) {
                rune.worldPosition.x = leftBound;
                rune.velocity.x *= -1.0f;
            } else if (rune.worldPosition.x > rightBound) {
                rune.worldPosition.x = rightBound;
                rune.velocity.x *= -1.0f;
            }

            if (rune.worldPosition.y < bottomBound) {
                rune.worldPosition.y = bottomBound;
                rune.velocity.y *= -1.0f;
            } else if (rune.worldPosition.y > topBound) {
                rune.worldPosition.y = topBound;
                rune.velocity.y *= -1.0f;
            }

            // 更新實際圖片角度使其配合新的速度方向
            rune.angle = atan2(rune.velocity.y, rune.velocity.x);
            rune.object->m_Transform.rotation = rune.angle;

            rune.object->m_Transform.translation = rune.worldPosition - m_CameraPosition;

            rune.timeToLiveMs -= Util::Time::GetDeltaTimeMs();
            if (rune.timeToLiveMs <= 0.0f) {
                rune.active = false;
                rune.object->SetVisible(false);
            }
        }
    }

    // Determine simple offset right next to the player
    const glm::vec2 playerSize = m_Player->GetScaledSize();
    const float sideOffset = playerSize.x * 0.7f;  // simple side offset
    const glm::vec2 weaponOffset = m_IsFacingLeft ? glm::vec2{-sideOffset, 0.0f} : glm::vec2{sideOffset, 0.0f};

    m_WeaponEffect.object->m_Transform.translation = (m_PlayerWorldPosition + weaponOffset) - m_CameraPosition;

    const float deltaTimeMs = Util::Time::GetDeltaTimeMs();
    m_GameTimeMs += deltaTimeMs;  // 推進遊戲總時間
    m_EnemySpawnTimerMs += deltaTimeMs;

    // Object Pool: Build a list of active enemies to drastically reduce loop iterations later
    std::vector<EnemyUnit *> activeEnemies;
    activeEnemies.reserve(m_Enemies.size());
    for (auto &enemy : m_Enemies) {
        if (enemy.active) {
            activeEnemies.push_back(&enemy);
        }
    }
    int activeEnemyCount = static_cast<int>(activeEnemies.size());

    // 【空間分割 (Spatial HashingGrid)】: 用於快速查找附近敵人
    const float CELL_SIZE = 150.0f;  // 格子大小至少要大於最大怪物的直徑
    std::unordered_map<int64_t, std::vector<EnemyUnit *>> grid;

    auto getGridKey = [CELL_SIZE](const glm::vec2 &pos) -> int64_t {
        int cx = static_cast<int>(std::floor(pos.x / CELL_SIZE));
        int cy = static_cast<int>(std::floor(pos.y / CELL_SIZE));
        return (static_cast<int64_t>(cx) << 32) | (cy & 0xFFFFFFFF);
    };

    // 將所有活著的怪放入 Grid 中
    for (auto *enemyPtr : activeEnemies) {
        grid[getGridKey(enemyPtr->worldPosition)].push_back(enemyPtr);
    }

    if (m_EnemySpawnTimerMs >= m_EnemySpawnIntervalMs && activeEnemyCount < m_MaxEnemies) {
        m_EnemySpawnTimerMs = 0.0f;

        static std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> angleDist(0.0f, glm::two_pi<float>());
        std::uniform_real_distribution<float> distanceDist(m_EnemySpawnMinDistance, m_EnemySpawnMaxDistance);

        // 特殊波次生成 (每 5 波觸發一次)
        if (m_CurrentWave > 0 && m_CurrentWave % 5 == 0 && m_LastSpecialWaveTriggered != m_CurrentWave) {
            m_LastSpecialWaveTriggered = m_CurrentWave;

            // 生成 Boss
            bool bossSpawned = false;
            for (auto &enemy : m_Enemies) {
                if (!enemy.active) {
                    enemy.active = true;
                    enemy.isBoss = true;

                    float angle = angleDist(rng);
                    float bossDist = m_EnemySpawnMaxDistance * 0.8f;
                    enemy.worldPosition =
                        m_PlayerWorldPosition + glm::vec2{std::cos(angle), std::sin(angle)} * bossDist;

                    float baselineHP = 100.0f + static_cast<float>(m_CurrentWave - 1) * 200.0f;
                    float baselineDmg = 10.0f + static_cast<float>(m_CurrentWave - 1) * 5.0f;

                    float bossHealthMult = 8.0f;
                    float bossDmgMult = 2.0f;

                    enemy.defaultAnimation = m_BossLeftAnimation;
                    enemy.hurtImage = m_BossHurtLeftImage;
                    enemy.speed = 65.0f;                            // 再調慢
                    enemy.damage = baselineDmg * bossDmgMult;       // 王的攻擊力
                    enemy.maxHealth = baselineHP * bossHealthMult;  // 王的血量
                    enemy.health = enemy.maxHealth;
                    enemy.hitCooldownTimerMs = 0.0f;

                    enemy.object->SetDrawable(enemy.defaultAnimation);
                    enemy.object->SetZIndex(4.2f);

                    const float targetEnemyWidth = m_Player->GetScaledSize().x * 3.0f;  // Boss很大
                    const float enemyScale = targetEnemyWidth / m_BossLeftAnimation->GetSize().x;
                    enemy.object->m_Transform.scale = {enemyScale, enemyScale};
                    enemy.object->m_Transform.translation = enemy.worldPosition - m_CameraPosition;
                    enemy.object->SetVisible(true);

                    bossSpawned = true;
                    break;
                }
            }

            // 畫一個圈圈生成 enemy4
            if (bossSpawned) {
                int specialEnemyCount = 60;                           // 將數量提升至 60 隻，讓圈更密集
                float circleRadius = m_EnemySpawnMaxDistance * 1.5f;  // 生成在比王更遠的圓圈上
                for (int i = 0; i < specialEnemyCount; ++i) {
                    // Find available enemy
                    for (auto &enemy : m_Enemies) {
                        if (!enemy.active) {
                            enemy.active = true;
                            enemy.isBoss = false;
                            enemy.defaultAnimation = nullptr;

                            float angle = (glm::two_pi<float>() / specialEnemyCount) * i;
                            enemy.worldPosition =
                                m_PlayerWorldPosition + glm::vec2{std::cos(angle), std::sin(angle)} * circleRadius;

                            float baselineHP = 100.0f + static_cast<float>(m_CurrentWave - 1) * 60.0f;

                            enemy.defaultImage = m_Enemy4Image;
                            enemy.hurtImage = m_Enemy4HurtImage;
                            enemy.speed = 45.0f;
                            enemy.damage = 0.0f;  // 百分比傷害會在碰撞處特別處理
                            enemy.maxHealth = baselineHP * 3.0f;
                            enemy.health = enemy.maxHealth;
                            enemy.hitCooldownTimerMs = 0.0f;

                            enemy.object->SetDrawable(enemy.defaultImage);
                            enemy.object->SetZIndex(4.1f);

                            const float targetEnemyWidth = m_Player->GetScaledSize().x * m_EnemyWidthRatioToPlayer;
                            const float enemyScale = targetEnemyWidth / enemy.defaultImage->GetSize().x;
                            enemy.object->m_Transform.scale = {enemyScale, enemyScale};
                            enemy.object->m_Transform.translation = enemy.worldPosition - m_CameraPosition;
                            enemy.object->SetVisible(true);
                            break;
                        }
                    }
                }
            }
        }

        // 設定該敵人的基礎數值 (後期強化)
        float baselineHP = 60.0f + static_cast<float>(m_CurrentWave - 1) * 70.0f;
        float baselineDmg = 6.0f + static_cast<float>(m_CurrentWave - 1) * 3.0f;

        const float angle = angleDist(rng);
        const float distance = distanceDist(rng);
        const glm::vec2 spawnOffset = glm::vec2{std::cos(angle), std::sin(angle)} * distance;

        // Find available enemy in pool
        for (auto &enemy : m_Enemies) {
            if (!enemy.active) {
                enemy.active = true;
                enemy.isBoss = false;
                enemy.defaultAnimation = nullptr;
                enemy.worldPosition = m_PlayerWorldPosition + spawnOffset;

                // 決定要生哪一種敵人
                std::uniform_real_distribution<float> typeDist(0.0f, 100.0f);
                float typeRoll = typeDist(rng);

                if (typeRoll < 20.0f) {
                    // Tanky
                    enemy.defaultImage = m_Enemy3Image;
                    enemy.hurtImage = m_Enemy3HurtImage;
                    enemy.speed = 55.0f;
                    enemy.damage = baselineDmg * 2.0f;
                    enemy.maxHealth = baselineHP * 2.0f;
                } else if (typeRoll < 50.0f) {
                    // Balanced
                    enemy.defaultImage = m_Enemy2Image;
                    enemy.hurtImage = m_Enemy2HurtImage;
                    enemy.speed = 95.0f;
                    enemy.damage = baselineDmg;
                    enemy.maxHealth = baselineHP;
                } else {
                    // Fast
                    enemy.defaultImage = m_Enemy1Image;
                    enemy.hurtImage = m_Enemy1HurtImage;
                    enemy.speed = 170.0f;
                    enemy.damage = baselineDmg * 0.5f;
                    enemy.maxHealth = baselineHP * 0.5f;
                }

                enemy.health = enemy.maxHealth;
                enemy.hitCooldownTimerMs = 0.0f;

                enemy.object->SetDrawable(enemy.defaultImage);
                // 重新設定縮放比例，以防不同圖大小不同
                // 依據玩家要求，全體再調大一點點，把原先 1.0f 的基準改為 1.2f
                const float targetEnemyWidth = m_Player->GetScaledSize().x * m_EnemyWidthRatioToPlayer * 1.2f;
                const float enemyScale = targetEnemyWidth / enemy.defaultImage->GetSize().x;
                float finalScale = enemyScale;
                if (typeRoll < 20.0f)
                    finalScale *= 1.5f;  // enemy3: larger
                else if (typeRoll < 50.0f)
                    finalScale *= 1.0f;  // enemy2: standard
                else
                    finalScale *= 0.7f;  // enemy1: smaller

                enemy.object->m_Transform.scale = {finalScale, finalScale};
                enemy.object->m_Transform.translation = enemy.worldPosition - m_CameraPosition;
                enemy.object->SetVisible(true);
                break;
            }
        }
    }

    // 更新玩家無敵時間
    if (m_PlayerHitCooldownTimerMs > 0.0f) {
        m_PlayerHitCooldownTimerMs -= deltaTimeMs;
    }

    for (auto *enemyPtr : activeEnemies) {
        auto &enemy = *enemyPtr;
        if (!enemy.active) {
            continue;
        }

        // 減少敵人無敵時間
        if (enemy.hitCooldownTimerMs > 0.0f) {
            enemy.hitCooldownTimerMs -= deltaTimeMs;
        }

        const glm::vec2 toPlayer = m_PlayerWorldPosition - enemy.worldPosition;
        const float distanceToPlayer = glm::length(toPlayer);

        // 給予不同體型對應的碰撞半徑 (用來判定傷害與碰撞)
        float enemyCollisionRadius = enemy.object->GetScaledSize().x * 0.35f;
        float playerCollisionRadius = playerSize.x * 0.35f;

        // 將受傷判定半徑與實體碰撞半徑一致，防止被推開而無法受傷
        float damageHitRadius = playerCollisionRadius + enemyCollisionRadius;

        // 如果距離太近，玩家扣血
        if (distanceToPlayer < damageHitRadius && m_PlayerHitCooldownTimerMs <= 0.0f) {
            float actualDamage = enemy.damage;
            if (enemy.defaultImage == m_Enemy4Image) {
                float percent = 0.10f + static_cast<float>(m_CurrentWave - 1) * 0.02f;  // 初始 10%，每波增加 2%
                actualDamage = m_PlayerMaxHealth * percent;
            }
            // 護甲減免
            actualDamage *= (1.0f - m_PlayerArmor);
            m_PlayerHealth -= actualDamage;
            m_PlayerHitCooldownTimerMs = 500.0f;  // 玩家 0.5秒無敵時間
            if (m_PlayerHealth <= 0.0f) {
                m_PlayerHealth = 0.0f;
                m_CurrentState = State::GAME_OVER;  // 玩家死亡，切換狀態
                return;
            }
        }

        if (distanceToPlayer > 0.1f) {
            const glm::vec2 direction = toPlayer / distanceToPlayer;

            // 加入敵人之間彼此推擠(排斥)的碰撞體積 (簡單的分離力)
            glm::vec2 separation(0.0f);

            // 找出自身所在的格子與周圍 8 宮格
            int cx = static_cast<int>(std::floor(enemy.worldPosition.x / CELL_SIZE));
            int cy = static_cast<int>(std::floor(enemy.worldPosition.y / CELL_SIZE));

            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    int64_t key = (static_cast<int64_t>(cx + dx) << 32) | ((cy + dy) & 0xFFFFFFFF);
                    auto it = grid.find(key);
                    if (it != grid.end()) {
                        for (const auto *otherEnemyPtr : it->second) {
                            const auto &otherEnemy = *otherEnemyPtr;
                            if (!otherEnemy.active || &enemy == &otherEnemy) continue;

                            glm::vec2 toOther = enemy.worldPosition - otherEnemy.worldPosition;

                            float otherRadius = otherEnemy.object->GetScaledSize().x * 0.35f;
                            float minSeparation = enemyCollisionRadius + otherRadius;

                            // 【AABB 快速過濾 (Manhattan Distance)】省去大量不必要的 length 計算
                            if (std::abs(toOther.x) > minSeparation || std::abs(toOther.y) > minSeparation) {
                                continue;
                            }

                            float dist = glm::length(toOther);

                            // 如果距離太近，產生與之反向的分離推移量
                            if (dist > 0.0f && dist < minSeparation) {
                                // 推擠量與重疊程度成正比
                                separation += (toOther / dist) * (minSeparation - dist) * 0.4f;
                            }
                        }
                    }
                }
            }

            // 加入玩家與敵人之間相互推擠的碰撞體積，避免穿過敵人群
            float playerThickSeparation = playerCollisionRadius + enemyCollisionRadius;
            // 若玩家和敵人間的距離過近則互相擠壓
            if (distanceToPlayer > 0.0f && distanceToPlayer < playerThickSeparation) {
                // 給敵人的推擠力量，方向是遠離玩家
                float overlap = playerThickSeparation - distanceToPlayer;
                separation -= (toPlayer / distanceToPlayer) * overlap * 0.6f;
                // 同時也給玩家反向位移，這造成「卡牆」與「推擠」視覺感
                m_PlayerWorldPosition -= (toPlayer / distanceToPlayer) * overlap * 0.1f;
            }

            // 限制單幀最大推擠位移，避免因疊加力量導致瞬移出界
            if (glm::length(separation) > 10.0f) {
                separation = glm::normalize(separation) * 10.0f;
            }

            // 若為 Boss，根據移動方向切換左右圖片
            if (enemy.isBoss) {
                if (direction.x < 0.0f) {
                    enemy.defaultAnimation = m_BossLeftAnimation;
                    enemy.hurtImage = m_BossHurtLeftImage;
                } else {
                    enemy.defaultAnimation = m_BossRightAnimation;
                    enemy.hurtImage = m_BossHurtRightImage;
                }

                // 如果沒有處於受傷狀態，立即更新為對應的 defaultImage
                if (enemy.hitCooldownTimerMs <= 0.0f) {
                    enemy.object->SetDrawable(enemy.defaultAnimation);
                }
            }

            // 更新敵人位置 (速度為 pixel per second, 所以乘上 deltaTimeMs/1000.0f)
            // 將 separation 作為直接的位置偏移 (Position Resolve)，而不是加速度
            enemy.worldPosition += (direction * (enemy.speed / 1000.0f)) * deltaTimeMs + separation;
        }

        enemy.object->m_Transform.translation = enemy.worldPosition - m_CameraPosition;
    }

    bool weaponCanHit = false;
    if (m_WeaponEffect.activeAnimation->GetState() == Util::Animation::State::PLAY) {
        const std::size_t currentFrame = m_WeaponEffect.activeAnimation->GetCurrentFrameIndex();
        weaponCanHit = currentFrame >= m_WeaponHitStartFrame && currentFrame <= m_WeaponHitEndFrame;
    }

    if (weaponCanHit) {
        const glm::vec2 weaponHitCenter = m_PlayerWorldPosition + weaponOffset;
        const float hitRadius = m_Player->GetScaledSize().x * m_WeaponHitRadiusRatioToPlayer;
        const float hitRadiusSq = hitRadius * hitRadius;

        for (auto *enemyPtr : activeEnemies) {
            auto &enemy = *enemyPtr;
            if (enemy.active) {
                float dx = enemy.worldPosition.x - weaponHitCenter.x;
                float dy = enemy.worldPosition.y - weaponHitCenter.y;
                float distSq = dx * dx + dy * dy;

                if (distSq <= hitRadius * hitRadius && enemy.hitCooldownTimerMs <= 0.0f) {
                    enemy.hitCooldownTimerMs = 300.0f;  // 武器冷卻：同一次揮擊只受傷一次
                    enemy.health -= m_WeaponDamage;

                    if (enemy.health <= 0.0f) {
                        m_PlayerHealth =
                            std::min(m_PlayerHealth + m_PlayerMaxHealth * m_PlayerVampirism, m_PlayerMaxHealth);
                        HandleEnemyDeath(enemy);
                    }
                }
            }
        }
    }

    // --- 飛刀碰撞邏輯 ---
    for (auto &knife : m_Knives) {
        if (!knife.active) continue;

        const float knifeHitRadius = m_Player->GetScaledSize().x * 0.4f * 0.5f;

        for (auto *enemyPtr : activeEnemies) {
            auto &enemy = *enemyPtr;
            if (enemy.active && enemy.hitCooldownTimerMs <= 0.0f) {
                float enemyCollisionRadius = enemy.object->GetScaledSize().x * 0.35f;
                float hitRadius = knifeHitRadius + enemyCollisionRadius;

                float dx = enemy.worldPosition.x - knife.worldPosition.x;
                float dy = enemy.worldPosition.y - knife.worldPosition.y;

                if ((dx * dx + dy * dy) <= hitRadius * hitRadius) {
                    enemy.hitCooldownTimerMs = 300.0f;
                    enemy.health -= m_KnifeDamage;
                    m_PlayerHealth = std::min(m_PlayerHealth + m_KnifeDamage * m_PlayerVampirism, m_PlayerMaxHealth);

                    knife.active = false;  // 飛刀碰撞後消失 (目前沒有穿透)
                    knife.object->SetVisible(false);

                    if (enemy.health <= 0.0f) {
                        HandleEnemyDeath(enemy);
                    }
                    break;  // 飛刀已經銷毀，不需要再檢查其他敵人
                }
            }
        }
    }

    // --- 符文追蹤者碰撞邏輯 ---
    for (auto &rune : m_Runetracers) {
        if (!rune.active) continue;

        const float runeHitRadius = m_Player->GetScaledSize().x * 0.5f * 0.5f;

        for (auto *enemyPtr : activeEnemies) {
            auto &enemy = *enemyPtr;
            if (enemy.active) {
                float enemyCollisionRadius = enemy.object->GetScaledSize().x * 0.35f;
                float hitRadius = runeHitRadius + enemyCollisionRadius;

                float dx = enemy.worldPosition.x - rune.worldPosition.x;
                float dy = enemy.worldPosition.y - rune.worldPosition.y;
                float distSq = dx * dx + dy * dy;

                if (distSq <= hitRadius * hitRadius && enemy.hitCooldownTimerMs <= 0.0f) {
                    enemy.hitCooldownTimerMs = 300.0f;  // 擊中冷卻
                    enemy.health -= m_RunetracerDamage;
                    m_PlayerHealth =
                        std::min(m_PlayerHealth + m_RunetracerDamage * m_PlayerVampirism, m_PlayerMaxHealth);

                    // 符文追蹤者不會因為碰撞而消失 (自帶穿透屬性)

                    if (enemy.health <= 0.0f) {
                        HandleEnemyDeath(enemy);
                    }
                }
            }
        }
    }

    // 處理寶石吸收系統
    const float pickupRadius = playerSize.x * 0.7f;  // 將吸收範圍改為 0.7 倍，不讓範圍過大
    for (auto &gem : m_ExpGems) {
        if (gem.active) {
            // 計算寶石冷卻，讓玩家有時間看到寶石掉落
            if (gem.pickupCooldownTimerMs > 0.0f && !gem.isMagnetized) {
                gem.pickupCooldownTimerMs -= deltaTimeMs;
            } else {
                float dist = glm::distance(m_PlayerWorldPosition, gem.worldPosition);

                if (gem.isMagnetized) {
                    // 吸住後快速飛向玩家
                    float moveDist = 1200.0f * (deltaTimeMs / 1000.0f);  // 再加快吸取速度
                    if (moveDist > dist) moveDist = dist;
                    glm::vec2 toPlayer = glm::normalize(m_PlayerWorldPosition - gem.worldPosition);
                    gem.worldPosition += toPlayer * moveDist;
                    dist = glm::distance(m_PlayerWorldPosition, gem.worldPosition);
                }

                if (dist <= pickupRadius) {
                    // 給玩家經驗值
                    m_PlayerExp += gem.expValue;
                    gem.active = false;
                    gem.object->SetVisible(false);

                    // 簡單的升級邏輯
                    if (m_PlayerExp >= m_PlayerExpNext) {
                        m_PlayerExp -= m_PlayerExpNext;
                        m_PlayerLevel++;
                        m_PlayerExpNext = static_cast<int>(m_PlayerExpNext * 1.15f);  // 升級需求遞增 (難度1.15倍)
                        m_PlayerHealth = m_PlayerMaxHealth;                           // 只要升級就回滿血

                        m_CurrentState = State::LEVEL_UP;  // 切換到升級暫停介面
                        GenerateUpgradeOptions();
                    }
                }
            }
            // 更新寶石位置
            gem.object->m_Transform.translation = gem.worldPosition - m_CameraPosition;
        }
    }

    // 處理 Boss Reward 的拾取
    for (auto &reward : m_RewardItems) {
        if (reward.active) {
            if (reward.pickupCooldownTimerMs > 0.0f) {
                reward.pickupCooldownTimerMs -= deltaTimeMs;
            } else {
                float dist = glm::distance(m_PlayerWorldPosition, reward.worldPosition);
                if (dist <= pickupRadius) {
                    reward.active = false;
                    reward.object->SetVisible(false);
                    m_PlayerHealth = m_PlayerMaxHealth;
                    m_PendingLevelUps += 3;
                    if (m_CurrentState != State::LEVEL_UP) {
                        m_CurrentState = State::LEVEL_UP;
                        GenerateUpgradeOptions();
                    }
                }
            }
            reward.object->m_Transform.translation = reward.worldPosition - m_CameraPosition;
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

    // 處理磁鐵道具的拾取
    for (auto &magnet : m_MagnetItems) {
        if (magnet.active) {
            if (magnet.pickupCooldownTimerMs > 0.0f) {
                magnet.pickupCooldownTimerMs -= deltaTimeMs;
            } else {
                float dist = glm::distance(m_PlayerWorldPosition, magnet.worldPosition);
                if (dist <= pickupRadius) {
                    magnet.active = false;
                    magnet.object->SetVisible(false);
                    m_MagnetTimerMs = 5000.0f;  // 啟動磁鐵效果 5 秒
                }
            }
            magnet.object->m_Transform.translation = magnet.worldPosition - m_CameraPosition;
        }
    }

    // 處理磁鐵持續時間內吸取寶石
    if (m_MagnetTimerMs > 0.0f) {
        m_MagnetTimerMs -= deltaTimeMs;
        float cullDistX = WINDOW_WIDTH * 0.6f;
        float cullDistY = WINDOW_HEIGHT * 0.6f;
        for (auto &gem : m_ExpGems) {
            if (gem.active && !gem.isMagnetized) {
                float dx = std::abs(gem.worldPosition.x - m_CameraPosition.x);
                float dy = std::abs(gem.worldPosition.y - m_CameraPosition.y);
                if (dx <= cullDistX && dy <= cullDistY) {
                    gem.isMagnetized = true;
                }
            }
        }
    }

    const float tileWidth = m_BackgroundTileSize.x;
    const float tileHeight = m_BackgroundTileSize.y;
    const float halfWindowWidth = static_cast<float>(WINDOW_WIDTH) * 0.5f;
    const float halfWindowHeight = static_cast<float>(WINDOW_HEIGHT) * 0.5f;

    const int minTileX = static_cast<int>(std::floor((m_CameraPosition.x - halfWindowWidth) / tileWidth)) - 1;
    const int maxTileX = static_cast<int>(std::floor((m_CameraPosition.x + halfWindowWidth) / tileWidth)) + 1;
    const int minTileY = static_cast<int>(std::floor((m_CameraPosition.y - halfWindowHeight) / tileHeight)) - 1;
    const int maxTileY = static_cast<int>(std::floor((m_CameraPosition.y + halfWindowHeight) / tileHeight)) + 1;

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
            tile.object->m_Transform.translation = glm::vec2{worldPosX, worldPosY} - m_CameraPosition;

            ++tileIndex;
        }
    }

    DrawGameObjects();

    // =============== UI 繪製區 ===============
    // 取得 ImGui 背景畫布來畫進度條與血條
    ImDrawList *drawList = ImGui::GetBackgroundDrawList();

    // 1. 最上方的經驗值條
    float expRatio = std::clamp((float)m_PlayerExp / m_PlayerExpNext, 0.0f, 1.0f);
    float expBarHeight = 24.0f;
    float expBarWidth = WINDOW_WIDTH - 80.0f;  // 留空間給右側的等級數字顯示
    // 黃色邊框包住整個頂部 (白框改為黃色，包住含 Level 區域)
    drawList->AddRectFilled(ImVec2(0, 0), ImVec2(WINDOW_WIDTH, expBarHeight), IM_COL32(0, 0, 0, 255));  // 黑底
    drawList->AddRectFilled(ImVec2(0, 0), ImVec2(expBarWidth * expRatio, expBarHeight),
                            IM_COL32(30, 60, 200, 255));  // 藍條
    // 在經驗值條和等級的最外圍畫黃色框
    drawList->AddRect(ImVec2(0, 0), ImVec2(WINDOW_WIDTH, expBarHeight), IM_COL32(230, 230, 50, 255), 0.0f, 0,
                      2.0f);  // 黃框 (厚度 2)
    // 區隔經驗值與 Level 的線 (可選)
    drawList->AddLine(ImVec2(expBarWidth, 0), ImVec2(expBarWidth, expBarHeight), IM_COL32(230, 230, 50, 255), 2.0f);

    // 2. 玩家腳下的血條
    // 計算血條在畫面中「玩家下方」的位置
    glm::vec2 screenPlayerPos = m_Player->m_Transform.translation;
    screenPlayerPos.x += (WINDOW_WIDTH / 2.0f);
    screenPlayerPos.y += (WINDOW_HEIGHT / 2.0f);  // 置中

    ImVec2 healthBarSize(playerSize.x * 1.0f, 10.0f);  // 與角色等同寬度比例
    // 置換在角色下方 (Y軸往下遞增，因 ImGui 座標系 origin 於左上)
    ImVec2 healthBarPos(screenPlayerPos.x - healthBarSize.x / 2.0f,
                        WINDOW_HEIGHT - screenPlayerPos.y + playerSize.y / 2.0f + 10.0f);

    // 繪製背景 (灰色)
    drawList->AddRectFilled(healthBarPos, ImVec2(healthBarPos.x + healthBarSize.x, healthBarPos.y + healthBarSize.y),
                            IM_COL32(80, 80, 80, 255));
    // 繪製前景 (血量比例，紅色)
    float healthRatio = std::clamp(m_PlayerHealth / m_PlayerMaxHealth, 0.0f, 1.0f);
    drawList->AddRectFilled(healthBarPos,
                            ImVec2(healthBarPos.x + (healthBarSize.x * healthRatio), healthBarPos.y + healthBarSize.y),
                            IM_COL32(230, 40, 40, 255));

    // 3. 上方數值顯示 (等級、時間、擊殺數)
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, 120), ImGuiCond_Always);
    ImGui::Begin("Player Status", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs |
                     ImGuiWindowFlags_NoMove);

    // 等級 (放在經驗條右側)
    ImGui::SetCursorPos(ImVec2(WINDOW_WIDTH - 70, 4));
    ImGui::TextColored(ImVec4(1, 1, 1, 1), "LV %d", m_PlayerLevel);

    // 格式化遊戲時間 (分:秒)
    int timeMinutes = static_cast<int>(m_GameTimeMs / 60000.0f) % 60;
    int timeSeconds = static_cast<int>(m_GameTimeMs / 1000.0f) % 60;
    char timeText[64];
    snprintf(timeText, sizeof(timeText), "%02d:%02d", timeMinutes, timeSeconds);

    // 時間置中與放大
    ImGui::SetWindowFontScale(2.5f);  // 放大時間顯示的字體 (2.5倍)
    ImVec2 timeTextSize = ImGui::CalcTextSize(timeText);
    ImGui::SetCursorPos(ImVec2((WINDOW_WIDTH - timeTextSize.x) * 0.5f, 30));
    ImGui::Text("%s", timeText);
    ImGui::SetWindowFontScale(1.0f);  // 還原字體大小給後面的 UI 使用

    // 殺敵數 (右側)
    char killText[64];
    snprintf(killText, sizeof(killText), "%d", m_EnemiesDefeated);
    ImVec2 killTextSize = ImGui::CalcTextSize(killText);

    float killX = WINDOW_WIDTH - killTextSize.x - 45.0f;
    ImGui::SetCursorPos(ImVec2(killX, 35));
    ImGui::Text("%s", killText);
    ImGui::SameLine();
    if (m_EnemyCountIconImage && m_EnemyCountIconImage->GetTextureId() != 0) {
        ImGui::SetCursorPosY(31);  // 稍微往上移對齊文字
        ImGui::Image((void *)(intptr_t)m_EnemyCountIconImage->GetTextureId(), ImVec2(30, 30));
    }

    // 預留鍵盤快捷鍵觸發暫停
    if (Util::Input::IsKeyUp(Util::Keycode::P)) {
        m_CurrentState = State::PAUSED;
    }

    ImGui::End();

    // 4. 獨立的暫停按鈕視窗 (接受滑鼠輸入)
    ImGui::SetNextWindowPos(ImVec2(WINDOW_WIDTH - 60, 75), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(50, 50), ImGuiCond_Always);
    ImGui::Begin("PauseUI", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);

    if (m_PauseIconImage && m_PauseIconImage->GetTextureId() != 0) {
        ImGui::Image((void *)(intptr_t)m_PauseIconImage->GetTextureId(), ImVec2(32, 32));
        // 若圖示被點擊，觸發暫停 (注意只處理左鍵 0)
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
            m_CurrentState = State::PAUSED;
        }
    } else {
        // Fallback 文字按鈕
        if (ImGui::Button("||", ImVec2(40, 40))) {
            m_CurrentState = State::PAUSED;
        }
    }
    ImGui::End();
    // =========================================

    /*
     * Do not touch the code below as they serve the purpose for
     * closing the window.
     */
    if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) || Util::Input::IfExit()) {
        m_CurrentState = State::END;
    }
}

void App::DrawGameObjects() {
    // 優化渲染效能：視錐體剔除 (Frustum Culling) - 不渲染畫面外物品
    const float cullDistX = WINDOW_WIDTH * 0.5f + 100.0f;  // 給予 100 像素緩衝區
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
    for (auto &reward : m_RewardItems) {
        if (reward.active) {
            if (std::abs(reward.worldPosition.x - m_CameraPosition.x) < cullDistX &&
                std::abs(reward.worldPosition.y - m_CameraPosition.y) < cullDistY) {
                reward.object->Draw();
            }
        }
    }
    for (auto &magnet : m_MagnetItems) {
        if (magnet.active) {
            if (std::abs(magnet.worldPosition.x - m_CameraPosition.x) < cullDistX &&
                std::abs(magnet.worldPosition.y - m_CameraPosition.y) < cullDistY) {
                magnet.object->Draw();
            }
        }
    }
    for (auto &enemy : m_Enemies) {
        if (enemy.active) {
            if (std::abs(enemy.worldPosition.x - m_CameraPosition.x) < cullDistX &&
                std::abs(enemy.worldPosition.y - m_CameraPosition.y) < cullDistY) {
                // 受擊閃爍特效改為「實體白化」: 只要還處於冷卻時間的前半段 (大於 150ms，因為總共是 300ms)
                // 就顯示白圖，給出單次閃爍的感覺
                if (enemy.hitCooldownTimerMs > 150.0f) {
                    enemy.object->SetDrawable(enemy.hurtImage);
                } else {
                    if (enemy.defaultAnimation) {
                        enemy.object->SetDrawable(enemy.defaultAnimation);
                    } else {
                        enemy.object->SetDrawable(enemy.defaultImage);
                    }
                }
                enemy.object->Draw();
            }
        }
    }

    // 玩家受擊特效已在 UpdateStart() 透過 SetState() 切換圖片，這裡一律畫出來
    m_Player->Draw();

    m_WeaponEffect.object->Draw();

    for (auto &knife : m_Knives) {
        if (knife.active) {
            if (std::abs(knife.worldPosition.x - m_CameraPosition.x) < cullDistX &&
                std::abs(knife.worldPosition.y - m_CameraPosition.y) < cullDistY) {
                knife.object->Draw();
            }
        }
    }

    // 繪製符文追蹤者及其殘影
    for (auto &rune : m_Runetracers) {
        if (rune.active) {
            // 這個物件的基本屬性是由我們啟動時決定的，拿來當 reference base
            const float targetRuneWidth = m_Player->GetScaledSize().x * 0.5f;
            const float baseScale = targetRuneWidth / m_RunetracerImage->GetSize().x;

            // 繪製殘影
            float scaleStep = baseScale / rune.maxHistory;
            // 反向迴圈：從最舊的殘影點(i 最大)開始畫到最新的殘影點(i = 0)，
            // 這樣新的殘影才會蓋在舊的殘影上面，並且最後繪製的本體會蓋在最新殘影的上面
            for (int i = static_cast<int>(rune.historyPositions.size()) - 1; i >= 0; --i) {
                float trailScale = baseScale - (i * scaleStep);  // 越舊的越小

                // 根據要求，殘影全部改成使用 Runetracer65
                rune.object->SetDrawable(m_RunetracerImage65);
                rune.object->SetZIndex(4.4f - (i * 0.01f));

                rune.object->m_Transform.translation = rune.historyPositions[i] - m_CameraPosition;
                rune.object->m_Transform.scale = {trailScale, trailScale};
                rune.object->Draw();
            }

            // 繪製完殘影後，將圖片換回預設 100% 準備畫本體
            rune.object->SetDrawable(m_RunetracerImage);
            rune.object->SetZIndex(4.5f);

            // 繪製本體
            if (std::abs(rune.worldPosition.x - m_CameraPosition.x) < cullDistX &&
                std::abs(rune.worldPosition.y - m_CameraPosition.y) < cullDistY) {
                rune.object->m_Transform.translation = rune.worldPosition - m_CameraPosition;
                rune.object->m_Transform.scale = {baseScale, baseScale};
                rune.object->Draw();
            }
        }
    }
}

void App::UpdatePaused() {
    if (m_BossLeftAnimation) m_BossLeftAnimation->Pause();
    if (m_BossRightAnimation) m_BossRightAnimation->Pause();
    if (m_Player) m_Player->PauseAnimation();
    if (m_BGM) m_BGM->Pause();  // 暫停時音樂也停下

    DrawGameObjects();  // 畫出底層但不會更新他們的邏輯，形成暫停效果

    // 1. 畫滿版半透明黑底，暗化背景
    ImDrawList *bgDrawList = ImGui::GetBackgroundDrawList();
    bgDrawList->AddRectFilled(ImVec2(0, 0), ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT), IM_COL32(0, 0, 0, 150));

    // 設定為滿版視窗
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT), ImGuiCond_Always);

    // 移除預設的標題列、背景等
    ImGui::Begin("PausedMenu", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);

    // ==========================================
    // 左側面板 (遊戲統計數據)
    // ==========================================
    ImGui::SetCursorPos(ImVec2(20, 20));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.0f);

    ImGui::BeginChild("LeftStatsPanel", ImVec2(280, WINDOW_HEIGHT - 100), true);
    ImGui::SetWindowFontScale(1.5f);
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Game Stats");
    ImGui::Separator();
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Dummy(ImVec2(0, 10));

    int timeMinutes = static_cast<int>(m_GameTimeMs / 60000.0f) % 60;
    int timeSeconds = static_cast<int>(m_GameTimeMs / 1000.0f) % 60;

    if (ImGui::BeginTable("GameStatsTable", 2, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        auto drawStatRow = [](const char *name, const char *val) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", name);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", val);
        };
        char buf[64];
        snprintf(buf, sizeof(buf), "%02d:%02d", timeMinutes, timeSeconds);
        drawStatRow("Time", buf);
        ImGui::Dummy(ImVec2(0, 10));
        snprintf(buf, sizeof(buf), "%d", m_EnemiesDefeated);
        drawStatRow("Defeated", buf);
        ImGui::Dummy(ImVec2(0, 10));
        snprintf(buf, sizeof(buf), "%d", m_PlayerLevel);
        drawStatRow("Level", buf);
        ImGui::Dummy(ImVec2(0, 10));
        snprintf(buf, sizeof(buf), "%d", m_CurrentWave);
        drawStatRow("Wave", buf);
        ImGui::Dummy(ImVec2(0, 10));
        snprintf(buf, sizeof(buf), "%d", m_CurrentStage);
        drawStatRow("Stage", buf);
        ImGui::EndTable();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    // ==========================================
    // 右側面板 (玩家能力數值)
    // ==========================================
    ImGui::SetCursorPos(ImVec2(320, 20));  // 定位到左側欄右邊
    // 設定顏色與金色邊框
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));  // 金色
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.0f);

    ImGui::BeginChild("RightStatsPanel", ImVec2(WINDOW_WIDTH - 340, WINDOW_HEIGHT - 100), true);

    ImGui::SetWindowFontScale(1.5f);
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Player Statistics");
    ImGui::Separator();
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Dummy(ImVec2(0, 10));

    if (ImGui::BeginTable("PlayerStatsTable", 2, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 180.0f);

        auto drawStatRow = [](const char *name, const std::string &val) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", name);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", val.c_str());
        };
        auto drawHeader = [](const char *name, bool locked) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Dummy(ImVec2(0, 10));  // Add some spacing before header
            ImGui::Separator();
            if (locked) {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", name);
            } else {
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "%s", name);
            }
        };

        drawStatRow("Max Health", std::to_string((int)m_PlayerMaxHealth));
        if (m_ArmorUnlocked) {
            drawStatRow("Armor Reduction",
                        std::to_string((int)(m_PlayerArmor * 100)) + "% (Lv " + std::to_string(m_ArmorLevel) + ")");
        } else {
            drawStatRow("Armor Reduction", "(Locked)");
        }
        if (m_VampirismUnlocked) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%.1f%% (Lv %d)", m_PlayerVampirism * 100.0f, m_VampirismLevel);
            drawStatRow("Lifesteal", buf);
        } else {
            drawStatRow("Lifesteal", "(Locked)");
        }

        drawHeader("[ Whip ]", false);
        drawStatRow("Range", std::to_string((int)(m_WeaponHitRadiusRatioToPlayer * 100)) + "% (Lv " +
                                 std::to_string(m_WhipRangeLevel) + ")");
        drawStatRow("Damage", std::to_string((int)m_WeaponDamage) + " (Lv " + std::to_string(m_WhipDamageLevel) + ")");
        drawStatRow("Cooldown", std::to_string((int)m_WeaponAttackIntervalMs) + "ms (Lv " +
                                    std::to_string(m_WhipCooldownLevel) + ")");

        if (m_KnifeUnlocked) {
            drawHeader("[ Knife ]", false);
            drawStatRow("Count", std::to_string(m_KnifeCount) + " (Lv " + std::to_string(m_KnifeCountLevel) + ")");
            drawStatRow("Damage",
                        std::to_string((int)m_KnifeDamage) + " (Lv " + std::to_string(m_KnifeDamageLevel) + ")");
            drawStatRow("Cooldown", std::to_string((int)m_KnifeAttackIntervalMs) + "ms (Lv " +
                                        std::to_string(m_KnifeCooldownLevel) + ")");
        } else {
            drawHeader("[ Knife ] (Locked)", true);
        }

        if (m_RunetracerUnlocked) {
            drawHeader("[ Runetracer ]", false);
            drawStatRow("Count",
                        std::to_string(m_RunetracerCount) + " (Lv " + std::to_string(m_RunetracerCountLevel) + ")");
            drawStatRow("Damage", std::to_string((int)m_RunetracerDamage) + " (Lv " +
                                      std::to_string(m_RunetracerDamageLevel) + ")");
            drawStatRow("Cooldown", std::to_string((int)m_RunetracerAttackIntervalMs) + "ms (Lv " +
                                        std::to_string(m_RunetracerCooldownLevel) + ")");
        } else {
            drawHeader("[ Runetracer ] (Locked)", true);
        }

        ImGui::EndTable();
    }

    ImGui::SetWindowFontScale(1.0f);
    ImGui::EndChild();

    // 還原顏色設定
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    // ==========================================
    // 底部按鈕區
    // ==========================================
    float buttonY = WINDOW_HEIGHT - 70;

    // 賦予按鈕金色邊框
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));  // 金色
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);

    // 離開按鈕 (紅色風格)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
    ImGui::SetCursorPos(ImVec2(20, buttonY));
    if (ImGui::Button("EXIT", ImVec2(150, 40))) {
        m_CurrentState = State::END;
    }
    ImGui::PopStyleColor();

    // 繼續按鈕 (藍底黃字風格)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.3f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.2f, 1.0f));

    ImGui::SetCursorPos(ImVec2(WINDOW_WIDTH - 220, buttonY));
    if (ImGui::Button("CONTINUE", ImVec2(200, 40)) || Util::Input::IsKeyUp(Util::Keycode::P)) {
        if (m_BGM) m_BGM->Resume();
        m_CurrentState = State::UPDATE;
    }
    ImGui::PopStyleColor(2);

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImGui::End();

    if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) || Util::Input::IfExit()) {
        m_CurrentState = State::END;
    }
}

void App::UpdateLevelUp() {
    if (m_BossLeftAnimation) m_BossLeftAnimation->Pause();
    if (m_BossRightAnimation) m_BossRightAnimation->Pause();
    if (m_Player) m_Player->PauseAnimation();
    DrawGameObjects();

    // 畫出升級畫面的背景物件
    m_LevelUpObject->m_Transform.translation = {0.0f, 0.0f};
    m_LevelUpObject->Draw();

    // 設定 ImGui 視窗蓋在圖片上方 (置中)
    ImGui::SetNextWindowPos(ImVec2(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_Always);

    // 把 ImGui 背景設為透明，並且隱藏視窗邊框與標題列，以便看到後面的圖片
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));                     // 按鈕預設透明
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.1f));  // 懸停時稍微有點亮度提示
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.2f));   // 按下時亮度增加
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("Level Up!", nullptr,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoTitleBar);

    ImGui::Dummy(ImVec2(0, 105));

    for (int i = 0; i < m_CurrentUpgradeOptions.size(); ++i) {
        UpgradeType type = m_CurrentUpgradeOptions[i];
        std::string buttonText = "";

        switch (type) {
            case UpgradeType::UNLOCK_KNIFE:
                buttonText = "Unlock Knife";
                break;
            case UpgradeType::UNLOCK_RUNETRACER:
                buttonText = "Unlock Runetracer";
                break;
            case UpgradeType::UNLOCK_ARMOR:
                buttonText = "Unlock Armor (5%)";
                break;
            case UpgradeType::UNLOCK_VAMPIRISM:
                buttonText = "Unlock Lifesteal (0.5%)";
                break;
            case UpgradeType::WHIP_RANGE:
                buttonText = "Whip Range Up (Lv " + std::to_string(m_WhipRangeLevel + 1) + ")";
                break;
            case UpgradeType::WHIP_DAMAGE:
                buttonText = "Whip Damage Up (Lv " + std::to_string(m_WhipDamageLevel + 1) + ")";
                break;
            case UpgradeType::WHIP_COOLDOWN:
                buttonText = "Whip Cooldown Down (Lv " + std::to_string(m_WhipCooldownLevel + 1) + ")";
                break;
            case UpgradeType::KNIFE_COUNT:
                buttonText = "Knife Count +2 (Lv " + std::to_string(m_KnifeCountLevel + 1) + ")";
                break;
            case UpgradeType::KNIFE_DAMAGE:
                buttonText = "Knife Damage Up (Lv " + std::to_string(m_KnifeDamageLevel + 1) + ")";
                break;
            case UpgradeType::KNIFE_COOLDOWN:
                buttonText = "Knife Cooldown Down (Lv " + std::to_string(m_KnifeCooldownLevel + 1) + ")";
                break;
            case UpgradeType::RUNETRACER_COUNT:
                buttonText = "Runetracer Count +1 (Lv " + std::to_string(m_RunetracerCountLevel + 1) + ")";
                break;
            case UpgradeType::RUNETRACER_DAMAGE:
                buttonText = "Runetracer Damage Up (Lv " + std::to_string(m_RunetracerDamageLevel + 1) + ")";
                break;
            case UpgradeType::RUNETRACER_COOLDOWN:
                buttonText = "Runetracer Cooldown Down (Lv " + std::to_string(m_RunetracerCooldownLevel + 1) + ")";
                break;
            case UpgradeType::MAX_HEALTH:
                buttonText = "Max Health +100";
                break;
            case UpgradeType::ARMOR:
                buttonText = "Armor +5% (Lv " + std::to_string(m_ArmorLevel + 1) + ")";
                break;
            case UpgradeType::VAMPIRISM:
                buttonText = "Lifesteal +0.5% (Lv " + std::to_string(m_VampirismLevel + 1) + ")";
                break;
        }

        if (ImGui::Button(buttonText.c_str(), ImVec2(380, 80))) {
            switch (type) {
                case UpgradeType::UNLOCK_KNIFE:
                    m_KnifeUnlocked = true;
                    break;
                case UpgradeType::UNLOCK_RUNETRACER:
                    m_RunetracerUnlocked = true;
                    break;
                case UpgradeType::UNLOCK_ARMOR:
                    m_ArmorUnlocked = true;
                    m_PlayerArmor = 0.05f;
                    break;
                case UpgradeType::UNLOCK_VAMPIRISM:
                    m_VampirismUnlocked = true;
                    m_PlayerVampirism = 0.005f;
                    break;
                case UpgradeType::WHIP_RANGE:
                    m_WhipRangeLevel++;
                    m_WeaponHitRadiusRatioToPlayer *= 1.20f;
                    {
                        const glm::vec2 weaponNativeSize = m_WeaponEffect.activeAnimation->GetSize();
                        const float targetWeaponWidth = m_Player->GetScaledSize().x * m_WeaponWidthRatioToPlayer *
                                                        (m_WeaponHitRadiusRatioToPlayer / 1.0f);
                        const float weaponScale = targetWeaponWidth / weaponNativeSize.x;
                        m_WeaponEffect.object->m_Transform.scale = {weaponScale, weaponScale};
                    }
                    break;
                case UpgradeType::WHIP_DAMAGE:
                    m_WhipDamageLevel++;
                    m_WeaponDamage += 15.0f;
                    break;
                case UpgradeType::WHIP_COOLDOWN:
                    m_WhipCooldownLevel++;
                    m_WeaponAttackIntervalMs *= 0.80f;
                    break;
                case UpgradeType::KNIFE_COUNT:
                    m_KnifeCountLevel++;
                    m_KnifeCount += 2;
                    break;
                case UpgradeType::KNIFE_DAMAGE:
                    m_KnifeDamageLevel++;
                    m_KnifeDamage += 15.0f;
                    break;
                case UpgradeType::KNIFE_COOLDOWN:
                    m_KnifeCooldownLevel++;
                    m_KnifeAttackIntervalMs *= 0.80f;
                    break;
                case UpgradeType::RUNETRACER_COUNT:
                    m_RunetracerCountLevel++;
                    m_RunetracerCount++;
                    break;
                case UpgradeType::RUNETRACER_DAMAGE:
                    m_RunetracerDamageLevel++;
                    m_RunetracerDamage += 15.0f;
                    break;
                case UpgradeType::RUNETRACER_COOLDOWN:
                    m_RunetracerCooldownLevel++;
                    m_RunetracerAttackIntervalMs *= 0.80f;
                    break;
                case UpgradeType::MAX_HEALTH:
                    m_MaxHealthLevel++;
                    m_PlayerMaxHealth += 100.0f;
                    m_PlayerHealth += 100.0f;
                    break;
                case UpgradeType::ARMOR:
                    m_ArmorLevel++;
                    m_PlayerArmor += 0.05f;
                    break;  // 5% reduction per level
                case UpgradeType::VAMPIRISM:
                    m_VampirismLevel++;
                    m_PlayerVampirism += 0.005f;
                    break;  // 0.5% lifesteal per level
            }
            if (m_PendingLevelUps > 0) {
                m_PendingLevelUps--;
                GenerateUpgradeOptions();
            } else {
                m_CurrentState = State::UPDATE;
            }
        }
        ImGui::Dummy(ImVec2(0, 30));
    }

    ImGui::End();

    // 恢復樣式 (按鈕顏色3個 + 視窗背景1個 = 4個)
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);

    if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) || Util::Input::IfExit()) {
        m_CurrentState = State::END;
    }
}

void App::GenerateUpgradeOptions() {
    m_CurrentUpgradeOptions.clear();
    std::vector<UpgradeType> validPool;

    if (!m_KnifeUnlocked) validPool.push_back(UpgradeType::UNLOCK_KNIFE);
    if (!m_RunetracerUnlocked) validPool.push_back(UpgradeType::UNLOCK_RUNETRACER);

    if (m_WhipRangeLevel < 15) validPool.push_back(UpgradeType::WHIP_RANGE);
    if (m_WhipDamageLevel < 15) validPool.push_back(UpgradeType::WHIP_DAMAGE);
    if (m_WhipCooldownLevel < 15) validPool.push_back(UpgradeType::WHIP_COOLDOWN);

    if (m_KnifeUnlocked) {
        if (m_KnifeCountLevel < 15) validPool.push_back(UpgradeType::KNIFE_COUNT);
        if (m_KnifeDamageLevel < 15) validPool.push_back(UpgradeType::KNIFE_DAMAGE);
        if (m_KnifeCooldownLevel < 15) validPool.push_back(UpgradeType::KNIFE_COOLDOWN);
    }

    if (m_RunetracerUnlocked) {
        if (m_RunetracerCountLevel < 15) validPool.push_back(UpgradeType::RUNETRACER_COUNT);
        if (m_RunetracerDamageLevel < 15) validPool.push_back(UpgradeType::RUNETRACER_DAMAGE);
        if (m_RunetracerCooldownLevel < 15) validPool.push_back(UpgradeType::RUNETRACER_COOLDOWN);
    }

    if (m_MaxHealthLevel < 15) validPool.push_back(UpgradeType::MAX_HEALTH);
    if (!m_ArmorUnlocked)
        validPool.push_back(UpgradeType::UNLOCK_ARMOR);
    else if (m_ArmorLevel < 15)
        validPool.push_back(UpgradeType::ARMOR);

    if (!m_VampirismUnlocked)
        validPool.push_back(UpgradeType::UNLOCK_VAMPIRISM);
    else if (m_VampirismLevel < 15)
        validPool.push_back(UpgradeType::VAMPIRISM);

    if (validPool.empty()) {
        validPool.push_back(UpgradeType::MAX_HEALTH);
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(validPool.begin(), validPool.end(), g);

    for (int i = 0; i < std::min((int)validPool.size(), 3); ++i) {
        m_CurrentUpgradeOptions.push_back(validPool[i]);
    }
}

void App::UpdateGameOver() {
    if (m_BossLeftAnimation) m_BossLeftAnimation->Pause();
    if (m_BossRightAnimation) m_BossRightAnimation->Pause();
    if (m_Player) m_Player->PauseAnimation();
    DrawGameObjects();

    ImDrawList *bgDrawList = ImGui::GetBackgroundDrawList();
    bgDrawList->AddRectFilled(ImVec2(0, 0), ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT), IM_COL32(150, 0, 0, 150));

    if (m_GameOverImage && m_GameOverImage->GetTextureId() != 0) {
        ImVec2 texSize(m_GameOverImage->GetSize().x, m_GameOverImage->GetSize().y);
        ImVec2 pos((WINDOW_WIDTH - texSize.x) * 0.5f, 50.0f);
        bgDrawList->AddImage((void *)(intptr_t)m_GameOverImage->GetTextureId(), pos,
                             ImVec2(pos.x + texSize.x, pos.y + texSize.y));
    }

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT), ImGuiCond_Always);
    ImGui::Begin("GameOverMenu", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);

    ImGui::SetCursorPos(ImVec2(20, 250));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.0f);

    ImGui::BeginChild("LeftStatsPanel", ImVec2(280, WINDOW_HEIGHT - 350), true);
    ImGui::SetWindowFontScale(1.5f);
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Game Stats");
    ImGui::Separator();
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Dummy(ImVec2(0, 10));

    int timeMinutes = static_cast<int>(m_GameTimeMs / 60000.0f) % 60;
    int timeSeconds = static_cast<int>(m_GameTimeMs / 1000.0f) % 60;

    if (ImGui::BeginTable("GameStatsTable", 2, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        auto drawStatRow = [](const char *name, const char *val) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", name);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", val);
        };
        char buf[64];
        snprintf(buf, sizeof(buf), "%02d:%02d", timeMinutes, timeSeconds);
        drawStatRow("Time", buf);
        ImGui::Dummy(ImVec2(0, 10));
        snprintf(buf, sizeof(buf), "%d", m_EnemiesDefeated);
        drawStatRow("Defeated", buf);
        ImGui::Dummy(ImVec2(0, 10));
        snprintf(buf, sizeof(buf), "%d", m_PlayerLevel);
        drawStatRow("Level", buf);
        ImGui::Dummy(ImVec2(0, 10));
        snprintf(buf, sizeof(buf), "%d", m_CurrentWave);
        drawStatRow("Wave", buf);
        ImGui::Dummy(ImVec2(0, 10));
        snprintf(buf, sizeof(buf), "%d", m_CurrentStage);
        drawStatRow("Stage", buf);
        ImGui::EndTable();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    ImGui::SetCursorPos(ImVec2(320, 250));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.0f);

    ImGui::BeginChild("RightStatsPanel", ImVec2(WINDOW_WIDTH - 340, WINDOW_HEIGHT - 350), true);

    ImGui::SetWindowFontScale(1.5f);
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Player Statistics");
    ImGui::Separator();
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Dummy(ImVec2(0, 10));

    if (ImGui::BeginTable("PlayerStatsTable", 2, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 180.0f);

        auto drawStatRow = [](const char *name, const std::string &val) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", name);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", val.c_str());
        };
        auto drawHeader = [](const char *name, bool locked) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Dummy(ImVec2(0, 10));
            ImGui::Separator();
            if (locked) {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", name);
            } else {
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "%s", name);
            }
        };

        drawStatRow("Max Health", std::to_string((int)m_PlayerMaxHealth));
        if (m_ArmorUnlocked) {
            drawStatRow("Armor Reduction",
                        std::to_string((int)(m_PlayerArmor * 100)) + "% (Lv " + std::to_string(m_ArmorLevel) + ")");
        } else {
            drawStatRow("Armor Reduction", "(Locked)");
        }
        if (m_VampirismUnlocked) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%.1f%% (Lv %d)", m_PlayerVampirism * 100.0f, m_VampirismLevel);
            drawStatRow("Lifesteal", buf);
        } else {
            drawStatRow("Lifesteal", "(Locked)");
        }

        drawHeader("[ Whip ]", false);
        drawStatRow("Range", std::to_string((int)(m_WeaponHitRadiusRatioToPlayer * 100)) + "% (Lv " +
                                 std::to_string(m_WhipRangeLevel) + ")");
        drawStatRow("Damage", std::to_string((int)m_WeaponDamage) + " (Lv " + std::to_string(m_WhipDamageLevel) + ")");
        drawStatRow("Cooldown", std::to_string((int)m_WeaponAttackIntervalMs) + "ms (Lv " +
                                    std::to_string(m_WhipCooldownLevel) + ")");

        if (m_KnifeUnlocked) {
            drawHeader("[ Knife ]", false);
            drawStatRow("Count", std::to_string(m_KnifeCount) + " (Lv " + std::to_string(m_KnifeCountLevel) + ")");
            drawStatRow("Damage",
                        std::to_string((int)m_KnifeDamage) + " (Lv " + std::to_string(m_KnifeDamageLevel) + ")");
            drawStatRow("Cooldown", std::to_string((int)m_KnifeAttackIntervalMs) + "ms (Lv " +
                                        std::to_string(m_KnifeCooldownLevel) + ")");
        } else {
            drawHeader("[ Knife ] (Locked)", true);
        }

        if (m_RunetracerUnlocked) {
            drawHeader("[ Runetracer ]", false);
            drawStatRow("Count",
                        std::to_string(m_RunetracerCount) + " (Lv " + std::to_string(m_RunetracerCountLevel) + ")");
            drawStatRow("Damage", std::to_string((int)m_RunetracerDamage) + " (Lv " +
                                      std::to_string(m_RunetracerDamageLevel) + ")");
            drawStatRow("Cooldown", std::to_string((int)m_RunetracerAttackIntervalMs) + "ms (Lv " +
                                        std::to_string(m_RunetracerCooldownLevel) + ")");
        } else {
            drawHeader("[ Runetracer ] (Locked)", true);
        }

        ImGui::EndTable();
    }

    ImGui::SetWindowFontScale(1.0f);
    ImGui::EndChild();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    float buttonY = WINDOW_HEIGHT - 70;

    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
    ImGui::SetCursorPos(ImVec2(20, buttonY));
    if (ImGui::Button("EXIT", ImVec2(150, 40))) {
        m_CurrentState = State::END;
    }
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.3f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.2f, 1.0f));

    ImGui::SetCursorPos(ImVec2(WINDOW_WIDTH - 220, buttonY));
    if (ImGui::Button("RESTART", ImVec2(200, 40))) {
        m_CurrentState = State::START;
    }
    ImGui::PopStyleColor(2);

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImGui::End();

    if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) || Util::Input::IfExit()) {
        m_CurrentState = State::END;
    }
}

void App::HandleEnemyDeath(EnemyUnit &enemy) {
    enemy.active = false;
    enemy.object->SetVisible(false);

    m_EnemiesDefeated++;
    if (m_EnemiesDefeated >= m_KillsToNextWave) {
        m_CurrentWave++;
        // 前期很快升級波次，後期需求擊殺數越來越多
        int nextRequirement = 10 + (m_CurrentWave - 1) * 5;
        m_KillsToNextWave += nextRequirement;
        m_CurrentStage = (m_CurrentWave - 1) / 5 + 1;
        // 後期怪物生成數量加多
        m_EnemySpawnIntervalMs = std::max(50.0f, 1200.0f - static_cast<float>(m_CurrentWave - 1) * 150.0f);
    }

    static std::mt19937 dropRng(std::random_device{}());
    std::uniform_real_distribution<float> dropDist(0.0f, 100.0f);
    float dropRoll = dropDist(dropRng);

    if (enemy.isBoss) {
        for (auto &reward : m_RewardItems) {
            if (!reward.active) {
                reward.active = true;
                reward.worldPosition = enemy.worldPosition;
                reward.pickupCooldownTimerMs = 150.0f;
                reward.object->SetVisible(true);
                break;
            }
        }
    }

    // 掉落經驗值寶石 (依機率決定掉落哪一種)
    for (auto &gem : m_ExpGems) {
        if (!gem.active) {
            gem.active = true;
            gem.worldPosition = enemy.worldPosition;
            gem.pickupCooldownTimerMs = 150.0f;
            gem.isMagnetized = false;  // 重置磁吸狀態

            std::shared_ptr<Util::Image> selectedImage;
            if (dropRoll < 5.0f) {
                gem.expValue = 10;
                selectedImage = m_Gem3Image;
            } else if (dropRoll < 20.0f) {
                gem.expValue = 4;
                selectedImage = m_Gem2Image;
            } else {
                gem.expValue = 1;
                selectedImage = m_Gem1Image;
            }

            gem.object->SetDrawable(selectedImage);

            const float targetWidth = m_Player->GetScaledSize().x * m_ExpGemSizeRatioToPlayer;
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
                potion.worldPosition = enemy.worldPosition + glm::vec2(20.0f, 20.0f);
                potion.pickupCooldownTimerMs = 150.0f;
                potion.object->SetVisible(true);
                break;
            }
        }
    }

    // 3% 機率掉落磁鐵道具 (magnet.png)
    if (dropDist(dropRng) < 3.0f) {
        for (auto &magnet : m_MagnetItems) {
            if (!magnet.active) {
                magnet.active = true;
                magnet.worldPosition = enemy.worldPosition + glm::vec2(-20.0f, -20.0f);
                magnet.pickupCooldownTimerMs = 150.0f;
                magnet.object->SetVisible(true);
                break;
            }
        }
    }
}

void App::End() {  // NOLINT(this method will mutate members in the future)
    LOG_TRACE("End");
}
