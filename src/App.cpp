#include "App.hpp"

#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"
#include "Util/Time.hpp"
#include "config.hpp"

void App::Init() {
    if (m_Initialized) return;

    m_Camera = std::make_shared<Camera>();
    m_Player = std::make_shared<Player>();
    m_Background = std::make_shared<Background>();
    m_EnemyManager = std::make_shared<EnemyManager>();
    m_WeaponManager = std::make_shared<WeaponManager>();
    m_DropManager = std::make_shared<DropManager>();
    m_UpgradeManager = std::make_shared<UpgradeManager>();
    m_UIManager = std::make_shared<UIManager>();

    std::string resDir = std::string(RESOURCE_DIR);

    m_Background->Init(resDir + "/ground.png");
    m_EnemyManager->Init(resDir);
    m_WeaponManager->Init(resDir, *m_Player);
    m_DropManager->Init(resDir, *m_Player);
    m_UIManager->Init(resDir);

    try {
        m_BGM = std::make_shared<Util::BGM>(resDir + "/bgm.mp3");
        m_BGM->SetVolume(50);
        m_BGM->Play(-1);
    } catch (...) {
        // Ignore audio errors if file missing
    }

    m_Initialized = true;
}

void App::UpdateTitle() {
    Init();

    bool startRequested = false;
    m_UIManager->DrawTitleScreen(m_TitleScreen, m_TitleImage, startRequested);

    if (startRequested) {
        m_CurrentState = State::START;
    }
}

void App::Start() {
    LOG_TRACE("Start");
    Init();

    m_GameTimeMs = 0.0f;
    m_Player->Reset();
    m_Camera->SetPosition({0.0f, 0.0f});
    m_EnemyManager->Reset();
    m_WeaponManager->Reset();
    m_DropManager->Reset();
    m_UpgradeManager->Reset();

    m_CurrentState = State::UPDATE;
}

void App::Update() {
    const float deltaTimeMs = Util::Time::GetDeltaTimeMs();
    m_GameTimeMs += deltaTimeMs;

    m_EnemyManager->PlayAnimations();
    m_Player->PlayAnimation();

    // Input & Player Movement
    glm::vec2 moveDir = {0.0f, 0.0f};
    if (Util::Input::IsKeyPressed(Util::Keycode::W)) moveDir.y += 1.0f;
    if (Util::Input::IsKeyPressed(Util::Keycode::S)) moveDir.y -= 1.0f;
    if (Util::Input::IsKeyPressed(Util::Keycode::A)) moveDir.x -= 1.0f;
    if (Util::Input::IsKeyPressed(Util::Keycode::D)) moveDir.x += 1.0f;

    m_Player->UpdateMovement(deltaTimeMs, moveDir);

    // Camera follows player
    m_Camera->SetPosition(m_Player->GetWorldPosition());
    m_Player->Update(deltaTimeMs, *m_Camera);

    // Background update
    m_Background->Update(*m_Camera);

    // Enemy update & player collision
    bool playerDied = false;
    m_EnemyManager->Update(deltaTimeMs, *m_Player, *m_Camera,
                           [this](Enemy& e) { HandleEnemyDeath(e); },
                           playerDied);

    if (playerDied) {
        m_CurrentState = State::GAME_OVER;
        return;
    }

    // Weapons update
    m_WeaponManager->Update(deltaTimeMs, *m_Player, *m_Camera,
                            *m_EnemyManager,
                            [this](Enemy& e) { HandleEnemyDeath(e); });

    // Drops & Pickups update
    int pendingLevelUps = 0;
    bool triggerLevelUp = false;
    m_DropManager->Update(deltaTimeMs, *m_Player, *m_Camera,
                          pendingLevelUps, triggerLevelUp);

    if (pendingLevelUps > 0) {
        m_UpgradeManager->AddPendingLevelUps(pendingLevelUps);
    }

    if (triggerLevelUp) {
        m_CurrentState = State::LEVEL_UP;
        m_UpgradeManager->GenerateUpgradeOptions(*m_Player, *m_WeaponManager);
    }

    // Render game objects
    DrawGameObjects();

    // Render HUD
    bool pauseRequested = false;
    m_UIManager->DrawHUD(*m_Player, m_EnemyManager->GetEnemiesDefeated(), m_GameTimeMs, pauseRequested);
    if (pauseRequested) {
        m_CurrentState = State::PAUSED;
    }

    if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) || Util::Input::IfExit()) {
        m_CurrentState = State::END;
    }
}

void App::DrawGameObjects() {
    m_Background->Draw();
    m_DropManager->Draw(*m_Camera);
    m_EnemyManager->Draw(*m_Camera);
    m_Player->Draw();
    m_WeaponManager->Draw(*m_Camera);
}

void App::HandleEnemyDeath(Enemy& enemy) {
    m_EnemyManager->OnEnemyKilled(enemy);
    m_DropManager->HandleEnemyDeath(enemy, *m_Player);
}

void App::UpdatePaused() {
    m_EnemyManager->PauseAnimations();
    m_Player->PauseAnimation();
    if (m_BGM) m_BGM->Pause();

    DrawGameObjects();

    bool continueRequested = false;
    bool exitRequested = false;

    m_UIManager->DrawPausedMenu(*m_Player, *m_WeaponManager,
                                m_EnemyManager->GetEnemiesDefeated(),
                                m_EnemyManager->GetCurrentWave(),
                                m_EnemyManager->GetCurrentStage(),
                                m_GameTimeMs,
                                continueRequested, exitRequested);

    if (continueRequested) {
        if (m_BGM) m_BGM->Resume();
        m_CurrentState = State::UPDATE;
    }

    if (exitRequested || Util::Input::IsKeyUp(Util::Keycode::ESCAPE) || Util::Input::IfExit()) {
        m_CurrentState = State::END;
    }
}

void App::UpdateGameOver() {
    m_EnemyManager->PauseAnimations();
    m_Player->PauseAnimation();

    DrawGameObjects();

    bool restartRequested = false;
    bool exitRequested = false;

    m_UIManager->DrawGameOverMenu(*m_Player, *m_WeaponManager,
                                  m_EnemyManager->GetEnemiesDefeated(),
                                  m_EnemyManager->GetCurrentWave(),
                                  m_EnemyManager->GetCurrentStage(),
                                  m_GameTimeMs,
                                  restartRequested, exitRequested);

    if (restartRequested) {
        m_CurrentState = State::START;
    }

    if (exitRequested || Util::Input::IsKeyUp(Util::Keycode::ESCAPE) || Util::Input::IfExit()) {
        m_CurrentState = State::END;
    }
}

void App::UpdateLevelUp() {
    m_EnemyManager->PauseAnimations();
    m_Player->PauseAnimation();

    DrawGameObjects();

    m_UIManager->DrawLevelUpMenu(
        m_UpgradeManager->GetCurrentOptions(),
        *m_Player, *m_WeaponManager,
        [this](UpgradeType type) {
            m_UpgradeManager->ApplyUpgrade(type, *m_Player, *m_WeaponManager);
            if (m_UpgradeManager->ConsumePendingLevelUp()) {
                m_UpgradeManager->GenerateUpgradeOptions(*m_Player, *m_WeaponManager);
            } else {
                m_CurrentState = State::UPDATE;
            }
        });

    if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) || Util::Input::IfExit()) {
        m_CurrentState = State::END;
    }
}

void App::End() {
    LOG_TRACE("End");
}
