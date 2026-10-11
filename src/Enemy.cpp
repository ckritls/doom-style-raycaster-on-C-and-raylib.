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
#include "Enemy.h"

#include <algorithm>
#include <cmath>

#include "Audio.h"
#include "Config.h"
#include "Player.h"

Enemy::Enemy(Vector2 p) : pos(p), health(cfg::ENEMY_HP) {}

void Enemy::takeDamage(float dmg)
{
    if (health <= 0.0f) return;
    health -= dmg;
    hitFlash = 0.15f;
    if (health < 0.0f) health = 0.0f;
    if (health <= 0.0f) audio::playEnemyDeath();
}

void Enemy::update(const Map& map, Player& player, const std::vector<Enemy>& enemies, float dt)
{
    attackCooldown = std::max(0.0f, attackCooldown - dt);
    hitFlash = std::max(0.0f, hitFlash - dt);
    if (!alive()) return;

    const float dx = player.pos.x - pos.x;
    const float dy = player.pos.y - pos.y;
    const float dist = std::sqrt(dx * dx + dy * dy);
    if (dist < 1e-4f) return;

    const bool canSee = map.lineOfSight(pos.x, pos.y, player.pos.x, player.pos.y);

    // Пересчитываем маршрут, если игрок сменил клетку или таймер истёк.
    const int pcx = static_cast<int>(player.pos.x);
    const int pcy = static_cast<int>(player.pos.y);
    repathTimer_ -= dt;
    if (pcx != lastTargetX_ || pcy != lastTargetY_ || repathTimer_ <= 0.0f) {
        lastTargetX_ = pcx;
        lastTargetY_ = pcy;
        repathTimer_ = cfg::ENEMY_REPATH_INTERVAL;
        recomputePath(map, pcx, pcy);
    }

    if (dist > cfg::ENEMY_ATTACK_RANGE || !canSee) {
        // Далеко или не видно — идём к цели (вдоль стен, не толкаясь).
        const Vector2 target = canSee ? player.pos : nextTarget(player);
        moveToward(map, enemies, target, dt);
    } else if (attackCooldown <= 0.0f) {
        // Рядом и видно — кусаемся.
        attackCooldown = cfg::ENEMY_ATTACK_COOLDOWN;
        player.takeDamage(cfg::ENEMY_DAMAGE);
    }
}

void Enemy::recomputePath(const Map& map, int gx, int gy)
{
    path_.clear();
    pathIndex_ = 0;

    const int sx = static_cast<int>(pos.x);
    const int sy = static_cast<int>(pos.y);

    std::vector<Vector2> cells;
    if (map.findPath(sx, sy, gx, gy, cells)) {
        // Стартовую клетку (мы уже в ней) отбрасываем — берём со следующей.
        for (size_t i = 1; i < cells.size(); ++i) {
            path_.push_back(cells[i]);
        }
    }
}

Vector2 Enemy::nextTarget(const Player& player)
{
    // Скидываем уже пройденные вейпоинты.
    while (pathIndex_ < static_cast<int>(path_.size())) {
        const float dx = path_[pathIndex_].x - pos.x;
        const float dy = path_[pathIndex_].y - pos.y;
        if (dx * dx + dy * dy < 0.15f * 0.15f) {
            ++pathIndex_;
        } else {
            break;
        }
    }

    if (pathIndex_ < static_cast<int>(path_.size())) return path_[pathIndex_];
    return player.pos;   // маршрут кончился — цель сам игрок
}

void Enemy::moveToward(const Map& map, const std::vector<Enemy>& enemies, Vector2 target, float dt)
{
    // Базовое направление — к цели.
    float dx = target.x - pos.x;
    float dy = target.y - pos.y;
    const float d = std::sqrt(dx * dx + dy * dy);
    if (d > 1e-4f) { dx /= d; dy /= d; } else { dx = 0.0f; dy = 0.0f; }

    // Расталкивание: держим дистанцию от соседей.
    float sx = 0.0f, sy = 0.0f;
    for (const auto& o : enemies) {
        if (&o == this || !o.alive()) continue;
        const float ox = pos.x - o.pos.x;
        const float oy = pos.y - o.pos.y;
        const float d2 = ox * ox + oy * oy;
        if (d2 < cfg::ENEMY_SEPARATION_RADIUS * cfg::ENEMY_SEPARATION_RADIUS && d2 > 1e-6f) {
            const float dd = std::sqrt(d2);
            const float push = (cfg::ENEMY_SEPARATION_RADIUS - dd) / cfg::ENEMY_SEPARATION_RADIUS;
            sx += (ox / dd) * push;
            sy += (oy / dd) * push;
        }
    }

    // Смешиваем «к цели» и «от соседей».
    float mx = dx + sx * cfg::ENEMY_SEPARATION_WEIGHT;
    float my = dy + sy * cfg::ENEMY_SEPARATION_WEIGHT;
    const float m = std::sqrt(mx * mx + my * my);
    if (m < 1e-4f) return;
    mx /= m;
    my /= m;

    const float step = cfg::ENEMY_SPEED * dt;
    const float nx = pos.x + mx * step;
    const float ny = pos.y + my * step;
    if (!map.collides(nx, pos.y, cfg::ENEMY_RADIUS)) pos.x = nx;
    if (!map.collides(pos.x, ny, cfg::ENEMY_RADIUS)) pos.y = ny;
}
