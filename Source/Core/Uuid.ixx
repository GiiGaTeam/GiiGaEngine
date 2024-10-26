module;

#include <string>
#include <stduuid/uuid.h>

export module Uuid;

namespace Giiga
{

export class Uuid
{
private:
    static constexpr size_t UUID_SIZE = 16;
    
    uuids::uuid uuid_;

    Uuid() : uuid_{} {}
    Uuid(uuids::uuid uuid) : uuid_(uuid) {}

public:
    Uuid(const Uuid &other) : uuid_(other.uuid_) {}
    Uuid& operator=(const Uuid &other) {
        uuid_ = other.uuid_;
        return *this;
    }

    Uuid(Uuid &&other) noexcept : uuid_(std::move(other.uuid_)) {}
    Uuid& operator=(Uuid &&other) noexcept {
        uuid_ = std::move(other.uuid_);
        return *this;
    }
    
    void swap(Uuid& other) noexcept {
        uuid_.swap(other.uuid_);
    }
    
    static Uuid New() {
        std::random_device rd;
        auto seed_data = std::array<int, std::mt19937::state_size> {};
        std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
        std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
        std::mt19937 generator(seq);
        uuids::uuid_random_generator gen{generator};
        
        return Uuid(gen());
    }
    
    static Uuid Null() {
        return Uuid(uuids::uuid());
    }

    static Uuid FromBytes(const std::array<uuids::uuid::value_type, UUID_SIZE>& bytes) {
        return Uuid(uuids::uuid(bytes));
    }
    
    static std::optional<Uuid> FromString(const std::string& str) {
        auto result = uuids::uuid::from_string(str);

        if (result.has_value()) {
            return Uuid(result.value());
        }
        
        return std::nullopt;
    }

    bool IsNull() const {
        return uuid_.is_nil();
    }
    
    std::string ToString() const {
        return std::string(uuids::to_string(uuid_));
    }

    bool operator==(const Uuid& other) const {
        return uuid_ == other.uuid_;
    }

    bool operator!=(const Uuid& other) const {
        return !(*this == other);
    }

    std::ostream& operator<<(std::ostream& os) const {
        os << ToString();
        return os;
    }

   std::istream& operator>>(std::istream& is) {
        std::array<uuids::uuid::value_type, UUID_SIZE> bytes;
        is.read(reinterpret_cast<char*>(bytes.data()), UUID_SIZE);

        if (is) {
            auto temp = FromBytes(bytes);
            std::swap(*this, temp);
        }

        return is;
    }
};

}