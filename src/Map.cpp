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
#include "Map.h"

#include <algorithm>
#include <cmath>
#include <queue>

#include "Config.h"

void Map::load(const std::vector<std::string>& layout)
{
    grid_ = layout;
    height_ = static_cast<int>(grid_.size());
    width_ = 0;
    for (const auto& row : grid_) {
        width_ = std::max(width_, static_cast<int>(row.size()));
    }

    // Выравниваем ряды: короткие добиваем стеной, чтобы никто не выпал за край.
    for (auto& row : grid_) {
        row.resize(width_, '#');
    }

    enemyStarts_.clear();
    pickupStarts_.clear();
    playerStart_ = {1.5f, 1.5f};
    playerAngle_ = 0.0f;

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            const char c = grid_[y][x];
            if (c == 'P') {
                playerStart_ = {x + 0.5f, y + 0.5f};
            } else if (c == 'E') {
                enemyStarts_.push_back({x + 0.5f, y + 0.5f});
            } else if (c == 'H') {
                pickupStarts_.push_back({{x + 0.5f, y + 0.5f}, 0});
            } else if (c == 'A') {
                pickupStarts_.push_back({{x + 0.5f, y + 0.5f}, 1});
            }
        }
    }

    // Смотрим в центр карты — так на старте хоть что-то видно.
    const float cx = width_ * 0.5f;
    const float cy = height_ * 0.5f;
    playerAngle_ = std::atan2(cy - playerStart_.y, cx - playerStart_.x);

    computeWallDistance();
}

bool Map::isWall(int x, int y) const
{
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return true;
    const char c = grid_[y][x];
    return c != ' ' && c != '.' && c != 'P' && c != 'E' && c != 'H' && c != 'A';
}

int Map::wallType(int x, int y) const
{
    if (!isWall(x, y)) return 0;
    const char c = grid_[y][x];
    if (c >= '1' && c <= '9') return c - '0';
    return 1;   // '#' — текстура по умолчанию
}

bool Map::collides(float x, float y, float radius) const
{
    // Проверяем все клетки под квадратом вокруг точки — дёшево и надёжно.
    const int x0 = static_cast<int>(std::floor(x - radius));
    const int x1 = static_cast<int>(std::floor(x + radius));
    const int y0 = static_cast<int>(std::floor(y - radius));
    const int y1 = static_cast<int>(std::floor(y + radius));

    for (int gx = x0; gx <= x1; ++gx) {
        for (int gy = y0; gy <= y1; ++gy) {
            if (isWall(gx, gy)) return true;
        }
    }
    return false;
}

// Шагаем по сетке от A к B (DDA); врезались в стену — значит, перекрыто.
// Нужно и врагам (видят ли они тебя), и пулям (не стрелять сквозь стены).
bool Map::lineOfSight(float x0, float y0, float x1, float y1) const
{
    const float dx = x1 - x0;
    const float dy = y1 - y0;
    const float dist = std::sqrt(dx * dx + dy * dy);
    if (dist < 1e-4f) return true;

    const float rdx = dx / dist;
    const float rdy = dy / dist;

    int mapX = static_cast<int>(x0);
    int mapY = static_cast<int>(y0);

    const float deltaX = (std::fabs(rdx) > 1e-9f) ? std::fabs(1.0f / rdx) : 1e30f;
    const float deltaY = (std::fabs(rdy) > 1e-9f) ? std::fabs(1.0f / rdy) : 1e30f;

    const int stepX = (rdx > 0) ? 1 : -1;
    const int stepY = (rdy > 0) ? 1 : -1;

    float sideX = (rdx > 0) ? (mapX + 1 - x0) * deltaX : (x0 - mapX) * deltaX;
    float sideY = (rdy > 0) ? (mapY + 1 - y0) * deltaY : (y0 - mapY) * deltaY;

    float traveled = 0.0f;
    while (traveled <= dist) {
        if (isWall(mapX, mapY)) return false;
        if (sideX < sideY) {
            traveled = sideX;
            sideX += deltaX;
            mapX += stepX;
        } else {
            traveled = sideY;
            sideY += deltaY;
            mapY += stepY;
        }
    }
    return true;
}

// Тот же DDA, но возвращает точное расстояние до стены (нужно трассерам).
float Map::raycast(float x, float y, float dirX, float dirY, Vector2& hitPoint) const
{
    int mapX = static_cast<int>(x);
    int mapY = static_cast<int>(y);

    const float deltaX = (std::fabs(dirX) > 1e-9f) ? std::fabs(1.0f / dirX) : 1e30f;
    const float deltaY = (std::fabs(dirY) > 1e-9f) ? std::fabs(1.0f / dirY) : 1e30f;

    int stepX, stepY;
    float sideX, sideY;
    if (dirX < 0) { stepX = -1; sideX = (x - mapX) * deltaX; }
    else          { stepX =  1; sideX = (mapX + 1 - x) * deltaX; }
    if (dirY < 0) { stepY = -1; sideY = (y - mapY) * deltaY; }
    else          { stepY =  1; sideY = (mapY + 1 - y) * deltaY; }

    int side = 0;
    for (int i = 0; i < 256; ++i) {
        if (sideX < sideY) {
            sideX += deltaX;
            mapX += stepX;
            side = 0;
        } else {
            sideY += deltaY;
            mapY += stepY;
            side = 1;
        }
        if (isWall(mapX, mapY)) {
            const float dist = (side == 0) ? (sideX - deltaX) : (sideY - deltaY);
            hitPoint = {x + dirX * dist, y + dirY * dist};
            return dist;
        }
    }
    return -1.0f;
}

// Дистанция от каждой клетки до ближайшей стены (BFS сразу от всех стен).
// Нужно, чтобы враги «чувствовали», где открытая площадка, а где укрытие.
void Map::computeWallDistance()
{
    const int total = width_ * height_;
    wallDist_.assign(total, -1);
    std::vector<int> queue;
    queue.reserve(total);

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            if (isWall(x, y)) {
                wallDist_[y * width_ + x] = 0;
                queue.push_back(y * width_ + x);
            }
        }
    }

    const int dx4[4] = {1, -1, 0, 0};
    const int dy4[4] = {0, 0, 1, -1};
    for (size_t head = 0; head < queue.size(); ++head) {
        const int cur = queue[head];
        const int cx = cur % width_;
        const int cy = cur / width_;
        for (int k = 0; k < 4; ++k) {
            const int nx = cx + dx4[k];
            const int ny = cy + dy4[k];
            if (nx < 0 || ny < 0 || nx >= width_ || ny >= height_) continue;
            const int ni = ny * width_ + nx;
            if (wallDist_[ni] == -1) {
                wallDist_[ni] = wallDist_[cur] + 1;
                queue.push_back(ni);
            }
        }
    }
}

// Цена входа в клетку: чем дальше от стены, тем дороже. Так A* жмёт врагов
// к стеночкам, а не гоняет через центр зала.
float Map::cellCost(int x, int y) const
{
    if (isWall(x, y)) return 1e30f;
    const int d = wallDist_[y * width_ + x];
    return 1.0f + cfg::ENEMY_OPEN_SPACE_PENALTY * static_cast<float>(std::max(0, d - 1));
}

// A* с эвристикой Манхэттена и ценой открытого пространства.
bool Map::findPath(int sx, int sy, int gx, int gy, std::vector<Vector2>& path) const
{
    path.clear();
    if (isWall(sx, sy) || isWall(gx, gy)) return false;
    if (sx == gx && sy == gy) {
        path.push_back({sx + 0.5f, sy + 0.5f});
        return true;
    }

    const int total = width_ * height_;
    std::vector<float> gCost(total, 1e30f);
    std::vector<int> cameFrom(total, -1);
    std::vector<int> closed(total, 0);

    const int startIdx = sy * width_ + sx;
    const int goalIdx = gy * width_ + gx;

    const auto heuristic = [&](int x, int y) {
        const int ax = (x > gx) ? (x - gx) : (gx - x);
        const int ay = (y > gy) ? (y - gy) : (gy - y);
        return static_cast<float>(ax + ay);
    };

    // min-heap по f = g + h.
    using Node = std::pair<float, int>;
    struct NodeCmp {
        bool operator()(const Node& a, const Node& b) const { return a.first > b.first; }
    };
    std::priority_queue<Node, std::vector<Node>, NodeCmp> open;

    gCost[startIdx] = 0.0f;
    open.push({heuristic(sx, sy), startIdx});

    const int dx4[4] = {1, -1, 0, 0};
    const int dy4[4] = {0, 0, 1, -1};
    bool found = false;

    while (!open.empty()) {
        const int cur = open.top().second;
        open.pop();
        if (closed[cur]) continue;
        closed[cur] = 1;
        if (cur == goalIdx) { found = true; break; }

        const int cx = cur % width_;
        const int cy = cur / width_;
        for (int k = 0; k < 4; ++k) {
            const int nx = cx + dx4[k];
            const int ny = cy + dy4[k];
            if (nx < 0 || ny < 0 || nx >= width_ || ny >= height_) continue;
            if (isWall(nx, ny)) continue;
            const int ni = ny * width_ + nx;
            if (closed[ni]) continue;
            const float ng = gCost[cur] + cellCost(nx, ny);
            if (ng < gCost[ni]) {
                gCost[ni] = ng;
                cameFrom[ni] = cur;
                open.push({ng + heuristic(nx, ny), ni});
            }
        }
    }

    if (!found) return false;

    // Восстанавливаем путь от цели к старту и разворачиваем.
    std::vector<int> cells;
    for (int c = goalIdx; c != -1; c = cameFrom[c]) {
        cells.push_back(c);
    }
    std::reverse(cells.begin(), cells.end());

    for (int c : cells) {
        path.push_back({static_cast<float>(c % width_) + 0.5f,
                        static_cast<float>(c / width_) + 0.5f});
    }
    return true;
}
