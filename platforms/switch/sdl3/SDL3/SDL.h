// SDL 3 names on top of SDL 2, for the Nintendo Switch.
//
// devkitPro ships SDL 2 for the Switch and no SDL 3. The frontend is written
// against SDL 3, so this header is put ahead of the real one on the include
// path (CMakeLists.txt) and gives the part of SDL 3 the frontend uses, in
// terms of SDL 2. Nothing else in the tree knows about it: src/platform
// includes <SDL3/SDL.h> as it does everywhere.
//
// Only what the frontend calls is here. Where SDL 3 changed a function's
// signature but kept its name, a macro at the end redirects the name to a
// c3_ function; where it is a new name, the function is simply defined.
#pragma once

// SDL_ttf 2 calls the font `struct _TTF_Font`; the frontend forward-declares
// `struct TTF_Font`. One name for both.
#define TTF_Font _TTF_Font

#include <SDL2/SDL.h>

#include <cstdint>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

#if defined(__SWITCH__)
#include <switch.h>
#endif

// ---------------------------------------------------------------------------
// Types and constants that only changed their names
// ---------------------------------------------------------------------------
using SDL_Gamepad = SDL_GameController;
using SDL_GamepadButton = SDL_GameControllerButton;
using SDL_GamepadAxis = SDL_GameControllerAxis;
using SDL_GamepadAxisEvent = SDL_ControllerAxisEvent;
using SDL_DisplayID = int;
#define SDL_IOStream SDL_RWops

#ifndef SDL_PI_F
#define SDL_PI_F 3.14159265358979323846f
#endif

#define SDL_INIT_GAMEPAD SDL_INIT_GAMECONTROLLER

#define SDLK_R SDLK_r
#define SDLK_O SDLK_o
#define SDL_KMOD_ALT KMOD_ALT
#define SDL_KMOD_NONE KMOD_NONE

#define SDL_GAMEPAD_BUTTON_INVALID SDL_CONTROLLER_BUTTON_INVALID
#define SDL_GAMEPAD_BUTTON_SOUTH SDL_CONTROLLER_BUTTON_A
#define SDL_GAMEPAD_BUTTON_EAST SDL_CONTROLLER_BUTTON_B
#define SDL_GAMEPAD_BUTTON_WEST SDL_CONTROLLER_BUTTON_X
#define SDL_GAMEPAD_BUTTON_NORTH SDL_CONTROLLER_BUTTON_Y
#define SDL_GAMEPAD_BUTTON_BACK SDL_CONTROLLER_BUTTON_BACK
#define SDL_GAMEPAD_BUTTON_GUIDE SDL_CONTROLLER_BUTTON_GUIDE
#define SDL_GAMEPAD_BUTTON_START SDL_CONTROLLER_BUTTON_START
#define SDL_GAMEPAD_BUTTON_LEFT_STICK SDL_CONTROLLER_BUTTON_LEFTSTICK
#define SDL_GAMEPAD_BUTTON_RIGHT_STICK SDL_CONTROLLER_BUTTON_RIGHTSTICK
#define SDL_GAMEPAD_BUTTON_LEFT_SHOULDER SDL_CONTROLLER_BUTTON_LEFTSHOULDER
#define SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER SDL_CONTROLLER_BUTTON_RIGHTSHOULDER
#define SDL_GAMEPAD_BUTTON_DPAD_UP SDL_CONTROLLER_BUTTON_DPAD_UP
#define SDL_GAMEPAD_BUTTON_DPAD_DOWN SDL_CONTROLLER_BUTTON_DPAD_DOWN
#define SDL_GAMEPAD_BUTTON_DPAD_LEFT SDL_CONTROLLER_BUTTON_DPAD_LEFT
#define SDL_GAMEPAD_BUTTON_DPAD_RIGHT SDL_CONTROLLER_BUTTON_DPAD_RIGHT

#define SDL_GAMEPAD_AXIS_INVALID SDL_CONTROLLER_AXIS_INVALID
#define SDL_GAMEPAD_AXIS_LEFTX SDL_CONTROLLER_AXIS_LEFTX
#define SDL_GAMEPAD_AXIS_LEFTY SDL_CONTROLLER_AXIS_LEFTY
#define SDL_GAMEPAD_AXIS_RIGHTX SDL_CONTROLLER_AXIS_RIGHTX
#define SDL_GAMEPAD_AXIS_RIGHTY SDL_CONTROLLER_AXIS_RIGHTY
#define SDL_GAMEPAD_AXIS_LEFT_TRIGGER SDL_CONTROLLER_AXIS_TRIGGERLEFT
#define SDL_GAMEPAD_AXIS_RIGHT_TRIGGER SDL_CONTROLLER_AXIS_TRIGGERRIGHT

#define SDL_WINDOW_HIGH_PIXEL_DENSITY SDL_WINDOW_ALLOW_HIGHDPI
#define SDL_SCALEMODE_LINEAR SDL_ScaleModeLinear
#define SDL_SCALEMODE_NEAREST SDL_ScaleModeNearest
#define SDL_THREAD_PRIORITY_LOW SDL_THREAD_PRIORITY_LOW

#define SDL_GetRenderOutputSize SDL_GetRendererOutputSize
#define SDL_SetRenderScale SDL_RenderSetScale
#define SDL_SetRenderClipRect SDL_RenderSetClipRect
#define SDL_DestroySurface SDL_FreeSurface
#define SDL_SetCurrentThreadPriority SDL_SetThreadPriority

struct SDL_FColor {
    float r, g, b, a;
};

// ---------------------------------------------------------------------------
// Events. SDL 3 renamed the fields the frontend reads, so SDL 2's events are
// copied into a struct with SDL 3's names as they are polled.
// ---------------------------------------------------------------------------
enum : Uint32 {
    SDL_EVENT_QUIT = SDL_QUIT,
    SDL_EVENT_TERMINATING = SDL_APP_TERMINATING,
    SDL_EVENT_WILL_ENTER_BACKGROUND = SDL_APP_WILLENTERBACKGROUND,
    SDL_EVENT_DID_ENTER_BACKGROUND = SDL_APP_DIDENTERBACKGROUND,
    SDL_EVENT_DID_ENTER_FOREGROUND = SDL_APP_DIDENTERFOREGROUND,
    SDL_EVENT_KEY_DOWN = SDL_KEYDOWN,
    SDL_EVENT_KEY_UP = SDL_KEYUP,
    SDL_EVENT_TEXT_INPUT = SDL_TEXTINPUT,
    SDL_EVENT_MOUSE_MOTION = SDL_MOUSEMOTION,
    SDL_EVENT_MOUSE_BUTTON_DOWN = SDL_MOUSEBUTTONDOWN,
    SDL_EVENT_MOUSE_BUTTON_UP = SDL_MOUSEBUTTONUP,
    SDL_EVENT_GAMEPAD_AXIS_MOTION = SDL_CONTROLLERAXISMOTION,
    SDL_EVENT_GAMEPAD_BUTTON_DOWN = SDL_CONTROLLERBUTTONDOWN,
    SDL_EVENT_GAMEPAD_BUTTON_UP = SDL_CONTROLLERBUTTONUP,
    SDL_EVENT_GAMEPAD_ADDED = SDL_CONTROLLERDEVICEADDED,
    SDL_EVENT_GAMEPAD_REMOVED = SDL_CONTROLLERDEVICEREMOVED,
    SDL_EVENT_FINGER_DOWN = SDL_FINGERDOWN,
    SDL_EVENT_FINGER_UP = SDL_FINGERUP,
    SDL_EVENT_FINGER_MOTION = SDL_FINGERMOTION,
    SDL_EVENT_DROP_FILE = SDL_DROPFILE,
    // No SDL 2 event of their own.
    SDL_EVENT_WINDOW_FOCUS_LOST = SDL_USEREVENT + 0x1000,
    SDL_EVENT_FINGER_CANCELED,
    C3_EVENT_IGNORED,
};

struct C3_KeyboardEvent {
    SDL_Scancode scancode = SDL_SCANCODE_UNKNOWN;
    SDL_Keycode key = SDLK_UNKNOWN;
    Uint16 mod = 0;
    bool down = false;
    bool repeat = false;
};

struct C3_Event {
    Uint32 type = 0;
    C3_KeyboardEvent key;
    struct { const char* text = ""; } text;
    struct { SDL_JoystickID which = 0; Uint8 button = 0; } gbutton;
    SDL_ControllerAxisEvent gaxis{};
    // GAMEPAD_ADDED: SDL 2's device index, which is what c3 SDL_OpenGamepad
    // takes. GAMEPAD_REMOVED: the instance id, as in SDL 3.
    struct { SDL_JoystickID which = 0; } gdevice;
    struct { SDL_FingerID fingerID = 0; float x = 0, y = 0; } tfinger;
    struct { float x = 0, y = 0; Uint8 button = 0; } button;
    struct { float x = 0, y = 0; } motion;
    struct { const char* data = nullptr; } drop;
};

namespace c3 {

inline std::deque<C3_Event>& pushed_events() {
    static std::deque<C3_Event> q;
    return q;
}

// Text an event points at; good until the next event is polled, which is as
// long as the frontend keeps it.
inline std::string& event_text() {
    static std::string s;
    return s;
}

inline void convert_event(const ::SDL_Event& in, C3_Event& out) {
    out = C3_Event{};
    out.type = in.type;
    switch (in.type) {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        out.key.scancode = in.key.keysym.scancode;
        out.key.key = in.key.keysym.sym;
        out.key.mod = in.key.keysym.mod;
        out.key.down = in.type == SDL_KEYDOWN;
        out.key.repeat = in.key.repeat != 0;
        break;
    case SDL_TEXTINPUT:
        event_text() = in.text.text;
        out.text.text = event_text().c_str();
        break;
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP:
        out.gbutton.which = in.cbutton.which;
        out.gbutton.button = in.cbutton.button;
        break;
    case SDL_CONTROLLERAXISMOTION:
        out.gaxis = in.caxis;
        break;
    case SDL_CONTROLLERDEVICEADDED:
    case SDL_CONTROLLERDEVICEREMOVED:
        out.gdevice.which = in.cdevice.which;
        break;
    case SDL_FINGERDOWN:
    case SDL_FINGERUP:
    case SDL_FINGERMOTION:
        out.tfinger.fingerID = in.tfinger.fingerId;
        out.tfinger.x = in.tfinger.x;
        out.tfinger.y = in.tfinger.y;
        break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        out.button.x = float(in.button.x);
        out.button.y = float(in.button.y);
        out.button.button = in.button.button;
        break;
    case SDL_MOUSEMOTION:
        out.motion.x = float(in.motion.x);
        out.motion.y = float(in.motion.y);
        break;
    case SDL_DROPFILE:
        event_text() = in.drop.file ? in.drop.file : "";
        if (in.drop.file) SDL_free(in.drop.file);
        out.drop.data = event_text().c_str();
        break;
    case SDL_WINDOWEVENT:
        out.type = in.window.event == SDL_WINDOWEVENT_FOCUS_LOST ? Uint32(SDL_EVENT_WINDOW_FOCUS_LOST)
                                                                  : Uint32(C3_EVENT_IGNORED);
        break;
    default:
        break;
    }
}

inline bool PollEvent(C3_Event* out) {
    auto& q = pushed_events();
    if (!q.empty()) {
        *out = q.front();
        q.pop_front();
        return true;
    }
    ::SDL_Event e;
    if (!::SDL_PollEvent(&e)) return false;
    convert_event(e, *out);
    return true;
}

inline bool PushEvent(const C3_Event* e) {
    pushed_events().push_back(*e);
    return true;
}

// ---------------------------------------------------------------------------
// Video
// ---------------------------------------------------------------------------
inline SDL_Window* CreateWindow(const char* title, int w, int h, Uint32 flags) {
#if defined(__SWITCH__)
    // The console's screen, whatever size the desktop build would open at.
    (void)w; (void)h; (void)flags;
    return ::SDL_CreateWindow(title, 0, 0, 1280, 720, SDL_WINDOW_FULLSCREEN);
#else
    return ::SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, flags);
#endif
}

inline SDL_Renderer* CreateRenderer(SDL_Window* window, const char*) {
    return ::SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
}

inline SDL_Surface* RenderReadPixels(SDL_Renderer* renderer, const SDL_Rect* rect) {
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

inline SDL_Rect to_rect(const SDL_FRect& r) { return SDL_Rect{int(r.x), int(r.y), int(r.w), int(r.h)}; }

inline bool RenderTexture(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_FRect* src, const SDL_FRect* dst) {
    if (!src) return SDL_RenderCopyF(renderer, texture, nullptr, dst) == 0;
    const SDL_Rect s = to_rect(*src);
    return SDL_RenderCopyF(renderer, texture, &s, dst) == 0;
}

inline bool RenderTextureRotated(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_FRect* src,
                                 const SDL_FRect* dst, double angle, const SDL_FPoint* center, SDL_RendererFlip flip) {
    if (!src) return SDL_RenderCopyExF(renderer, texture, nullptr, dst, angle, center, flip) == 0;
    const SDL_Rect s = to_rect(*src);
    return SDL_RenderCopyExF(renderer, texture, &s, dst, angle, center, flip) == 0;
}

struct Vertex {
    SDL_FPoint position;
    SDL_FColor color;
    SDL_FPoint tex_coord;
};

inline Uint8 to_byte(float v) { return Uint8(v <= 0 ? 0 : v >= 1 ? 255 : int(v * 255.0f + 0.5f)); }

inline bool RenderGeometry(SDL_Renderer* renderer, SDL_Texture* texture, const Vertex* v, int n, const int* idx, int ni) {
    std::vector<::SDL_Vertex> out(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        out[size_t(i)].position = v[i].position;
        out[size_t(i)].color = SDL_Color{to_byte(v[i].color.r), to_byte(v[i].color.g), to_byte(v[i].color.b),
                                         to_byte(v[i].color.a)};
        out[size_t(i)].tex_coord = v[i].tex_coord;
    }
    return ::SDL_RenderGeometry(renderer, texture, out.data(), n, idx, ni) == 0;
}

inline SDL_Surface* CreateSurfaceFrom(int w, int h, Uint32 format, void* pixels, int pitch) {
    return SDL_CreateRGBSurfaceWithFormatFrom(pixels, w, h, 32, pitch, format);
}

inline const char* GetRendererName(SDL_Renderer* renderer) {
    static SDL_RendererInfo info;
    if (renderer && SDL_GetRendererInfo(renderer, &info) == 0) return info.name;
    return "unknown";
}

// Display modes: SDL 3 hands out pointers, and its refresh rate is a float.
struct DisplayMode {
    int w = 0, h = 0;
    float refresh_rate = 0;
};

inline const DisplayMode* current_mode(int display, bool desktop) {
    static DisplayMode out;
    ::SDL_DisplayMode m;
    if ((desktop ? ::SDL_GetDesktopDisplayMode(display, &m) : ::SDL_GetCurrentDisplayMode(display, &m)) != 0)
        return nullptr;
    out = DisplayMode{m.w, m.h, float(m.refresh_rate)};
    return &out;
}

// ---------------------------------------------------------------------------
// Audio. SDL 3's stream takes samples at the machine's rate, with a gain and
// a rate ratio that can change at any time; here that is a queue on an SDL 2
// device, with the gain and the ratio applied as the samples go in.
// ---------------------------------------------------------------------------
struct AudioSpec {
    SDL_AudioFormat format;
    int channels;
    int freq;
};

struct AudioStream {
    SDL_AudioDeviceID dev = 0;
    float gain = 1.0f;
    double ratio = 1.0;
    double phase = 0;          // position of the next output frame, in input frames
    int16_t prev[2] = {0, 0};  // the last input frame of the previous call
    std::vector<int16_t> out;
};

inline AudioStream* OpenAudioDeviceStream(const AudioSpec* spec) {
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = spec->freq;
    want.format = spec->format;
    want.channels = Uint8(spec->channels);
    want.samples = 1024;
    const SDL_AudioDeviceID dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (!dev) return nullptr;
    auto* s = new AudioStream;
    s->dev = dev;
    return s;
}

// 16-bit stereo, which is all the frontend plays.
inline bool PutAudioStreamData(AudioStream* s, const void* data, int bytes) {
    if (!s || bytes <= 0) return false;
    const int16_t* in = static_cast<const int16_t*>(data);
    const int frames = bytes / 4;
    if (!frames) return true;
    s->out.clear();
    const double step = s->ratio > 0.01 ? s->ratio : 1.0;
    // Frame i of this call sits at position i + 1; position 0 is `prev`.
    while (s->phase < double(frames)) {
        const int i = int(s->phase);
        const float f = float(s->phase - double(i));
        for (int ch = 0; ch < 2; ++ch) {
            const float a = float(i == 0 ? s->prev[ch] : in[(i - 1) * 2 + ch]);
            const float b = float(in[i * 2 + ch]);
            const float v = (a + (b - a) * f) * s->gain;
            s->out.push_back(int16_t(v > 32767.0f ? 32767 : v < -32768.0f ? -32768 : int(v)));
        }
        s->phase += step;
    }
    s->phase -= double(frames);
    s->prev[0] = in[(frames - 1) * 2];
    s->prev[1] = in[(frames - 1) * 2 + 1];
    if (s->out.empty()) return true;
    return SDL_QueueAudio(s->dev, s->out.data(), Uint32(s->out.size() * sizeof(int16_t))) == 0;
}

} // namespace c3

#define SDL_Event C3_Event
#define SDL_KeyboardEvent C3_KeyboardEvent
#define SDL_PollEvent(e) c3::PollEvent(e)
#define SDL_PushEvent(e) c3::PushEvent(e)
#define SDL_Vertex c3::Vertex
#define SDL_DisplayMode c3::DisplayMode
#define SDL_AudioSpec c3::AudioSpec
#define SDL_AudioStream c3::AudioStream
#define SDL_AUDIO_S16 AUDIO_S16SYS
#define SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK 0

#define SDL_Init(flags) (SDL_Init(flags) == 0)
#define SDL_CreateWindow(title, w, h, flags) c3::CreateWindow(title, w, h, flags)
#define SDL_CreateRenderer(window, name) c3::CreateRenderer(window, name)
#define SDL_RenderReadPixels(r, rect) c3::RenderReadPixels(r, rect)
#define SDL_RenderTexture(r, tex, src, dst) c3::RenderTexture(r, tex, src, dst)
#define SDL_RenderTextureRotated(r, tex, src, dst, angle, center, flip) \
    c3::RenderTextureRotated(r, tex, src, dst, angle, center, flip)
#define SDL_RenderGeometry(r, tex, v, n, idx, ni) c3::RenderGeometry(r, tex, v, n, idx, ni)
#define SDL_RenderFillRect(r, rect) SDL_RenderFillRectF(r, rect)
#define SDL_ConvertSurface(s, fmt) SDL_ConvertSurfaceFormat(s, fmt, 0)
#define SDL_CreateSurfaceFrom(w, h, fmt, pixels, pitch) c3::CreateSurfaceFrom(w, h, fmt, pixels, pitch)
#define SDL_GetRendererName(r) c3::GetRendererName(r)
#define SDL_SetRenderVSync(r, v) SDL_RenderSetVSync(r, v)
#define SDL_SetWindowFullscreen(w, on) SDL_SetWindowFullscreen(w, (on) ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0)
#define SDL_SetWindowBordered(w, on) SDL_SetWindowBordered(w, (on) ? SDL_TRUE : SDL_FALSE)
#define SDL_GetTicks() SDL_GetTicks64()
#define SDL_GetKeyFromScancode(sc, mod, events) SDL_GetKeyFromScancode(sc)
#define SDL_GetScancodeFromKey(key, mod) SDL_GetScancodeFromKey(key)
#if defined(__SWITCH__)
// The console's own keyboard would take the screen over; the menu's text rows
// are typed with the D-pad there, as on any pad.
#define SDL_StartTextInput(window) ((void)(window))
#define SDL_StopTextInput(window) ((void)(window))
#else
#define SDL_StartTextInput(window) SDL_StartTextInput()
#define SDL_StopTextInput(window) SDL_StopTextInput()
#endif

inline Uint64 SDL_GetTicksNS() {
    return Uint64(double(SDL_GetPerformanceCounter()) * 1e9 / double(SDL_GetPerformanceFrequency()));
}

inline void SDL_DelayPrecise(Uint64 ns) {
#if defined(__SWITCH__)
    svcSleepThread(s64(ns));
#else
    SDL_Delay(Uint32(ns / 1000000));
#endif
}

// SDL 2 keeps its keyboard state as bytes holding 0 or 1.
inline const bool* c3_keyboard_state() { return reinterpret_cast<const bool*>(::SDL_GetKeyboardState(nullptr)); }
#define SDL_GetKeyboardState(n) c3_keyboard_state()

// Displays
inline SDL_DisplayID SDL_GetDisplayForWindow(SDL_Window* w) { return SDL_GetWindowDisplayIndex(w); }
#define SDL_GetDesktopDisplayMode(d) c3::current_mode(d, true)
#define SDL_GetCurrentDisplayMode(d) c3::current_mode(d, false)
inline bool SDL_GetClosestFullscreenDisplayMode(SDL_DisplayID, int, int, float, bool, c3::DisplayMode*) { return false; }
inline bool SDL_SetWindowFullscreenMode(SDL_Window*, const c3::DisplayMode*) { return true; }

// Gamepads
inline SDL_Gamepad* SDL_OpenGamepad(SDL_JoystickID device_index) { return SDL_GameControllerOpen(int(device_index)); }
// The gamepads connected now, as what SDL_OpenGamepad takes (SDL 2's device
// indices); SDL_free the list.
inline SDL_JoystickID* SDL_GetGamepads(int* count) {
    const int n = SDL_NumJoysticks();
    auto* ids = static_cast<SDL_JoystickID*>(SDL_malloc(sizeof(SDL_JoystickID) * size_t(n > 0 ? n : 1)));
    int k = 0;
    for (int i = 0; ids && i < n; ++i)
        if (SDL_IsGameController(i)) ids[k++] = SDL_JoystickID(i);
    if (count) *count = k;
    return ids;
}
inline void SDL_CloseGamepad(SDL_Gamepad* g) { if (g) SDL_GameControllerClose(g); }
inline const char* SDL_GetGamepadName(SDL_Gamepad* g) { return g ? SDL_GameControllerName(g) : ""; }
inline bool SDL_GetGamepadButton(SDL_Gamepad* g, SDL_GamepadButton b) { return g && SDL_GameControllerGetButton(g, b) != 0; }
inline Sint16 SDL_GetGamepadAxis(SDL_Gamepad* g, SDL_GamepadAxis a) { return g ? SDL_GameControllerGetAxis(g, a) : Sint16(0); }
inline const char* SDL_GetGamepadStringForButton(SDL_GamepadButton b) { return SDL_GameControllerGetStringForButton(b); }
inline SDL_GamepadButton SDL_GetGamepadButtonFromString(const char* s) { return SDL_GameControllerGetButtonFromString(s); }
inline const char* SDL_GetGamepadStringForAxis(SDL_GamepadAxis a) { return SDL_GameControllerGetStringForAxis(a); }
inline SDL_GamepadAxis SDL_GetGamepadAxisFromString(const char* s) { return SDL_GameControllerGetAxisFromString(s); }
inline SDL_JoystickID SDL_GetGamepadID(SDL_Gamepad* g) {
    SDL_Joystick* j = g ? SDL_GameControllerGetJoystick(g) : nullptr;
    return j ? SDL_JoystickInstanceID(j) : -1;
}

// Streams
enum SDL_IOStatus { SDL_IO_STATUS_READY, SDL_IO_STATUS_ERROR, SDL_IO_STATUS_EOF, SDL_IO_STATUS_NOT_READY };
#define SDL_IOFromConstMem(ptr, size) SDL_RWFromConstMem(ptr, int(size))
#define SDL_IOFromFile(file, mode) SDL_RWFromFile(file, mode)
inline size_t SDL_ReadIO(SDL_RWops* io, void* ptr, size_t size) { return io ? SDL_RWread(io, ptr, 1, size) : 0; }
inline size_t SDL_WriteIO(SDL_RWops* io, const void* ptr, size_t size) { return io ? SDL_RWwrite(io, ptr, 1, size) : 0; }
inline bool SDL_CloseIO(SDL_RWops* io) { return io ? SDL_RWclose(io) == 0 : true; }
// SDL 2 does not tell an error from the end of the file; a short read is
// taken as the end, and what was read is verified by its hash afterwards.
inline SDL_IOStatus SDL_GetIOStatus(SDL_RWops*) { return SDL_IO_STATUS_EOF; }

// Audio
inline c3::AudioStream* SDL_OpenAudioDeviceStream(int, const c3::AudioSpec* spec, void*, void*) {
    return c3::OpenAudioDeviceStream(spec);
}
inline bool SDL_ResumeAudioStreamDevice(c3::AudioStream* s) { if (s) SDL_PauseAudioDevice(s->dev, 0); return s != nullptr; }
inline bool SDL_SetAudioStreamGain(c3::AudioStream* s, float gain) { if (s) s->gain = gain; return s != nullptr; }
inline bool SDL_SetAudioStreamFrequencyRatio(c3::AudioStream* s, float ratio) { if (s) s->ratio = double(ratio); return s != nullptr; }
inline bool SDL_PutAudioStreamData(c3::AudioStream* s, const void* data, int bytes) { return c3::PutAudioStreamData(s, data, bytes); }
inline int SDL_GetAudioStreamQueued(c3::AudioStream* s) { return s ? int(SDL_GetQueuedAudioSize(s->dev)) : 0; }
inline bool SDL_ClearAudioStream(c3::AudioStream* s) { if (s) SDL_ClearQueuedAudio(s->dev); return s != nullptr; }
inline void SDL_DestroyAudioStream(c3::AudioStream* s) {
    if (!s) return;
    SDL_CloseAudioDevice(s->dev);
    delete s;
}

// Things the console does not have: no folders of the user's, no file
// dialog, no browser.
enum SDL_Folder { SDL_FOLDER_DESKTOP, SDL_FOLDER_DOCUMENTS, SDL_FOLDER_DOWNLOADS };
inline const char* SDL_GetUserFolder(SDL_Folder) { return nullptr; }
struct SDL_DialogFileFilter {
    const char* name;
    const char* pattern;
};
using SDL_DialogFileCallback = void (*)(void* userdata, const char* const* files, int filter);
inline void SDL_ShowOpenFileDialog(SDL_DialogFileCallback callback, void* userdata, SDL_Window*,
                                   const SDL_DialogFileFilter*, int, const char*, bool) {
    callback(userdata, nullptr, -1);  // as if it had been cancelled
}
inline bool c3_open_url(const char*) { return false; }
#define SDL_OpenURL(url) c3_open_url(url)

// SDL 3 owns the string; SDL 2 hands over a copy to free.
inline const char* c3_base_path() {
    static std::string path = [] {
        char* p = ::SDL_GetBasePath();
        std::string s = p ? p : "";
        if (p) SDL_free(p);
        return s;
    }();
    return path.empty() ? nullptr : path.c_str();
}
#define SDL_GetBasePath() c3_base_path()

// SDL 3's 8x8 debug text (the FPS counter, the debug overlay), drawn with the
// embedded pixel face, which has the same cell (sdl3_shim.cpp).
void SDL_RenderDebugText(SDL_Renderer* renderer, float x, float y, const char* text);
