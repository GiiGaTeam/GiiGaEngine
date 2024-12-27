export module ConsoleComponent;

import <iostream>;
import <json/json.h>;

import Component;
import Time;
import IWorldQuery;

namespace GiiGa
{
    export class ConsoleComponent final : public Component
    {
    public:
        ConsoleComponent(Json::Value json, bool roll_id = false):
            Component(json, roll_id)
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

        std::shared_ptr<IComponent> Clone() override
        {
            return std::make_shared<ConsoleComponent>(*this);
        }

        virtual void Restore(const ::Json::Value&) override
        {
        }

        Json::Value DerivedToJson() override
        {
            Json::Value json;

            json["Type"] = typeid(ConsoleComponent).name();

            json["Properties"] = Json::Value();

            return json;
        }

    private:
        float time_sum_ = 0;
    };
}
