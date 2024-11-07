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
    GiiGa::Game game_loop;
    
    //auto obj = GiiGa::World::CreateObject();

    //obj->CreateComponent<GiiGa::ConsoleComponent>();
    
    auto settings = GiiGa::WindowSettings{"GiiGa Engine", 1240, 720};
    auto window = GiiGa::WindowManager::CreateWindow(settings);

    ImGui::CreateContext();
    GiiGa::Input::Init(window);

    EventDispatcher<int> test{};
    auto id = test.Register(
        [](int a) { 
            std::cout << a << std::endl;
        });

    test.Invoke(5);
    test.Unregister(id);
    test.Invoke(1);
    
    game_loop.Run();

    return 0;
}
