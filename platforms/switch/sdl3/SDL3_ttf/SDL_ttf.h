// SDL_ttf 3 names on top of SDL_ttf 2, for the Nintendo Switch (see
// ../SDL3/SDL.h). SDL_ttf 3 keeps a string as a TTF_Text that it draws through
// a text engine; here a TTF_Text is the string rendered once in white to a
// texture, which is tinted when it is drawn.
#pragma once

#include <SDL3/SDL.h>

#include <SDL2/SDL_ttf.h>

#include <string>

struct TTF_TextEngine {
    SDL_Renderer* renderer = nullptr;
};

struct TTF_Text {
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    int w = 0, h = 0;
    Uint8 r = 255, g = 255, b = 255, a = 255;
};

inline TTF_TextEngine* TTF_CreateRendererTextEngine(SDL_Renderer* renderer) {
    return renderer ? new TTF_TextEngine{renderer} : nullptr;
}
inline void TTF_DestroyRendererTextEngine(TTF_TextEngine* e) { delete e; }

inline TTF_Font* TTF_OpenFontIO(SDL_RWops* io, bool closeio, float ptsize) {
    return TTF_OpenFontRW(io, closeio ? 1 : 0, int(ptsize + 0.5f));
}
inline int TTF_GetFontHeight(const TTF_Font* font) { return TTF_FontHeight(font); }
inline bool c3_ttf_set_size(TTF_Font* font, float ptsize) { return (TTF_SetFontSize)(font, int(ptsize + 0.5f)) == 0; }
inline bool TTF_GetStringSize(TTF_Font* font, const char* text, size_t length, int* w, int* h) {
    return TTF_SizeUTF8(font, std::string(text, length).c_str(), w, h) == 0;
}

inline TTF_Text* TTF_CreateText(TTF_TextEngine* engine, TTF_Font* font, const char* text, size_t length) {
    if (!engine || !font) return nullptr;
    SDL_Surface* s = TTF_RenderUTF8_Blended(font, std::string(text, length).c_str(), SDL_Color{255, 255, 255, 255});
    if (!s) return nullptr;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(engine->renderer, s);
    const int w = s->w, h = s->h;
    SDL_FreeSurface(s);
    if (!tex) return nullptr;
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    // Text is always drawn at whole pixels and its own size.
    SDL_SetTextureScaleMode(tex, SDL_ScaleModeNearest);
    return new TTF_Text{engine->renderer, tex, w, h};
}
inline void TTF_DestroyText(TTF_Text* t) {
    if (!t) return;
    if (t->texture) SDL_DestroyTexture(t->texture);
    delete t;
}
inline bool TTF_SetTextColor(TTF_Text* t, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    if (!t) return false;
    t->r = r; t->g = g; t->b = b; t->a = a;
    return true;
}
inline bool TTF_GetTextSize(TTF_Text* t, int* w, int* h) {
    if (!t) return false;
    if (w) *w = t->w;
    if (h) *h = t->h;
    return true;
}
inline bool TTF_DrawRendererText(TTF_Text* t, float x, float y) {
    if (!t || !t->texture) return false;
    SDL_SetTextureColorMod(t->texture, t->r, t->g, t->b);
    SDL_SetTextureAlphaMod(t->texture, t->a);
    const SDL_FRect dst{x, y, float(t->w), float(t->h)};
    return SDL_RenderCopyF(t->renderer, t->texture, nullptr, &dst) == 0;
}

// Same names, other signatures.
#define TTF_Init() (TTF_Init() == 0)
#define TTF_SetFontSize(font, size) c3_ttf_set_size(font, size)
