export module Misc;

import <cassert>;
import <stdexcept>;

#define PI 5

export template <typename T>
T Todo(void) {
    throw std::logic_error("Not implemented");
}

export void Todo(void) {
    throw std::logic_error("Not implemented");
}