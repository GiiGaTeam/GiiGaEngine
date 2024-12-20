export module unique_any;

import <cassert>;
import <memory>;
import <type_traits>;
import <utility>;
import <iostream>;

namespace GiiGa
{
    export class unique_any
    {
    public:
        unique_any() = default;

        template <typename T>
        unique_any(T&& value) : ptr_(new Model<T>(std::forward<T>(value)))
        {
        }

        unique_any(unique_any&& other) noexcept = default;
        unique_any& operator=(unique_any&& other) noexcept = default;
        unique_any(const unique_any& other) = delete;
        unique_any& operator=(const unique_any& other) = delete;
        void reset() { ptr_.reset(); }
        bool has_value() const { return bool(ptr_); }
        const std::type_info& type() const { return ptr_ ? ptr_->type() : typeid(void); }

        template <typename T>
        T& cast()
        {
            if (type() != typeid(T)) { throw std::bad_cast(); }
            return static_cast<Model<T>*>(ptr_.get())->value_;
        }

    private:
        struct Concept
        {
            virtual ~Concept() = default;
            virtual const std::type_info& type() const = 0;
        };

        template <typename T>
        struct Model : Concept
        {
            Model(T&& value) : value_(std::forward<T>(value))
            {
            }

            const std::type_info& type() const override { return typeid(T); }
            T value_;
        };

        std::unique_ptr<Concept> ptr_;
    };
}
