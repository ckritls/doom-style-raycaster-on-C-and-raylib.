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
#include "Textures.h"

#include "Config.h"

namespace {

// Рисуем 64x64 текстуру стены пиксель за пикселем. Детерминированный шум —
// чтобы каждая отличалась, но плитки нормально стыковались.
Image makeWallTexture(int type)
{
    const int s = cfg::TEX_SIZE;
    Image img = GenImageColor(s, s, BLANK);

    for (int y = 0; y < s; ++y) {
        for (int x = 0; x < s; ++x) {
            Color c{0, 0, 0, 255};

            switch (type) {
                case 1: {   // серый камень
                    c = {110, 110, 118, 255};
                    if (x % 16 == 0 || y % 16 == 0) c = {70, 70, 78, 255};
                    const int n = (x * 7 + y * 11) % 9;
                    c.r = static_cast<unsigned char>(c.r - n);
                    c.g = static_cast<unsigned char>(c.g - n);
                    c.b = static_cast<unsigned char>(c.b - n);
                    break;
                }
                case 2: {   // красный кирпич (ряды в шахматку)
                    const int bh = 8;
                    const int bw = 16;
                    const int row = y / bh;
                    const int off = (row % 2) ? bw / 2 : 0;
                    const int xx = (x - off + s) % s;
                    const bool mortar = (y % bh == 0) || (xx % bw == 0);
                    c = mortar ? Color{60, 48, 44, 255} : Color{162, 66, 48, 255};
                    if (!mortar) {
                        const int n = (x * 3 + y * 5) % 7;
                        c.r = static_cast<unsigned char>(c.r - n);
                        c.g = static_cast<unsigned char>(c.g - n * 2);
                    }
                    break;
                }
                case 3: {   // металл/техно с заклёпками
                    c = {82, 96, 110, 255};
                    if (y % 16 < 2 || x % 16 < 2) c = {56, 66, 78, 255};
                    if ((x % 16 == 8) && (y % 16 == 8)) c = {150, 180, 200, 255};
                    break;
                }
                case 4: {   // дерево
                    c = {128, 86, 50, 255};
                    const int pw = 16;
                    if (x % pw < 1) c = {70, 45, 28, 255};
                    const int n = (x * 13 + y * 7) % 8;
                    c.r = static_cast<unsigned char>(c.r - n / 2);
                    c.g = static_cast<unsigned char>(c.g - n / 2);
                    break;
                }
                default:
                    c = {200, 0, 200, 255};   // ядовито-розовый = баг
                    break;
            }

            ImageDrawPixel(&img, x, y, c);
        }
    }
    return img;
}

// Рогатый бес из пары прямоугольников на прозрачном фоне.
Image makeEnemySprite()
{
    const int s = 64;
    Image img = GenImageColor(s, s, BLANK);

    const Color body{140, 40, 45, 255};
    const Color dark{90, 25, 30, 255};
    const Color skin{205, 130, 90, 255};
    const Color eye{255, 235, 80, 255};
    const Color horn{240, 235, 220, 255};

    // ноги
    ImageDrawRectangle(&img, 24, 50, 7, 14, dark);
    ImageDrawRectangle(&img, 33, 50, 7, 14, dark);
    // туловище
    ImageDrawRectangle(&img, 22, 26, 20, 24, body);
    // руки
    ImageDrawRectangle(&img, 14, 28, 8, 18, dark);
    ImageDrawRectangle(&img, 42, 28, 8, 18, dark);
    // голова
    ImageDrawCircle(&img, 32, 18, 9, skin);
    // глаза
    ImageDrawRectangle(&img, 28, 15, 3, 3, eye);
    ImageDrawRectangle(&img, 34, 15, 3, 3, eye);
    // рога
    ImageDrawTriangle(&img, {26.0f, 12.0f}, {20.0f, 4.0f}, {30.0f, 9.0f}, horn);
    ImageDrawTriangle(&img, {38.0f, 12.0f}, {44.0f, 4.0f}, {34.0f, 9.0f}, horn);

    return img;
}

// Аптечка с красным крестом.
Image makeHealthPickup()
{
    Image img = GenImageColor(cfg::TEX_SIZE, cfg::TEX_SIZE, BLANK);
    ImageDrawRectangle(&img, 14, 14, 36, 36, Color{228, 228, 234, 255});
    ImageDrawRectangle(&img, 27, 18, 10, 28, Color{210, 40, 40, 255});
    ImageDrawRectangle(&img, 18, 27, 28, 10, Color{210, 40, 40, 255});
    ImageDrawRectangleLines(&img, Rectangle{14.0f, 14.0f, 36.0f, 36.0f}, 2, Color{120, 120, 130, 255});
    return img;
}

// Синий щит-ромб.
Image makeArmorPickup()
{
    Image img = GenImageColor(cfg::TEX_SIZE, cfg::TEX_SIZE, BLANK);
    const Color base{58, 108, 198, 255};
    const Color light{130, 175, 245, 255};
    ImageDrawTriangle(&img, {32.0f, 4.0f}, {48.0f, 16.0f}, {16.0f, 16.0f}, base);
    ImageDrawRectangle(&img, 16, 16, 32, 34, base);
    ImageDrawTriangle(&img, {16.0f, 50.0f}, {48.0f, 50.0f}, {32.0f, 60.0f}, base);
    ImageDrawRectangle(&img, 22, 20, 8, 24, light);
    return img;
}

} // namespace

void Textures::load()
{
    walls_.assign(cfg::WALL_TYPE_COUNT + 1, Texture2D{});
    for (int t = 1; t <= cfg::WALL_TYPE_COUNT; ++t) {
        Image img = makeWallTexture(t);
        walls_[t] = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    Image e = makeEnemySprite();
    enemy_ = LoadTextureFromImage(e);
    UnloadImage(e);

    Image hp = makeHealthPickup();
    healthPickup_ = LoadTextureFromImage(hp);
    UnloadImage(hp);

    Image ap = makeArmorPickup();
    armorPickup_ = LoadTextureFromImage(ap);
    UnloadImage(ap);
}

void Textures::unload()
{
    for (auto& t : walls_) {
        if (t.id != 0) UnloadTexture(t);
    }
    walls_.clear();

    if (enemy_.id != 0) UnloadTexture(enemy_);
    enemy_ = Texture2D{};

    if (healthPickup_.id != 0) UnloadTexture(healthPickup_);
    healthPickup_ = Texture2D{};
    if (armorPickup_.id != 0) UnloadTexture(armorPickup_);
    armorPickup_ = Texture2D{};
}
