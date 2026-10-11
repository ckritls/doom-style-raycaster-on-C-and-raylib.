/*
 * Copyright (C) 2026 ckritls
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even meven implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include "Audio.h"

#include <cmath>

#include "raylib.h"

namespace {

constexpr float TAU = 6.28318530718f;

bool ready = false;

Sound sPistol{};
Sound sShotgun{};
Sound sSmg{};
Sound sHit{};
Sound sDeath{};
Sound sHurt{};
Sound sSwitch{};
Sound sReload{};
Sound sEmpty{};
Sound sPickupHealth{};
Sound sPickupArmor{};

float frac(float x)
{
    return x - std::floor(x);
}

// Дешёвый детерминированный белый шум — без внешнего ГСЧ.
float noise(float t)
{
    return frac(std::sin(t * 127.1f) * 43758.5453f) * 2.0f - 1.0f;
}

// Синтезируем моно 16-бит PCM вручную. Ассеты — для слабаков.
Wave buildWave(float duration, float (*sample)(float))
{
    const int sampleRate = 44100;
    const int channels = 1;
    const int frameCount = static_cast<int>(sampleRate * duration);

    Wave w{};
    w.frameCount = static_cast<unsigned int>(frameCount);
    w.sampleRate = static_cast<unsigned int>(sampleRate);
    w.sampleSize = 16;
    w.channels = static_cast<unsigned int>(channels);
    w.data = MemAlloc(static_cast<unsigned int>(frameCount * channels * sizeof(short)));

    short* s = static_cast<short*>(w.data);
    for (int i = 0; i < frameCount; ++i) {
        const float t = static_cast<float>(i) / sampleRate;
        float v = sample(t);
        if (v > 1.0f) v = 1.0f;
        if (v < -1.0f) v = -1.0f;
        s[i] = static_cast<short>(v * 32000.0f);
    }
    return w;
}

// Загоняем Wave в Sound; Wave после этого можно смело грохнуть.
Sound makeSound(float duration, float (*sample)(float))
{
    Wave w = buildWave(duration, sample);
    Sound s = LoadSoundFromWave(w);
    UnloadWave(w);
    return s;
}

// --- Формы волн (t в секундах; затухание зашито в каждую) ---

float pistolWave(float t)
{
    const float env = std::exp(-t * 35.0f);
    const float phase = TAU * (900.0f * t - 0.5f * 3800.0f * t * t);
    return env * (std::sin(phase) > 0.0f ? 0.9f : -0.9f);
}

float shotgunWave(float t)
{
    const float env = std::exp(-t * 16.0f);
    const float thump = std::sin(TAU * 90.0f * t);
    return env * (0.55f * noise(t * 9000.0f) + 0.45f * thump);
}

float smgWave(float t)
{
    const float env = std::exp(-t * 50.0f);
    const float phase = TAU * (1400.0f - 3000.0f * t) * t;
    return env * (std::sin(phase) > 0.0f ? 0.8f : -0.8f);
}

float hitWave(float t)
{
    const float env = std::exp(-t * 60.0f);
    const float phase = TAU * (320.0f - 1200.0f * t) * t;
    return env * std::sin(phase);
}

float deathWave(float t)
{
    const float env = std::exp(-t * 6.0f);
    const float f = 520.0f - 320.0f * (t / 0.45f);
    return env * 0.7f * (std::sin(TAU * f * t) * 0.6f + noise(t * 4000.0f) * 0.4f);
}

float hurtWave(float t)
{
    const float env = std::exp(-t * 18.0f);
    return env * (std::sin(TAU * 180.0f * t) > 0.0f ? 0.8f : -0.8f);
}

float switchWave(float t)
{
    const float env = std::exp(-t * 55.0f);
    const float phase = TAU * (600.0f + 700.0f * t) * t;
    return env * std::sin(phase);
}

float reloadWave(float t)
{
    // Два механических «клац-клац».
    const float c1 = std::exp(-t * 45.0f) * std::sin(TAU * 700.0f * t);
    const float t2 = t - 0.16f;
    const float c2 = (t2 > 0.0f) ? std::exp(-t2 * 45.0f) * std::sin(TAU * 500.0f * t2) : 0.0f;
    return 0.6f * (c1 + c2);
}

float emptyWave(float t)
{
    return std::exp(-t * 80.0f) * std::sin(TAU * 900.0f * t);
}

float pickupHealthWave(float t)
{
    const float env = std::exp(-t * 10.0f);
    const float f = 500.0f + 900.0f * (t / 0.25f);
    return env * 0.7f * std::sin(TAU * f * t);
}

float pickupArmorWave(float t)
{
    const float env = std::exp(-t * 9.0f);
    const float f = 260.0f + 400.0f * (t / 0.3f);
    return env * 0.7f * std::sin(TAU * f * t);
}

} // namespace

namespace audio {

void init()
{
    ready = false;
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;

    sPistol = makeSound(0.14f, pistolWave);
    sShotgun = makeSound(0.28f, shotgunWave);
    sSmg = makeSound(0.07f, smgWave);
    sHit = makeSound(0.06f, hitWave);
    sDeath = makeSound(0.45f, deathWave);
    sHurt = makeSound(0.18f, hurtWave);
    sSwitch = makeSound(0.09f, switchWave);
    sReload = makeSound(0.35f, reloadWave);
    sEmpty = makeSound(0.06f, emptyWave);
    sPickupHealth = makeSound(0.25f, pickupHealthWave);
    sPickupArmor = makeSound(0.30f, pickupArmorWave);

    SetMasterVolume(0.6f);
    ready = true;
}

void shutdown()
{
    if (!IsAudioDeviceReady()) return;
    UnloadSound(sPistol);
    UnloadSound(sShotgun);
    UnloadSound(sSmg);
    UnloadSound(sHit);
    UnloadSound(sDeath);
    UnloadSound(sHurt);
    UnloadSound(sSwitch);
    UnloadSound(sReload);
    UnloadSound(sEmpty);
    UnloadSound(sPickupHealth);
    UnloadSound(sPickupArmor);
    CloseAudioDevice();
    ready = false;
}

void playShoot(int weapon)
{
    if (!ready) return;
    switch (weapon) {
        case 1: PlaySound(sShotgun); break;
        case 2: PlaySound(sSmg); break;
        default: PlaySound(sPistol); break;
    }
}

void playHit() { if (ready) PlaySound(sHit); }
void playEnemyDeath() { if (ready) PlaySound(sDeath); }
void playPlayerHurt() { if (ready) PlaySound(sHurt); }
void playSwitch() { if (ready) PlaySound(sSwitch); }
void playReload() { if (ready) PlaySound(sReload); }
void playEmpty() { if (ready) PlaySound(sEmpty); }
void playPickupHealth() { if (ready) PlaySound(sPickupHealth); }
void playPickupArmor() { if (ready) PlaySound(sPickupArmor); }

} // namespace audio
