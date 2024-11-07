module;

#include <imgui.h>
#include<string>
#include<iostream>

export module Main;

import World;
import GameLoop;
import ConsoleComponent;
import WindowManager;
import WindowSettings;
import Input;
import EventSystem;

export int main()
{
    ImGui::CreateContext();
    auto settings = GiiGa::WindowSettings{"GiiGa Engine", 1240, 720};
    auto window = GiiGa::WindowManager::CreateWindow(settings);
    GiiGa::Input input;
    input.Init(window);

    GiiGa::Game game_loop(window);
    
    //auto obj = GiiGa::World::CreateObject();

    //obj->CreateComponent<GiiGa::ConsoleComponent>();
    
    game_loop.Run();

    return 0;
}
