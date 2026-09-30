#pragma once

#if defined(CHAOTIX_USE_SDL2) || defined(__SWITCH__)

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#if defined(__SWITCH__)
#include <switch.h>
#endif

// Controller & Gamepad types
using SDL_Gamepad = SDL_GameController;
using SDL_GamepadButton = SDL_GameControllerButton;
using SDL_GamepadAxis = SDL_GameControllerAxis;
using SDL_IOStream = SDL_RWops;

#ifndef SDL_PI_F
#define SDL_PI_F 3.14159265358979323846f
#endif

// Events
#define SDL_EVENT_QUIT                  SDL_QUIT
#define SDL_EVENT_KEY_DOWN              SDL_KEYDOWN
#define SDL_EVENT_KEY_UP                SDL_KEYUP
#define SDL_EVENT_FINGER_DOWN           SDL_FINGERDOWN
#define SDL_EVENT_FINGER_UP             SDL_FINGERUP
#define SDL_EVENT_FINGER_MOTION         SDL_FINGERMOTION
#define SDL_EVENT_MOUSE_BUTTON_DOWN     SDL_MOUSEBUTTONDOWN
#define SDL_EVENT_MOUSE_BUTTON_UP       SDL_MOUSEBUTTONUP
#define SDL_EVENT_MOUSE_MOTION          SDL_MOUSEMOTION
#define SDL_EVENT_DROP_FILE             SDL_DROPFILE
#define SDL_EVENT_WINDOW_FOCUS_LOST     SDL_WINDOWEVENT
#define SDL_EVENT_DID_ENTER_BACKGROUND  SDL_APP_DIDENTERBACKGROUND
#define SDL_EVENT_FINGER_CANCELED       0xFFFF

#define SDL_EVENT_GAMEPAD_ADDED         SDL_CONTROLLERDEVICEADDED
#define SDL_EVENT_GAMEPAD_REMOVED       SDL_CONTROLLERDEVICEREMOVED
#define SDL_EVENT_GAMEPAD_BUTTON_DOWN   SDL_CONTROLLERBUTTONDOWN
#define SDL_EVENT_GAMEPAD_BUTTON_UP     SDL_CONTROLLERBUTTONUP

#define gbutton cbutton
#define gdevice cdevice
#define fingerID fingerId

// Gamepad buttons mapping
#define SDL_GAMEPAD_BUTTON_INVALID        SDL_CONTROLLER_BUTTON_INVALID
#define SDL_GAMEPAD_BUTTON_SOUTH          SDL_CONTROLLER_BUTTON_A
#define SDL_GAMEPAD_BUTTON_EAST           SDL_CONTROLLER_BUTTON_B
#define SDL_GAMEPAD_BUTTON_WEST           SDL_CONTROLLER_BUTTON_X
#define SDL_GAMEPAD_BUTTON_NORTH          SDL_CONTROLLER_BUTTON_Y
#define SDL_GAMEPAD_BUTTON_BACK           SDL_CONTROLLER_BUTTON_BACK
#define SDL_GAMEPAD_BUTTON_GUIDE          SDL_CONTROLLER_BUTTON_GUIDE
#define SDL_GAMEPAD_BUTTON_START          SDL_CONTROLLER_BUTTON_START
#define SDL_GAMEPAD_BUTTON_LEFT_STICK     SDL_CONTROLLER_BUTTON_LEFTSTICK
#define SDL_GAMEPAD_BUTTON_RIGHT_STICK    SDL_CONTROLLER_BUTTON_RIGHTSTICK
#define SDL_GAMEPAD_BUTTON_LEFT_SHOULDER  SDL_CONTROLLER_BUTTON_LEFTSHOULDER
#define SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER SDL_CONTROLLER_BUTTON_RIGHTSHOULDER
#define SDL_GAMEPAD_BUTTON_DPAD_UP        SDL_CONTROLLER_BUTTON_DPAD_UP
#define SDL_GAMEPAD_BUTTON_DPAD_DOWN      SDL_CONTROLLER_BUTTON_DPAD_DOWN
#define SDL_GAMEPAD_BUTTON_DPAD_LEFT      SDL_CONTROLLER_BUTTON_DPAD_LEFT
#define SDL_GAMEPAD_BUTTON_DPAD_RIGHT     SDL_CONTROLLER_BUTTON_DPAD_RIGHT

// Gamepad axes mapping
#define SDL_GAMEPAD_AXIS_LEFTX            SDL_CONTROLLER_AXIS_LEFTX
#define SDL_GAMEPAD_AXIS_LEFTY            SDL_CONTROLLER_AXIS_LEFTY
#define SDL_GAMEPAD_AXIS_RIGHTX           SDL_CONTROLLER_AXIS_RIGHTX
#define SDL_GAMEPAD_AXIS_RIGHTY           SDL_CONTROLLER_AXIS_RIGHTY
#define SDL_GAMEPAD_AXIS_LEFT_TRIGGER     SDL_CONTROLLER_AXIS_TRIGGERLEFT
#define SDL_GAMEPAD_AXIS_RIGHT_TRIGGER    SDL_CONTROLLER_AXIS_TRIGGERRIGHT

// Window & Render constants
#define SDL_WINDOW_HIGH_PIXEL_DENSITY     SDL_WINDOW_ALLOW_HIGHDPI
#define SDL_SCALEMODE_LINEAR              SDL_ScaleModeLinear
#define SDL_SCALEMODE_NEAREST             SDL_ScaleModeNearest

// Renderer and Surface functions
#define SDL_GetRenderOutputSize           SDL_GetRendererOutputSize
#define SDL_SetRenderScale                SDL_RenderSetScale
inline int SDL_RenderTextureCompat(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_FRect* srcrect, const SDL_FRect* dstrect) {
    if (srcrect) {
        SDL_Rect src_int{int(srcrect->x), int(srcrect->y), int(srcrect->w), int(srcrect->h)};
        return SDL_RenderCopyF(renderer, texture, &src_int, dstrect);
    }
    return SDL_RenderCopyF(renderer, texture, nullptr, dstrect);
}
#define SDL_RenderTexture(r, tex, src, dst) SDL_RenderTextureCompat(r, tex, src, dst)
#define SDL_DestroySurface                SDL_FreeSurface
#define SDL_RenderFillRect(r, rect)       SDL_RenderFillRectF(r, rect)
#define SDL_ConvertSurface(s, fmt)        SDL_ConvertSurfaceFormat(s, fmt, 0)

inline SDL_Surface* SDL_RenderReadPixelsSurface(SDL_Renderer* renderer, const SDL_Rect* rect) {
    int w = 0, h = 0;
    SDL_GetRendererOutputSize(renderer, &w, &h);
    if (rect) { w = rect->w; h = rect->h; }
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!surface) return nullptr;
    if (::SDL_RenderReadPixels(renderer, rect, SDL_PIXELFORMAT_ARGB8888, surface->pixels, surface->pitch) != 0) {
        SDL_FreeSurface(surface);
        return nullptr;
    }
    return surface;
}
#define SDL_RenderReadPixels(r, rect) SDL_RenderReadPixelsSurface(r, rect)

inline SDL_Gamepad* SDL_OpenGamepad(int id) { return SDL_GameControllerOpen(id); }
inline void SDL_CloseGamepad(SDL_Gamepad* g) { if (g) SDL_GameControllerClose(g); }
inline const char* SDL_GetGamepadName(SDL_Gamepad* g) { return g ? SDL_GameControllerName(g) : ""; }
inline bool SDL_GetGamepadButton(SDL_Gamepad* g, SDL_GamepadButton b) { return g && (SDL_GameControllerGetButton(g, b) != 0); }
inline Sint16 SDL_GetGamepadAxis(SDL_Gamepad* g, SDL_GamepadAxis a) { return g ? SDL_GameControllerGetAxis(g, a) : 0; }
inline const char* SDL_GetGamepadStringForButton(SDL_GamepadButton b) { return SDL_GameControllerGetStringForButton(b); }
inline SDL_GamepadButton SDL_GetGamepadButtonFromString(const char* s) { return SDL_GameControllerGetButtonFromString(s); }
inline Sint32 SDL_GetGamepadID(SDL_Gamepad* g) {
    if (!g) return -1;
    SDL_Joystick* j = SDL_GameControllerGetJoystick(g);
    return j ? SDL_JoystickInstanceID(j) : -1;
}

inline void SDL_SetRenderVSync(SDL_Renderer* renderer, int vsync) { SDL_RenderSetVSync(renderer, vsync); }
inline void SDL_DelayPrecise(uint64_t ns) {
    uint32_t ms = uint32_t(ns / 1000000);
    if (ms > 0) SDL_Delay(ms);
}
inline const char* SDL_GetRendererName(SDL_Renderer* renderer) {
    static SDL_RendererInfo info;
    if (renderer && SDL_GetRendererInfo(renderer, &info) == 0) return info.name;
    return "unknown";
}
inline void SDL_RenderDebugText(SDL_Renderer*, float, float, const char*) {}

// IO Stream / RWops wrapper
#define SDL_IOFromConstMem(ptr, size) SDL_RWFromConstMem(ptr, (int)(size))
#define SDL_IOFromFile(file, mode) SDL_RWFromFile(file, mode)
inline size_t SDL_ReadIO(SDL_IOStream* io, void* ptr, size_t size) { return io ? SDL_RWread(io, ptr, 1, size) : 0; }
inline size_t SDL_WriteIO(SDL_IOStream* io, const void* ptr, size_t size) { return io ? SDL_RWwrite(io, ptr, 1, size) : 0; }
inline bool SDL_CloseIO(SDL_IOStream* io) { return io ? (SDL_RWclose(io) == 0) : true; }

#else

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#endif
