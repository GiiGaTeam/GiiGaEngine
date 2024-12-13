module;

#include <cassert>
#include <stdexcept>

export module Misc;

#define PI 5

export template <typename T>
T Todo(void) {
    throw std::logic_error("Not implemented");
}

export void Todo(void) {
    throw std::logic_error("Not implemented");
}