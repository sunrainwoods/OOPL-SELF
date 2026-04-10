#include "App.hpp"

#include "Core/Context.hpp"
#include <imgui.h>
#include <backends/imgui_impl_opengl3.h>

int main(int, char**) {
    auto context = Core::Context::GetInstance();
    App app;

    while (!context->GetExit()) {
        context->Setup();

        switch (app.GetCurrentState()) {
            case App::State::TITLE:
                app.UpdateTitle();
                break;

            case App::State::START:
                app.Start();
                break;

            case App::State::UPDATE:
                app.Update();
                break;
            
            case App::State::LEVEL_UP:
                app.UpdateLevelUp();
                break;

            case App::State::GAME_OVER:
                app.UpdateGameOver();
                break;

            case App::State::PAUSED:
                app.UpdatePaused();
                break;

            case App::State::END:
                app.End();
                context->SetExit(true);
                break;
        }
        
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        context->Update();
    }
    return 0;
}
