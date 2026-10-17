#pragma once
#include <SDL.h>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

class PixelFont {
public:
    bool load(const std::string& h12Path, const std::string& roboPath);
    int measure(std::string_view utf8) const;
    void draw(SDL_Renderer* r, int x, int y, std::string_view utf8, SDL_Color color) const;
    int measureFrontend(std::string_view utf8) const;
    void drawFrontend(SDL_Renderer* r, int x, int y, std::string_view utf8, SDL_Color color) const;

private:
    std::array<std::array<std::uint8_t, 32>, 360> h12_{};
    std::array<std::array<std::uint8_t, 16>, 256> robo_{};
    bool loaded_ = false;

    static bool nextUtf8(std::string_view s, std::size_t& pos, std::uint32_t& cp);
    static int widthOf(std::uint32_t cp);
    std::array<std::uint8_t,32> composeHangul(std::uint32_t cp) const;
    void drawAscii(SDL_Renderer* r, int x, int y, std::uint8_t c, SDL_Color color) const;
    void drawHangul(SDL_Renderer* r, int x, int y, const std::array<std::uint8_t,32>& g, SDL_Color color) const;
    bool drawCompatJamo(SDL_Renderer* r, int x, int y, std::uint32_t cp, SDL_Color color) const;
};