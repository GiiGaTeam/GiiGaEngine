module;

#include<string>
#include<iostream>

export module Main;

import World;
import GameLoop;
import ConsoleComponent;
import WindowManager;
import WindowSettings;

export int main()
{
    GiiGa::Game game_loop;
    
    auto obj = GiiGa::World::CreateObject();

    obj->CreateComponent<GiiGa::ConsoleComponent>();
    
    auto settings = GiiGa::WindowSettings{"GiiGa Engine", 1240, 720};
    auto window = GiiGa::WindowManager::CreateWindow(settings);

    //game_loop.Run();

    return 0;
}
