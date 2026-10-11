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

#include "raylib.h"

// Лежащая на полу вкусняшка.
struct Pickup {
    Vector2 pos;
    int type;          // 0 = аптечка, 1 = броня
    float bob = 0.0f;  // фаза покачивания
};
