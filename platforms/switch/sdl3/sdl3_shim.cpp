// The part of the SDL 3 stand-in (SDL3/SDL.h beside this) that is more than
// a renamed call.
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cstddef>
#include <list>
#include <string>

namespace chaotix {
extern const unsigned char font_pixel[];
extern const size_t font_pixel_len;
}

namespace {

// SDL 3's debug text is an 8x8 font. The embedded pixel face at 8 px has that
// cell, so the layouts made for the one (frontend/fps_counter.h) fit the
// other. Lines are kept as textures while they repeat: the FPS line changes a
// few times a second, the debug overlay's every frame.
struct Line {
    std::string text;
    SDL_Texture* texture;
    int w, h;
};

constexpr size_t kMaxLines = 48;

TTF_Font* debug_font() {
    static TTF_Font* font = [] () -> TTF_Font* {
        if (!TTF_WasInit() && !TTF_Init()) return nullptr;
        SDL_RWops* io = SDL_RWFromConstMem(chaotix::font_pixel, int(chaotix::font_pixel_len));
        TTF_Font* f = io ? TTF_OpenFontRW(io, 1, 8) : nullptr;
        if (f) TTF_SetFontHinting(f, TTF_HINTING_MONO);
        return f;
    }();
    return font;
}

} // namespace

void SDL_RenderDebugText(SDL_Renderer* renderer, float x, float y, const char* text) {
    static std::list<Line> lines;
    static SDL_Renderer* owner = nullptr;
    if (!renderer || !text || !*text) return;
    if (owner != renderer) {
        // Textures of a renderer that has gone were destroyed with it.
        lines.clear();
        owner = renderer;
    }
    auto it = lines.begin();
    for (; it != lines.end(); ++it)
        if (it->text == text) break;
    if (it == lines.end()) {
        TTF_Font* font = debug_font();
        if (!font) return;
        SDL_Surface* s = TTF_RenderUTF8_Solid(font, text, SDL_Color{255, 255, 255, 255});
        if (!s) return;
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, s);
        const int w = s->w, h = s->h;
        SDL_FreeSurface(s);
        if (!tex) return;
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(tex, SDL_ScaleModeNearest);
        lines.push_front(Line{text, tex, w, h});
        if (lines.size() > kMaxLines) {
            SDL_DestroyTexture(lines.back().texture);
            lines.pop_back();
        }
    } else if (it != lines.begin()) {
        lines.splice(lines.begin(), lines, it);
    }
    const Line& l = lines.front();
    Uint8 r, g, b, a;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
    SDL_SetTextureColorMod(l.texture, r, g, b);
    SDL_SetTextureAlphaMod(l.texture, a);
    const SDL_FRect dst{x, y, float(l.w), float(l.h)};
    SDL_RenderCopyF(renderer, l.texture, nullptr, &dst);
}
