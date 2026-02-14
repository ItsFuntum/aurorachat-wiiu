#include "chat.h"
#include "font.h"

std::vector<ChatLine> chatLines;
int chatPosY = 0;

void AddChatLine(SDL_Renderer* renderer,
                 const std::string& text,
                 int fontSize,
                 SDL_Color color,
                 int maxWidth)
{
    ChatLine line;
    line.rawText = text;

    line.texture = DrawTextToTexture(
        renderer,
        text.c_str(),
        fontSize,
        color,
        maxWidth
    );

    if (!line.texture) return;

    int w, h;
    SDL_QueryTexture(line.texture, nullptr, nullptr, &w, &h);
    line.height = h;

    chatLines.push_back(line);
}

void DrawChatBuffer(SDL_Renderer* renderer,
                    int x,
                    int y)
{
    int drawY = y + chatPosY;

    for (auto& line : chatLines)
    {
        int w, h;
        SDL_QueryTexture(line.texture, nullptr, nullptr, &w, &h);

        SDL_Rect dst = { x, drawY, w, h };
        SDL_RenderCopy(renderer, line.texture, nullptr, &dst);

        drawY += h + 6; // spacing between messages
    }
}

void FreeChatTextures()
{
    for (auto& line : chatLines)
    {
        if (line.texture)
            SDL_DestroyTexture(line.texture);
    }

    chatLines.clear();
}

void RebuildChatTextures(SDL_Renderer* renderer,
                         int fontSize,
                         SDL_Color color,
                         int maxWidth)
{
    for (auto& line : chatLines)
    {
        if (line.texture)
            SDL_DestroyTexture(line.texture);

        line.texture = DrawTextToTexture(
            renderer,
            line.rawText.c_str(),
            fontSize,
            color,
            maxWidth
        );

        int w, h;
        SDL_QueryTexture(line.texture, nullptr, nullptr, &w, &h);
        line.height = h;
    }
}
