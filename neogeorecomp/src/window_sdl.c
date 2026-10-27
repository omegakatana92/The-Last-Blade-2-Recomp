/*
 * window_sdl.c — the SDL2 window: presentation, sound, keyboard, gamepads,
 * frame pacing. Only built when SDL2 is found; see platform.c for the
 * headless stubs.
 *
 * Keys: arrows, Z/X/C/V = A/B/C/D, 1/2 = start P1/P2, 3 = select,
 * 5/6 = coin 1/2, 9 = service, F2 = test switch, F11/Alt+Enter toggles
 * fullscreen, Esc = quit.
 *
 * Gamepads (SDL's GameController layout, so any pad SDL knows): the first
 * connected is player 1, the second player 2, plugged in at any time.
 * D-pad or left stick; south/east/west/north (Xbox A/B/X/Y) = Neo Geo
 * A/B/C/D; Start = start; Back = coin; left shoulder = select. Keyboard
 * and pads work together. A pad SDL doesn't know can be described with the
 * SDL_GAMECONTROLLERCONFIG environment variable.
 */
#include "ng_internal.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>

static SDL_Window *s_win;
static SDL_Renderer *s_ren;
static SDL_Texture *s_tex;
static uint64_t s_next_tick;
static SDL_AudioDeviceID s_audio;
static SDL_AudioStream *s_audio_stream;  /* 55,555 Hz -> device rate */
static int s_audio_rate, s_audio_frame_bytes = 4;
static int s_audio_warned, s_audio_fed;
static uint64_t s_audio_calls;
static SDL_GameController *s_pad[2];     /* player 1, player 2 */
static int s_fullscreen;

/* ponytail: a fixed layout; remapping belongs to the frontend (ROADMAP.md) */
static void pad_added(int index) {
    for (int i = 0; i < 2; i++)
        if (!s_pad[i]) {
            s_pad[i] = SDL_GameControllerOpen(index);
            if (s_pad[i]) fprintf(stderr, "gamepad: %s is player %d\n", SDL_GameControllerName(s_pad[i]), i + 1);
            return;
        }
}

static void pad_removed(SDL_JoystickID id) {
    for (int i = 0; i < 2; i++)
        if (s_pad[i] && SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(s_pad[i])) == id) {
            SDL_GameControllerClose(s_pad[i]);
            s_pad[i] = NULL;
        }
}

int window_open(const char *title, int scale, int fullscreen) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "error: SDL_Init: %s\n", SDL_GetError());
        return -1;
    }
    s_fullscreen = fullscreen;
    Uint32 flags = SDL_WINDOW_RESIZABLE;
    if (s_fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    s_win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             NG_SCREEN_W * scale, NG_SCREEN_H * scale, flags);
    if (!s_win) {
        fprintf(stderr, "error: SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }
    s_ren = SDL_CreateRenderer(s_win, -1, SDL_RENDERER_ACCELERATED);
    if (!s_ren) {
        fprintf(stderr, "warning: no accelerated renderer (%s); trying software\n", SDL_GetError());
        s_ren = SDL_CreateRenderer(s_win, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!s_ren) {
        fprintf(stderr, "error: SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(s_win);
        s_win = NULL;
        SDL_Quit();
        return -1;
    }
    s_tex = SDL_CreateTexture(s_ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                              NG_SCREEN_W, NG_SCREEN_H);
    if (!s_tex) {
        fprintf(stderr, "error: SDL_CreateTexture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(s_ren);
        SDL_DestroyWindow(s_win);
        s_ren = NULL;
        s_win = NULL;
        SDL_Quit();
        return -1;
    }
    /* Keep the 320x224 pixels crisp when scaled. */
    SDL_SetTextureScaleMode(s_tex, SDL_ScaleModeNearest);
    s_next_tick = SDL_GetPerformanceCounter();
    /* The YM2610 runs at 55,555 Hz. Ask for that rate, but let SDL change
     * the frequency and buffer size: most devices run at 44.1 or 48 kHz.
     * When they do, an SDL_AudioStream resamples, so the game keeps the
     * right pitch and speed instead of playing 55,555 samples at 44.1 kHz.
     * A missing audio device leaves the game silent, not stopped. */
    SDL_AudioSpec want = {0}, have;
    want.freq = NG_AUDIO_RATE; want.format = AUDIO_S16SYS; want.channels = 2; want.samples = 1024;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) == 0)
        s_audio = SDL_OpenAudioDevice(NULL, 0, &want, &have,
                                      SDL_AUDIO_ALLOW_FREQUENCY_CHANGE |
                                      SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
    if (s_audio) {
        s_audio_rate = have.freq;
        s_audio_frame_bytes = (SDL_AUDIO_BITSIZE(have.format) / 8) * have.channels;
        if (s_audio_frame_bytes <= 0) s_audio_frame_bytes = 4;
        if (have.freq != NG_AUDIO_RATE || have.format != AUDIO_S16SYS || have.channels != 2) {
            s_audio_stream = SDL_NewAudioStream(AUDIO_S16SYS, 2, NG_AUDIO_RATE,
                                                have.format, have.channels, have.freq);
            if (s_audio_stream)
                fprintf(stderr, "audio: %d Hz stereo -> device %d Hz, %d ch (resampled)\n",
                        NG_AUDIO_RATE, have.freq, have.channels);
            else {
                fprintf(stderr, "warning: no audio converter: %s\n", SDL_GetError());
                SDL_CloseAudioDevice(s_audio);
                s_audio = 0;
            }
        } else {
            fprintf(stderr, "audio: %d Hz stereo\n", have.freq);
        }
    }
    if (!s_audio) fprintf(stderr, "warning: no sound: %s\n", SDL_GetError());
    /* Pads already connected arrive as SDL_CONTROLLERDEVICEADDED events. */
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0)
        fprintf(stderr, "warning: no gamepads: %s\n", SDL_GetError());
    return 0;
}

static void audio_queue(const void *data, Uint32 bytes) {
    if (SDL_QueueAudio(s_audio, data, bytes) != 0 && !s_audio_warned) {
        fprintf(stderr, "warning: audio queue: %s\n", SDL_GetError());
        s_audio_warned = 1;
    }
}

/* The frame loop and the sound card both run at the board's rates, so the
 * queue stays level. A 35 ms floor absorbs PulseAudio/PipeWire jitter; a
 * 300 ms ceiling catches a real stall without dropping audio during a
 * transient scheduling spike. When the device rate differs from the YM2610's, an
 * SDL_AudioStream converts before the samples reach the queue. */
void window_audio(const int16_t *samples, size_t frames) {
    if (!s_audio) return;
    const Uint32 bytes_per_ms = (Uint32)s_audio_rate * (Uint32)s_audio_frame_bytes / 1000;
    const Uint32 low_ms = 35, high_ms = 300;
    Uint32 queued = SDL_GetQueuedAudioSize(s_audio);
    int debug = getenv("NG_AUDIO_DEBUG") != NULL;
    if (debug && (++s_audio_calls % 60) == 0)
        fprintf(stderr, "[audio] queued %u ms, in %zu frames%s\n",
                queued * 1000 / (s_audio_rate * s_audio_frame_bytes), frames,
                s_audio_stream ? " (resampled)" : "");
    if (queued > high_ms * bytes_per_ms) {
        if (debug) fprintf(stderr, "[audio] dropping a frame (queue high)\n");
        return;
    }
    if (queued < low_ms * bytes_per_ms) {
        static const uint8_t silence[4096] = {0};
        Uint32 need = low_ms * bytes_per_ms - queued;
        while (need) {
            Uint32 n = need < sizeof(silence) ? need : (Uint32)sizeof(silence);
            audio_queue(silence, n);
            need -= n;
        }
    }
    if (s_audio_stream) {
        if (SDL_AudioStreamPut(s_audio_stream, samples, (int)(frames * 4)) != 0) {
            if (!s_audio_warned) {
                fprintf(stderr, "warning: audio resampler: %s\n", SDL_GetError());
                s_audio_warned = 1;
            }
            return;
        }
        uint8_t buf[65536];
        int avail;
        while ((avail = SDL_AudioStreamAvailable(s_audio_stream)) > 0) {
            int n = avail < (int)sizeof(buf) ? avail : (int)sizeof(buf);
            int got = SDL_AudioStreamGet(s_audio_stream, buf, n);
            if (got <= 0) break;
            audio_queue(buf, (Uint32)got);
        }
    } else {
        audio_queue(samples, (Uint32)(frames * 4));
    }
    SDL_PauseAudioDevice(s_audio, 0);
    s_audio_fed = 1;
}

static void read_keys(ng_input_t *in) {
    const Uint8 *k = SDL_GetKeyboardState(NULL);
    if (k[SDL_SCANCODE_UP]) in->p1 |= NG_UP;
    if (k[SDL_SCANCODE_DOWN]) in->p1 |= NG_DOWN;
    if (k[SDL_SCANCODE_LEFT]) in->p1 |= NG_LEFT;
    if (k[SDL_SCANCODE_RIGHT]) in->p1 |= NG_RIGHT;
    if (k[SDL_SCANCODE_Z]) in->p1 |= NG_A;
    if (k[SDL_SCANCODE_X]) in->p1 |= NG_B;
    if (k[SDL_SCANCODE_C]) in->p1 |= NG_C;
    if (k[SDL_SCANCODE_V]) in->p1 |= NG_D;
    in->start1 = k[SDL_SCANCODE_1];
    in->start2 = k[SDL_SCANCODE_2];
    in->select1 = k[SDL_SCANCODE_3];
    in->coin1 = k[SDL_SCANCODE_5];
    in->coin2 = k[SDL_SCANCODE_6];
    in->service = k[SDL_SCANCODE_9];
    in->test = k[SDL_SCANCODE_F2];
}

static void read_pad(SDL_GameController *c, uint8_t *bits, uint8_t *start, uint8_t *coin, uint8_t *select) {
    enum { DEAD = 12000 };   /* about a third of the stick's travel */
    int x = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTX);
    int y = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTY);
#define PAD(b) SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_##b)
    if (PAD(DPAD_UP) || y < -DEAD) *bits |= NG_UP;
    if (PAD(DPAD_DOWN) || y > DEAD) *bits |= NG_DOWN;
    if (PAD(DPAD_LEFT) || x < -DEAD) *bits |= NG_LEFT;
    if (PAD(DPAD_RIGHT) || x > DEAD) *bits |= NG_RIGHT;
    if (PAD(A)) *bits |= NG_A;
    if (PAD(B)) *bits |= NG_B;
    if (PAD(X)) *bits |= NG_C;
    if (PAD(Y)) *bits |= NG_D;
    *start |= PAD(START);
    *coin |= PAD(BACK);
    *select |= PAD(LEFTSHOULDER);
#undef PAD
}

/* Nudge the video pacing toward the audio queue's target. The sound card's
 * clock and the nominal 59.1856 Hz frame clock differ by a fraction of a
 * percent on real hardware (and more under PulseAudio/PipeWire), so a fixed
 * frame clock slowly fills or drains the queue. This PLL changes the wall
 * clock period by at most 0.5 ms per frame - under 3% - which is enough to
 * follow the audio device without dropping samples. */
static double audio_pacing_ms(void) {
    if (!s_audio || !s_audio_fed) return 0.0;
    const double bytes_per_ms = (double)s_audio_rate * (double)s_audio_frame_bytes / 1000.0;
    if (bytes_per_ms <= 0.0) return 0.0;
    const double target_ms = 45.0;
    double queued_ms = (double)SDL_GetQueuedAudioSize(s_audio) / bytes_per_ms;
    double adjust = (queued_ms - target_ms) * 0.10;
    if (adjust > 0.5) adjust = 0.5;
    if (adjust < -0.5) adjust = -0.5;
    return adjust;
}

bool window_frame(const uint32_t *fb, ng_input_t *in) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) return false;
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) return false;
        if (e.type == SDL_KEYDOWN &&
            (e.key.keysym.sym == SDLK_F11 ||
             (e.key.keysym.sym == SDLK_RETURN && (e.key.keysym.mod & KMOD_ALT)))) {
            s_fullscreen = !s_fullscreen;
            SDL_SetWindowFullscreen(s_win, s_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        }
        if (e.type == SDL_CONTROLLERDEVICEADDED) pad_added(e.cdevice.which);
        if (e.type == SDL_CONTROLLERDEVICEREMOVED) pad_removed(e.cdevice.which);
    }
    read_keys(in);
    if (s_pad[0]) read_pad(s_pad[0], &in->p1, &in->start1, &in->coin1, &in->select1);
    if (s_pad[1]) read_pad(s_pad[1], &in->p2, &in->start2, &in->coin2, &in->select2);
    if (fb) {
        SDL_UpdateTexture(s_tex, NULL, fb, NG_SCREEN_W * 4);
        SDL_RenderClear(s_ren);
        /* Fit the board's 10:7 picture inside the window with a uniform
         * scale, centred: pillarboxed on 16:10 Steam Deck screens,
         * letterboxed on 16:9, never stretched. */
        int ww = NG_SCREEN_W, wh = NG_SCREEN_H;
        SDL_GetRendererOutputSize(s_ren, &ww, &wh);
        float sx = (float)ww / NG_SCREEN_W, sy = (float)wh / NG_SCREEN_H;
        float scale = sx < sy ? sx : sy;
        int dw = (int)(NG_SCREEN_W * scale + 0.5f);
        int dh = (int)(NG_SCREEN_H * scale + 0.5f);
        SDL_Rect dst = { (ww - dw) / 2, (wh - dh) / 2, dw, dh };
        SDL_RenderCopy(s_ren, s_tex, NULL, &dst);
        SDL_RenderPresent(s_ren);
    }
    /* Pace to the MVS refresh rate, 59.1856 Hz, with a small audio-queue
     * correction so the sound card and the frame clock do not drift. */
    uint64_t freq = SDL_GetPerformanceFrequency();
    double period = (double)freq / 59.185606 + audio_pacing_ms() * (double)freq / 1000.0;
    s_next_tick += (uint64_t)period;
    uint64_t now = SDL_GetPerformanceCounter();
    if (now < s_next_tick) SDL_Delay((Uint32)((s_next_tick - now) * 1000 / freq));
    else if (now - s_next_tick > freq / 10) s_next_tick = now;
    return true;
}

void window_close(void) {
    for (int i = 0; i < 2; i++)
        if (s_pad[i]) SDL_GameControllerClose(s_pad[i]);
    if (s_audio_stream) { SDL_FreeAudioStream(s_audio_stream); s_audio_stream = NULL; }
    if (s_audio) SDL_CloseAudioDevice(s_audio);
    if (s_tex) SDL_DestroyTexture(s_tex);
    if (s_ren) SDL_DestroyRenderer(s_ren);
    if (s_win) SDL_DestroyWindow(s_win);
    SDL_Quit();
}
