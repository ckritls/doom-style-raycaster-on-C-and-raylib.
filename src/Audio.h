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
#pragma once

// Крошечный звуковой движок: всё синтезируется кодом (никаких ассетов),
// через конвейер raylib Wave -> Sound.
namespace audio {

void init();        // после InitWindow()
void shutdown();    // перед CloseWindow()

void playShoot(int weapon);   // 0 = пистолет, 1 = дробовик, 2 = SMG
void playHit();               // попали
void playEnemyDeath();
void playPlayerHurt();
void playSwitch();
void playReload();
void playEmpty();             // сухой щелчок
void playPickupHealth();
void playPickupArmor();

} // namespace audio
