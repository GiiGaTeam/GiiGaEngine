module;

#include<string>
#include<iostream>

export module Main;

import World;
import GameLoop;
import ConsoleComponent;

export int main()
{
    GiiGa::Game game_loop;
    
    auto obj = GiiGa::World::Spawn();
            
    obj->CreateComponent<GiiGa::ConsoleComponent>();
    
    game_loop.Run();
    
    return 0;
}
