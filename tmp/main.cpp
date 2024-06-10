
#include <cstdint>
#include <iostream>
#include <bitset>

template<typename Tag, typename Base = std::uint32_t>
struct Handle final { Base x; };

template<typename Tag, typename Base>
std::uint16_t generation(Handle<Tag, Base> hnd) {
    return hnd.x >> 16;
}

template<typename Tag, typename Base>
std::uint16_t index(Handle<Tag, Base> hnd) {
    return hnd.x & (static_cast<std::uint16_t>(-1));
}


struct Shader;

struct Draw {
    Handle<Shader> shader;
};



int main() {
    Handle<Shader> h{0x12344565u};

    std::cout << h.x << "\n";
    std::cout << std::bitset<32>(h.x) << "\n";
    std::cout << generation(h) << '|' << index(h) << std::endl;
    std::cout << std::bitset<32>((static_cast<std::uint32_t>(-1) & 0xffff)) << "\n";
    return 0;
}
