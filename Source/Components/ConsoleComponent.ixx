module;

#include <iostream>
#include <json/json.h>

export module ConsoleComponent;

import Component;
import Time;

namespace GiiGa
{
    export class ConsoleComponent final : public Component
    {
    public:
        ConsoleComponent(Json::Value json)
        {
            
        }
        
        void Tick(float dt) override
        {
            std::cout << time_sum_ << std::endl;
            time_sum_ += Time::GetDeltaTime();
        }

        void Init() override
        {
            
        }
        std::shared_ptr<Component> Clone() override
        {
            return std::make_shared<ConsoleComponent>(*this);
        }
        void Restore(const ::Json::Value&) override
        {
            
        }

    private:
        float time_sum_ = 0;
    };
}