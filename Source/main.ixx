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

export int main()
{
    GiiGa::Game game_loop;
    
    //auto obj = GiiGa::World::CreateObject();

    //obj->CreateComponent<GiiGa::ConsoleComponent>();
    
    auto settings = GiiGa::WindowSettings{"GiiGa Engine", 1240, 720};
    auto window = GiiGa::WindowManager::CreateWindow(settings);

    GiiGa::Input::Init(window);
    
    game_loop.Run();

    return 0;
}
