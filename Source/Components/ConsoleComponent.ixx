module;

#include <iostream>

export module ConsoleComponent;

import Component;
import Time;

namespace GiiGa
{
    export class ConsoleComponent final : public Component
    {
    public:
        void Tick(float dt) override
        {
            std::cout << time_sum_ << std::endl;
            time_sum_ += Time::GetDeltaTime();
        }

    private:
        float time_sum_ = 0;
    };
}