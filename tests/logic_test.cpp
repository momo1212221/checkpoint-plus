#include <array>
#include <cassert>
#include "logic.hpp"
int main() {
    std::array<unsigned char,2> r{0xff,0xff};
    assert(qol::pad_buttons(r.data()) == 0);
    assert(qol::combo_pressed(7,0,7));
    assert(!qol::combo_pressed(7,7,7));
}
