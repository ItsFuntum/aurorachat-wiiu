#include "theme.h"

std::map<int, Theme> Themes = {
    {1, {{255, 255, 255, 255}, {0, 0, 0, 255}, "Aurora White"}},
    {2, {{73, 73, 73, 255}, {0, 0, 0, 255}, "Deep Gray"}},
    {3, {{0, 26, 242, 255}, {0, 0, 0, 255}, "Homeblue Chat"}},
    {4, {{0, 0, 0, 255}, {17, 255, 0, 255}, "Hackertron Style"}},
    {5, {{23, 27, 57, 255}, {255, 255, 255, 255}, "True Dark Mode"}},
    {6, {{0, 25, 117, 255}, {255, 189, 97, 255}, "Blurange"}},
    {7, {{255, 80, 80, 255}, {255, 255, 255, 255}, "Red Paint"}},
    {8, {{6, 0, 57, 255}, {255, 255, 255, 255}, "Deep Blue."}}
};

Theme current = Themes[1];
int currentTheme = 1;
