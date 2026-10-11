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
#include "Renderer.h"

#include <algorithm>
#include <cmath>

#include "Config.h"

void Renderer::render(const Map& map, const Player& player,
                      const std::vector<Enemy>& enemies, const std::vector<Pickup>& pickups,
                      const Textures& textures)
{
    // Потолок и пол — градиентом, чтобы было не скучно.
    DrawRectangleGradientV(0, 0, cfg::SCREEN_WIDTH, cfg::SCREEN_HEIGHT / 2,
                           Color{22, 22, 26, 255}, Color{48, 48, 56, 255});
    DrawRectangleGradientV(0, cfg::SCREEN_HEIGHT / 2, cfg::SCREEN_WIDTH, cfg::SCREEN_HEIGHT / 2,
                           Color{60, 52, 44, 255}, Color{18, 16, 14, 255});

    renderWalls(map, player, textures);
    renderSprites(player, enemies, pickups, textures);
}

void Renderer::renderWalls(const Map& map, const Player& player, const Textures& textures)
{
    const Vector2 d = player.dir();
    const Vector2 p = player.plane();

    zbuffer_.assign(cfg::SCREEN_WIDTH, 0.0f);

    for (int x = 0; x < cfg::SCREEN_WIDTH; ++x) {
        // cameraX ∈ [-1, 1]: -1 — левый край экрана, 1 — правый.
        const float cameraX = 2.0f * x / cfg::SCREEN_WIDTH - 1.0f;
        const float rdx = d.x + p.x * cameraX;
        const float rdy = d.y + p.y * cameraX;

        // --- DDA: шагаем лучом по сетке, пока не упрёмся в стену ---
        int mapX = static_cast<int>(player.pos.x);
        int mapY = static_cast<int>(player.pos.y);

        const float deltaX = (std::fabs(rdx) > 1e-9f) ? std::fabs(1.0f / rdx) : 1e30f;
        const float deltaY = (std::fabs(rdy) > 1e-9f) ? std::fabs(1.0f / rdy) : 1e30f;

        int stepX, stepY;
        float sideX, sideY;
        if (rdx < 0) { stepX = -1; sideX = (player.pos.x - mapX) * deltaX; }
        else         { stepX =  1; sideX = (mapX + 1 - player.pos.x) * deltaX; }
        if (rdy < 0) { stepY = -1; sideY = (player.pos.y - mapY) * deltaY; }
        else         { stepY =  1; sideY = (mapY + 1 - player.pos.y) * deltaY; }

        int side = 0;
        int hitType = 0;
        bool hit = false;

        for (int i = 0; i < 256 && !hit; ++i) {
            if (sideX < sideY) {
                sideX += deltaX;
                mapX += stepX;
                side = 0;
            } else {
                sideY += deltaY;
                mapY += stepY;
                side = 1;
            }
            if (map.isWall(mapX, mapY)) {
                hit = true;
                hitType = map.wallType(mapX, mapY);
            }
        }

        if (!hit) {
            zbuffer_[x] = 1e30f;
            continue;
        }

        hitType = std::clamp(hitType, 1, cfg::WALL_TYPE_COUNT);

        // Перпендикулярное расстояние убирает «рыбий глаз». Без него стены
        // выглядели бы так, будто смотришь изнутри аквариума.
        float perpDist = (side == 0) ? (sideX - deltaX) : (sideY - deltaY);
        if (perpDist < 0.02f) perpDist = 0.02f;
        zbuffer_[x] = perpDist;

        // Где именно луч ударил (0..1) — по этому выбираем колонку текстуры.
        float wallX = (side == 0) ? player.pos.y + perpDist * rdy
                                  : player.pos.x + perpDist * rdx;
        wallX -= std::floor(wallX);

        int texX = static_cast<int>(wallX * cfg::TEX_SIZE);
        if ((side == 0 && rdx > 0) || (side == 1 && rdy < 0)) {
            texX = cfg::TEX_SIZE - texX - 1;
        }
        texX = std::clamp(texX, 0, cfg::TEX_SIZE - 1);

        // Туман дальности + затемнение граней, смотрящих по Y.
        const float fog = 1.0f / (1.0f + perpDist * perpDist * 0.012f);
        const float sideMul = (side == 1) ? 0.72f : 1.0f;
        const float bright = std::clamp(fog * sideMul, 0.0f, 1.0f);
        const unsigned char b = static_cast<unsigned char>(bright * 255.0f);
        const Color tint{b, b, b, 255};

        const int lineHeight = static_cast<int>(cfg::SCREEN_HEIGHT / perpDist);
        const int drawStart = -lineHeight / 2 + cfg::SCREEN_HEIGHT / 2;
        const int drawEnd = lineHeight / 2 + cfg::SCREEN_HEIGHT / 2;

        const Texture2D& tex = textures.walls()[hitType];
        DrawTexturePro(
            tex,
            Rectangle{static_cast<float>(texX), 0.0f, 1.0f, static_cast<float>(tex.height)},
            Rectangle{static_cast<float>(x), static_cast<float>(drawStart),
                      1.0f, static_cast<float>(drawEnd - drawStart)},
            Vector2{0.0f, 0.0f}, 0.0f, tint);
    }
}

void Renderer::renderSprites(const Player& player, const std::vector<Enemy>& enemies,
                             const std::vector<Pickup>& pickups, const Textures& textures)
{
    // Враги и пикапы — одинаковые вертикальные билборды. Сливаем их в один
    // список и рисуем от дальних к ближним, чтобы они перекрывались правильно.
    struct Billboard {
        float tx, ty;        // преобразование камеры (ty = глубина по оси взгляда)
        Texture2D texture;
        float sizeFactor;    // высота как доля от целой стены
        float yOffset;       // вертикальный сдвиг на экране (покачивание пикапа)
        Color tint;
    };

    const Vector2 d = player.dir();
    const Vector2 p = player.plane();
    const float invDet = 1.0f / (p.x * d.y - d.x * p.y);

    std::vector<Billboard> list;
    list.reserve(enemies.size() + pickups.size());

    for (const auto& e : enemies) {
        if (!e.alive()) continue;
        const float relX = e.pos.x - player.pos.x;
        const float relY = e.pos.y - player.pos.y;
        const float tX = invDet * (d.y * relX - d.x * relY);
        const float tY = invDet * (-p.y * relX + p.x * relY);
        if (tY <= 0.1f) continue;

        const float fog = 1.0f / (1.0f + tY * tY * 0.012f);
        const unsigned char sh = static_cast<unsigned char>(std::clamp(fog, 0.0f, 1.0f) * 255.0f);
        const Color tint = (e.hitFlash > 0.0f) ? Color{255, 140, 140, 255}
                                               : Color{sh, sh, sh, 255};
        list.push_back({tX, tY, textures.enemy(), 0.85f, 0.0f, tint});
    }

    for (const auto& pk : pickups) {
        const float relX = pk.pos.x - player.pos.x;
        const float relY = pk.pos.y - player.pos.y;
        const float tX = invDet * (d.y * relX - d.x * relY);
        const float tY = invDet * (-p.y * relX + p.x * relY);
        if (tY <= 0.1f) continue;

        const float fog = 1.0f / (1.0f + tY * tY * 0.012f);
        const unsigned char sh = static_cast<unsigned char>(std::clamp(fog, 0.0f, 1.0f) * 255.0f);
        const Texture2D tex = (pk.type == 0) ? textures.healthPickup() : textures.armorPickup();
        const float bob = std::sin(pk.bob) * 4.0f;
        list.push_back({tX, tY, tex, 0.35f, bob, Color{sh, sh, sh, 255}});
    }

    std::sort(list.begin(), list.end(),
              [](const Billboard& a, const Billboard& b) { return a.ty > b.ty; });

    for (const auto& b : list) {
        // Экранный X центра билборда.
        const int screenX = static_cast<int>(
            (cfg::SCREEN_WIDTH / 2.0f) * (1.0f + b.tx / b.ty));

        // Ставим ноги на линию пола на этой глубине.
        const float wallH = cfg::SCREEN_HEIGHT / b.ty;
        const int spriteH = static_cast<int>(wallH * b.sizeFactor);
        const int drawEndY = static_cast<int>(cfg::SCREEN_HEIGHT / 2.0f + wallH / 2.0f + b.yOffset);
        const int drawStartY = drawEndY - spriteH;
        const int drawStartX = screenX - spriteH / 2;   // текстура квадратная

        const float texW = static_cast<float>(b.texture.width);
        const float texH = static_cast<float>(b.texture.height);

        const int colStart = std::max(0, drawStartX);
        const int colEnd = std::min(cfg::SCREEN_WIDTH, drawStartX + spriteH);

        for (int col = colStart; col < colEnd; ++col) {
            if (b.ty >= zbuffer_[col]) continue;   // стена ближе — не рисуем

            int texX = static_cast<int>((col - drawStartX) * texW / spriteH);
            texX = std::clamp(texX, 0, static_cast<int>(texW) - 1);

            DrawTexturePro(
                b.texture,
                Rectangle{static_cast<float>(texX), 0.0f, 1.0f, texH},
                Rectangle{static_cast<float>(col), static_cast<float>(drawStartY),
                          1.0f, static_cast<float>(spriteH)},
                Vector2{0.0f, 0.0f}, 0.0f, b.tint);
        }
    }
}
