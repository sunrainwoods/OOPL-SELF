#include "UIManager.hpp"
#include "Player.hpp"
#include "WeaponManager.hpp"
#include "config.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Time.hpp"

#include <imgui.h>
#include <algorithm>
#include <cstdio>

UIManager::UIManager() = default;

void UIManager::Init(const std::string& resourceDir) {
    m_PauseIconImage = std::make_shared<Util::Image>(resourceDir + "/pause_icon.png");
    m_EnemyCountIconImage = std::make_shared<Util::Image>(resourceDir + "/enemy_count_icon.png");
    m_GameOverImage = std::make_shared<Util::Image>(resourceDir + "/game_over.png");

    m_LevelUpImage = std::make_shared<Util::Image>(resourceDir + "/levelup.png");
    m_LevelUpObject = std::make_shared<Util::GameObject>();
    m_LevelUpObject->SetDrawable(m_LevelUpImage);
    m_LevelUpObject->SetZIndex(100.0f);
    m_LevelUpObject->SetVisible(true);

    const float levelUpScale = static_cast<float>(WINDOW_HEIGHT) / m_LevelUpImage->GetSize().y * 0.9f;
    m_LevelUpObject->m_Transform.scale = {levelUpScale, levelUpScale};
}

void UIManager::DrawTitleScreen(std::shared_ptr<Util::GameObject>& titleScreen,
                                std::shared_ptr<Util::Image>& titleImage,
                                bool& startRequested) {
    startRequested = false;

    if (!titleScreen) {
        titleScreen = std::make_shared<Util::GameObject>();
        titleImage = std::make_shared<Util::Image>(std::string(RESOURCE_DIR) + "/Vampire_Survivors_start.jpg");
        titleScreen->SetDrawable(titleImage);
        titleScreen->SetZIndex(99.0f);

        const glm::vec2 imageSize = titleImage->GetSize();
        titleScreen->m_Transform.scale = {static_cast<float>(WINDOW_WIDTH) / imageSize.x,
                                          static_cast<float>(WINDOW_HEIGHT) / imageSize.y};
    }

    titleScreen->m_Transform.translation = glm::vec2(0.0f, 0.0f);
    titleScreen->Draw();

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)), ImGuiCond_Always);
    ImGui::Begin("TitleScreenUI", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs |
                     ImGuiWindowFlags_NoMove);

    if (static_cast<int>(Util::Time::GetElapsedTimeMs() / 500.0f) % 2 == 0) {
        ImVec2 textSize = ImGui::CalcTextSize("PRESS ANY KEY TO START");
        ImGui::SetCursorPos(ImVec2((static_cast<float>(WINDOW_WIDTH) - textSize.x) * 0.5f,
                                   static_cast<float>(WINDOW_HEIGHT) * 0.75f));
        ImGui::TextColored(ImVec4(1, 1, 1, 1), "PRESS ANY KEY TO START");
    }

    ImGui::End();

    if (Util::Input::IfAnyKeyPressed()) {
        startRequested = true;
    }
}

void UIManager::DrawHUD(const Player& player, int defeated, float gameTimeMs, bool& pauseRequested) {
    pauseRequested = false;
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    // 1. Exp Bar
    float expRatio = std::clamp(static_cast<float>(player.GetExp()) / player.GetExpNext(), 0.0f, 1.0f);
    float expBarHeight = 24.0f;
    float expBarWidth = static_cast<float>(WINDOW_WIDTH) - 80.0f;

    drawList->AddRectFilled(ImVec2(0, 0), ImVec2(static_cast<float>(WINDOW_WIDTH), expBarHeight), IM_COL32(0, 0, 0, 255));
    drawList->AddRectFilled(ImVec2(0, 0), ImVec2(expBarWidth * expRatio, expBarHeight), IM_COL32(30, 60, 200, 255));
    drawList->AddRect(ImVec2(0, 0), ImVec2(static_cast<float>(WINDOW_WIDTH), expBarHeight), IM_COL32(230, 230, 50, 255), 0.0f, 0, 2.0f);
    drawList->AddLine(ImVec2(expBarWidth, 0), ImVec2(expBarWidth, expBarHeight), IM_COL32(230, 230, 50, 255), 2.0f);

    // 2. Health Bar under player
    glm::vec2 playerSize = player.GetScaledSize();
    glm::vec2 screenPlayerPos = player.m_Transform.translation;
    screenPlayerPos.x += (static_cast<float>(WINDOW_WIDTH) / 2.0f);
    screenPlayerPos.y += (static_cast<float>(WINDOW_HEIGHT) / 2.0f);

    ImVec2 healthBarSize(playerSize.x * 1.0f, 10.0f);
    ImVec2 healthBarPos(screenPlayerPos.x - healthBarSize.x / 2.0f,
                        static_cast<float>(WINDOW_HEIGHT) - screenPlayerPos.y + playerSize.y / 2.0f + 10.0f);

    drawList->AddRectFilled(healthBarPos, ImVec2(healthBarPos.x + healthBarSize.x, healthBarPos.y + healthBarSize.y),
                            IM_COL32(80, 80, 80, 255));
    float healthRatio = std::clamp(player.GetHealth() / player.GetMaxHealth(), 0.0f, 1.0f);
    drawList->AddRectFilled(healthBarPos,
                            ImVec2(healthBarPos.x + (healthBarSize.x * healthRatio), healthBarPos.y + healthBarSize.y),
                            IM_COL32(230, 40, 40, 255));

    // 3. Status text window
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(WINDOW_WIDTH), 120), ImGuiCond_Always);
    ImGui::Begin("Player Status", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs |
                     ImGuiWindowFlags_NoMove);

    ImGui::SetCursorPos(ImVec2(static_cast<float>(WINDOW_WIDTH) - 70, 4));
    ImGui::TextColored(ImVec4(1, 1, 1, 1), "LV %d", player.GetLevel());

    int timeMinutes = static_cast<int>(gameTimeMs / 60000.0f) % 60;
    int timeSeconds = static_cast<int>(gameTimeMs / 1000.0f) % 60;
    char timeText[64];
    std::snprintf(timeText, sizeof(timeText), "%02d:%02d", timeMinutes, timeSeconds);

    ImGui::SetWindowFontScale(2.5f);
    ImVec2 timeTextSize = ImGui::CalcTextSize(timeText);
    ImGui::SetCursorPos(ImVec2((static_cast<float>(WINDOW_WIDTH) - timeTextSize.x) * 0.5f, 30));
    ImGui::Text("%s", timeText);
    ImGui::SetWindowFontScale(1.0f);

    char killText[64];
    std::snprintf(killText, sizeof(killText), "%d", defeated);
    ImVec2 killTextSize = ImGui::CalcTextSize(killText);
    float killX = static_cast<float>(WINDOW_WIDTH) - killTextSize.x - 45.0f;
    ImGui::SetCursorPos(ImVec2(killX, 35));
    ImGui::Text("%s", killText);
    ImGui::SameLine();
    if (m_EnemyCountIconImage && m_EnemyCountIconImage->GetTextureId() != 0) {
        ImGui::SetCursorPosY(31);
        ImGui::Image((void*)(intptr_t)m_EnemyCountIconImage->GetTextureId(), ImVec2(30, 30));
    }

    if (Util::Input::IsKeyUp(Util::Keycode::P)) {
        pauseRequested = true;
    }

    ImGui::End();

    // 4. Pause button
    ImGui::SetNextWindowPos(ImVec2(static_cast<float>(WINDOW_WIDTH) - 60, 75), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(50, 50), ImGuiCond_Always);
    ImGui::Begin("PauseUI", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);

    if (m_PauseIconImage && m_PauseIconImage->GetTextureId() != 0) {
        ImGui::Image((void*)(intptr_t)m_PauseIconImage->GetTextureId(), ImVec2(32, 32));
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
            pauseRequested = true;
        }
    } else {
        if (ImGui::Button("||", ImVec2(40, 40))) {
            pauseRequested = true;
        }
    }
    ImGui::End();
}

void UIManager::DrawStatsTables(const Player& player, const WeaponManager& weaponManager,
                                int defeated, int wave, int stage, float gameTimeMs,
                                float leftY, float rightY, float panelHeight) {
    auto whip = weaponManager.GetWhip();
    auto knife = weaponManager.GetKnife();
    auto rune = weaponManager.GetRunetracer();

    // Left panel
    ImGui::SetCursorPos(ImVec2(20, leftY));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.0f);

    ImGui::BeginChild("LeftStatsPanel", ImVec2(280, panelHeight), true);
    ImGui::SetWindowFontScale(1.5f);
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Game Stats");
    ImGui::Separator();
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Dummy(ImVec2(0, 10));

    int timeMinutes = static_cast<int>(gameTimeMs / 60000.0f) % 60;
    int timeSeconds = static_cast<int>(gameTimeMs / 1000.0f) % 60;

    if (ImGui::BeginTable("GameStatsTable", 2, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        auto drawStatRow = [](const char* name, const char* val) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", name);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", val);
        };
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%02d:%02d", timeMinutes, timeSeconds);
        drawStatRow("Time", buf);
        ImGui::Dummy(ImVec2(0, 10));
        std::snprintf(buf, sizeof(buf), "%d", defeated);
        drawStatRow("Defeated", buf);
        ImGui::Dummy(ImVec2(0, 10));
        std::snprintf(buf, sizeof(buf), "%d", player.GetLevel());
        drawStatRow("Level", buf);
        ImGui::Dummy(ImVec2(0, 10));
        std::snprintf(buf, sizeof(buf), "%d", wave);
        drawStatRow("Wave", buf);
        ImGui::Dummy(ImVec2(0, 10));
        std::snprintf(buf, sizeof(buf), "%d", stage);
        drawStatRow("Stage", buf);
        ImGui::EndTable();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    // Right panel
    ImGui::SetCursorPos(ImVec2(320, rightY));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.0f);

    ImGui::BeginChild("RightStatsPanel", ImVec2(static_cast<float>(WINDOW_WIDTH) - 340, panelHeight), true);
    ImGui::SetWindowFontScale(1.5f);
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Player Statistics");
    ImGui::Separator();
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Dummy(ImVec2(0, 10));

    if (ImGui::BeginTable("PlayerStatsTable", 2, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 180.0f);

        auto drawStatRow = [](const char* name, const std::string& val) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", name);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", val.c_str());
        };
        auto drawHeader = [](const char* name, bool locked) {
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

        drawStatRow("Max Health", std::to_string(static_cast<int>(player.GetMaxHealth())));
        if (player.IsArmorUnlocked()) {
            drawStatRow("Armor Reduction",
                        std::to_string(static_cast<int>(player.GetArmor() * 100)) + "% (Lv " +
                            std::to_string(player.GetArmorLevel()) + ")");
        } else {
            drawStatRow("Armor Reduction", "(Locked)");
        }
        if (player.IsVampirismUnlocked()) {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%.1f%% (Lv %d)", player.GetVampirism() * 100.0f, player.GetVampirismLevel());
            drawStatRow("Lifesteal", buf);
        } else {
            drawStatRow("Lifesteal", "(Locked)");
        }

        drawHeader("[ Whip ]", false);
        drawStatRow("Range", std::to_string(static_cast<int>(whip->GetHitRadiusRatio() * 100)) + "% (Lv " +
                                 std::to_string(whip->GetRangeLevel()) + ")");
        drawStatRow("Damage", std::to_string(static_cast<int>(whip->GetDamage())) + " (Lv " +
                                  std::to_string(whip->GetDamageLevel()) + ")");
        drawStatRow("Cooldown", std::to_string(static_cast<int>(whip->GetAttackIntervalMs())) + "ms (Lv " +
                                    std::to_string(whip->GetCooldownLevel()) + ")");

        if (knife->IsUnlocked()) {
            drawHeader("[ Knife ]", false);
            drawStatRow("Count", std::to_string(knife->GetCount()) + " (Lv " + std::to_string(knife->GetCountLevel()) + ")");
            drawStatRow("Damage", std::to_string(static_cast<int>(knife->GetDamage())) + " (Lv " +
                                      std::to_string(knife->GetDamageLevel()) + ")");
            drawStatRow("Cooldown", std::to_string(static_cast<int>(knife->GetAttackIntervalMs())) + "ms (Lv " +
                                        std::to_string(knife->GetCooldownLevel()) + ")");
        } else {
            drawHeader("[ Knife ] (Locked)", true);
        }

        if (rune->IsUnlocked()) {
            drawHeader("[ Runetracer ]", false);
            drawStatRow("Count", std::to_string(rune->GetCount()) + " (Lv " + std::to_string(rune->GetCountLevel()) + ")");
            drawStatRow("Damage", std::to_string(static_cast<int>(rune->GetDamage())) + " (Lv " +
                                      std::to_string(rune->GetDamageLevel()) + ")");
            drawStatRow("Cooldown", std::to_string(static_cast<int>(rune->GetAttackIntervalMs())) + "ms (Lv " +
                                        std::to_string(rune->GetCooldownLevel()) + ")");
        } else {
            drawHeader("[ Runetracer ] (Locked)", true);
        }

        ImGui::EndTable();
    }

    ImGui::SetWindowFontScale(1.0f);
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

void UIManager::DrawPausedMenu(const Player& player, const WeaponManager& weaponManager,
                               int defeated, int wave, int stage, float gameTimeMs,
                               bool& continueRequested, bool& exitRequested) {
    continueRequested = false;
    exitRequested = false;

    ImDrawList* bgDrawList = ImGui::GetBackgroundDrawList();
    bgDrawList->AddRectFilled(ImVec2(0, 0),
                              ImVec2(static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)),
                              IM_COL32(0, 0, 0, 150));

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)), ImGuiCond_Always);
    ImGui::Begin("PausedMenu", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);

    DrawStatsTables(player, weaponManager, defeated, wave, stage, gameTimeMs,
                    20, 20, static_cast<float>(WINDOW_HEIGHT) - 100);

    float buttonY = static_cast<float>(WINDOW_HEIGHT) - 70;
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);

    // Exit Button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
    ImGui::SetCursorPos(ImVec2(20, buttonY));
    if (ImGui::Button("EXIT", ImVec2(150, 40))) {
        exitRequested = true;
    }
    ImGui::PopStyleColor();

    // Continue Button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.3f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.2f, 1.0f));
    ImGui::SetCursorPos(ImVec2(static_cast<float>(WINDOW_WIDTH) - 220, buttonY));
    if (ImGui::Button("CONTINUE", ImVec2(200, 40)) || Util::Input::IsKeyUp(Util::Keycode::P)) {
        continueRequested = true;
    }
    ImGui::PopStyleColor(2);

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImGui::End();
}

void UIManager::DrawGameOverMenu(const Player& player, const WeaponManager& weaponManager,
                                 int defeated, int wave, int stage, float gameTimeMs,
                                 bool& restartRequested, bool& exitRequested) {
    restartRequested = false;
    exitRequested = false;

    ImDrawList* bgDrawList = ImGui::GetBackgroundDrawList();
    bgDrawList->AddRectFilled(ImVec2(0, 0),
                              ImVec2(static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)),
                              IM_COL32(150, 0, 0, 150));

    if (m_GameOverImage && m_GameOverImage->GetTextureId() != 0) {
        ImVec2 texSize(m_GameOverImage->GetSize().x, m_GameOverImage->GetSize().y);
        ImVec2 pos((static_cast<float>(WINDOW_WIDTH) - texSize.x) * 0.5f, 50.0f);
        bgDrawList->AddImage((void*)(intptr_t)m_GameOverImage->GetTextureId(), pos,
                             ImVec2(pos.x + texSize.x, pos.y + texSize.y));
    }

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)), ImGuiCond_Always);
    ImGui::Begin("GameOverMenu", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);

    DrawStatsTables(player, weaponManager, defeated, wave, stage, gameTimeMs,
                    250, 250, static_cast<float>(WINDOW_HEIGHT) - 350);

    float buttonY = static_cast<float>(WINDOW_HEIGHT) - 70;
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);

    // Exit Button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
    ImGui::SetCursorPos(ImVec2(20, buttonY));
    if (ImGui::Button("EXIT", ImVec2(150, 40))) {
        exitRequested = true;
    }
    ImGui::PopStyleColor();

    // Restart Button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.3f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.2f, 1.0f));
    ImGui::SetCursorPos(ImVec2(static_cast<float>(WINDOW_WIDTH) - 220, buttonY));
    if (ImGui::Button("RESTART", ImVec2(200, 40))) {
        restartRequested = true;
    }
    ImGui::PopStyleColor(2);

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImGui::End();
}

void UIManager::DrawLevelUpMenu(const std::vector<UpgradeType>& options,
                                const Player& player, const WeaponManager& weaponManager,
                                const std::function<void(UpgradeType)>& onSelectUpgrade) {
    if (m_LevelUpObject) {
        m_LevelUpObject->m_Transform.translation = {0.0f, 0.0f};
        m_LevelUpObject->Draw();
    }

    auto whip = weaponManager.GetWhip();
    auto knife = weaponManager.GetKnife();
    auto rune = weaponManager.GetRunetracer();

    ImGui::SetNextWindowPos(ImVec2(static_cast<float>(WINDOW_WIDTH) / 2.0f,
                                   static_cast<float>(WINDOW_HEIGHT) / 2.0f),
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.1f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.2f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("Level Up!", nullptr,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoTitleBar);

    ImGui::Dummy(ImVec2(0, 105));

    for (size_t i = 0; i < options.size(); ++i) {
        UpgradeType type = options[i];
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
                buttonText = "Whip Range Up (Lv " + std::to_string(whip->GetRangeLevel() + 1) + ")";
                break;
            case UpgradeType::WHIP_DAMAGE:
                buttonText = "Whip Damage Up (Lv " + std::to_string(whip->GetDamageLevel() + 1) + ")";
                break;
            case UpgradeType::WHIP_COOLDOWN:
                buttonText = "Whip Cooldown Down (Lv " + std::to_string(whip->GetCooldownLevel() + 1) + ")";
                break;
            case UpgradeType::KNIFE_COUNT:
                buttonText = "Knife Count +2 (Lv " + std::to_string(knife->GetCountLevel() + 1) + ")";
                break;
            case UpgradeType::KNIFE_DAMAGE:
                buttonText = "Knife Damage Up (Lv " + std::to_string(knife->GetDamageLevel() + 1) + ")";
                break;
            case UpgradeType::KNIFE_COOLDOWN:
                buttonText = "Knife Cooldown Down (Lv " + std::to_string(knife->GetCooldownLevel() + 1) + ")";
                break;
            case UpgradeType::RUNETRACER_COUNT:
                buttonText = "Runetracer Count +1 (Lv " + std::to_string(rune->GetCountLevel() + 1) + ")";
                break;
            case UpgradeType::RUNETRACER_DAMAGE:
                buttonText = "Runetracer Damage Up (Lv " + std::to_string(rune->GetDamageLevel() + 1) + ")";
                break;
            case UpgradeType::RUNETRACER_COOLDOWN:
                buttonText = "Runetracer Cooldown Down (Lv " + std::to_string(rune->GetCooldownLevel() + 1) + ")";
                break;
            case UpgradeType::MAX_HEALTH:
                buttonText = "Max Health +100";
                break;
            case UpgradeType::ARMOR:
                buttonText = "Armor +5% (Lv " + std::to_string(player.GetArmorLevel() + 1) + ")";
                break;
            case UpgradeType::VAMPIRISM:
                buttonText = "Lifesteal +0.5% (Lv " + std::to_string(player.GetVampirismLevel() + 1) + ")";
                break;
        }

        if (ImGui::Button(buttonText.c_str(), ImVec2(380, 80))) {
            onSelectUpgrade(type);
        }
        ImGui::Dummy(ImVec2(0, 30));
    }

    ImGui::End();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
}
