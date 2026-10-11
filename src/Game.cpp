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
#include "Game.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>

#include "Audio.h"
#include "Config.h"

namespace {

struct Room { int x, y, w, h; };

void carveH(std::vector<std::string>& g, int x1, int x2, int y)
{
    for (int x = std::min(x1, x2); x <= std::max(x1, x2); ++x) g[y][x] = '.';
}

void carveV(std::vector<std::string>& g, int y1, int y2, int x)
{
    for (int y = std::min(y1, y2); y <= std::max(y1, y2); ++y) g[y][x] = '.';
}

// Раскидываем `count` символов `c` по случайным свободным клеткам; при желании
// держимся не ближе `minDist` клеток от (px, py).
void placeRandom(std::vector<std::string>& g, char c, int count, int px, int py, float minDist)
{
    const int h = static_cast<int>(g.size());
    const int w = static_cast<int>(g[0].size());
    int placed = 0;
    for (int attempt = 0; attempt < 4000 && placed < count; ++attempt) {
        const int x = GetRandomValue(1, w - 2);
        const int y = GetRandomValue(1, h - 2);
        if (g[y][x] != '.') continue;
        if (minDist > 0.0f) {
            const float dx = static_cast<float>(x - px);
            const float dy = static_cast<float>(y - py);
            if (dx * dx + dy * dy < minDist * minDist) continue;
        }
        g[y][x] = c;
        ++placed;
    }
}

// Собираем свежий уровень: случайные комнаты + коридоры + игрок + враги + пикапы.
std::vector<std::string> generateLevel()
{
    const int w = GetRandomValue(18, 26);
    const int h = GetRandomValue(18, 26);
    std::vector<std::string> g(h, std::string(w, '#'));

    // 1) Рубим случайные комнаты топором.
    const int roomCount = GetRandomValue(4, 7);
    std::vector<Room> rooms;
    rooms.reserve(roomCount);
    for (int i = 0; i < roomCount; ++i) {
        const int rw = GetRandomValue(3, 6);
        const int rh = GetRandomValue(3, 6);
        const int rx = GetRandomValue(1, w - rw - 1);
        const int ry = GetRandomValue(1, h - rh - 1);
        rooms.push_back({rx, ry, rw, rh});
        for (int y = ry; y < ry + rh; ++y) {
            for (int x = rx; x < rx + rw; ++x) {
                g[y][x] = '.';
            }
        }
    }

    // 2) Соединяем соседние комнаты Г-коридором — карта всегда проходима.
    for (size_t i = 1; i < rooms.size(); ++i) {
        const int x1 = rooms[i - 1].x + rooms[i - 1].w / 2;
        const int y1 = rooms[i - 1].y + rooms[i - 1].h / 2;
        const int x2 = rooms[i].x + rooms[i].w / 2;
        const int y2 = rooms[i].y + rooms[i].h / 2;
        if (GetRandomValue(0, 1) == 0) {
            carveH(g, x1, x2, y1);
            carveV(g, y1, y2, x2);
        } else {
            carveV(g, y1, y2, x1);
            carveH(g, x1, x2, y2);
        }
    }

    // 3) Игрок появляется в первой комнате.
    const int px = rooms[0].x + rooms[0].w / 2;
    const int py = rooms[0].y + rooms[0].h / 2;
    g[py][px] = 'P';

    // 4) Враги (обязательно подальше от игрока) и пикапы.
    placeRandom(g, 'E', GetRandomValue(4, 9), px, py, cfg::MIN_ENEMY_SPAWN_DIST);
    placeRandom(g, 'H', GetRandomValue(2, 4), px, py, 0.0f);
    placeRandom(g, 'A', GetRandomValue(1, 3), px, py, 0.0f);

    return g;
}

// Равномерно в [-1, 1] — для разброса дроби.
float randBilateral()
{
    return (GetRandomValue(0, 2000) / 1000.0f) - 1.0f;
}

} // namespace

Game::Game() = default;

void Game::run()
{
    InitWindow(cfg::SCREEN_WIDTH, cfg::SCREEN_HEIGHT, "idk");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);   // ESC обрабатываем сами

    audio::init();
    textures_.load();

    // Сидим ГСЧ от системных часов — каждый запуск даёт новую карту.
    SetRandomSeed(static_cast<unsigned int>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()));

    newLevel();
    DisableCursor();

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ESCAPE)) break;

        // Клэмпим dt, чтобы после альт-таба не пролететь сквозь стену.
        const float dt = std::min(GetFrameTime(), 0.05f);

        update(dt);

        BeginDrawing();
        ClearBackground(BLACK);
        draw();
        EndDrawing();
    }

    ShowCursor();
    textures_.unload();
    audio::shutdown();
    CloseWindow();
}

void Game::newLevel()
{
    ++runNumber_;
    map_.load(generateLevel());
    player_.spawn(map_.playerStart(), map_.playerAngle());

    enemies_.clear();
    for (const auto& p : map_.enemyStarts()) {
        enemies_.emplace_back(p);
    }

    pickups_.clear();
    for (const auto& ps : map_.pickupStarts()) {
        pickups_.push_back({ps.pos, ps.type, 0.0f});
    }

    tracers_.clear();
    state_ = State::Play;
}

void Game::update(float dt)
{
    // Трассеры гаснут всегда, даже поверх оверлея.
    for (auto it = tracers_.begin(); it != tracers_.end();) {
        it->life -= dt;
        if (it->life <= 0.0f) it = tracers_.erase(it);
        else ++it;
    }

    switch (state_) {
        case State::Play: {
            player_.update(map_, dt);
            for (auto& e : enemies_) {
                e.update(map_, player_, enemies_, dt);
            }
            updatePickups(dt);
            handleWeaponSwitch();
            if (IsKeyPressed(KEY_R)) player_.startReload();
            handleShooting();

            if (!player_.alive()) {
                state_ = State::GameOver;
            } else if (std::all_of(enemies_.begin(), enemies_.end(),
                                   [](const Enemy& e) { return !e.alive(); })) {
                state_ = State::LevelClear;
            }
            break;
        }
        case State::LevelClear:
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                newLevel();
            }
            break;
        case State::GameOver:
            if (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_ENTER)) {
                newLevel();
            }
            break;
    }
}

void Game::handleWeaponSwitch()
{
    for (int i = 0; i < cfg::WEAPON_COUNT; ++i) {
        if (IsKeyPressed(KEY_ONE + i)) {
            switchWeapon(i);
            return;
        }
    }

    const float wheel = GetMouseWheelMove();
    if (wheel > 0.0f) {
        switchWeapon((player_.weapon + 1) % cfg::WEAPON_COUNT);
    } else if (wheel < 0.0f) {
        switchWeapon((player_.weapon + cfg::WEAPON_COUNT - 1) % cfg::WEAPON_COUNT);
    }
}

void Game::switchWeapon(int index)
{
    if (index == player_.weapon) return;
    player_.weapon = index;
    player_.reloadTimer = 0.0f;   // переключение сбрасывает перезарядку
    player_.fireCooldown = std::max(player_.fireCooldown, 0.15f);
    audio::playSwitch();
}

void Game::handleShooting()
{
    const int w = player_.weapon;

    const bool held = IsMouseButtonDown(MOUSE_BUTTON_LEFT) ||
                      IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_LEFT_CONTROL);
    const bool pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
                         IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_LEFT_CONTROL);
    const bool wantFire = cfg::WEAPON_AUTO[w] ? held : pressed;
    if (!wantFire) return;

    if (player_.isReloading()) return;

    if (player_.ammo[w] <= 0) {
        // Автоперезарядка; заряжать нечем — сухой щелчок.
        if (!player_.startReload() && pressed) audio::playEmpty();
        return;
    }

    if (player_.fireCooldown > 0.0f) return;

    player_.fireCooldown = cfg::WEAPON_COOLDOWN[w];
    player_.muzzleTimer = (w == 1) ? 0.14f : 0.1f;
    player_.ammo[w]--;
    audio::playShoot(w);

    const Vector2 muzzle{cfg::SCREEN_WIDTH / 2.0f, cfg::SCREEN_HEIGHT - 110.0f};

    bool anyHit = false;
    for (int i = 0; i < cfg::WEAPON_PELLETS[w]; ++i) {
        float offset = 0.0f;
        if (cfg::WEAPON_SPREAD[w] > 0.0f) {
            offset = randBilateral() * cfg::WEAPON_SPREAD[w] * 0.5f;
        }

        const float angle = player_.angle + offset;
        Vector2 endWorld;
        shootRay(angle, cfg::WEAPON_DAMAGE[w], anyHit, endWorld);

        const Vector2 endScreen = projectPoint(endWorld);
        tracers_.push_back({muzzle, endScreen, cfg::TRACER_LIFE, cfg::TRACER_LIFE});
    }

    if (anyHit) audio::playHit();

    // Пустой магазин — перезаряжаемся сами.
    if (player_.ammo[w] <= 0) player_.startReload();
}

void Game::shootRay(float angle, float damage, bool& anyHit, Vector2& endPoint)
{
    const Vector2 d{std::cos(angle), std::sin(angle)};

    // Ближайший враг, в чей (щедрый) круг попал луч и кого не закрывает стена.
    Enemy* best = nullptr;
    float bestT = cfg::WEAPON_RANGE;

    for (auto& e : enemies_) {
        if (!e.alive()) continue;

        const float ex = e.pos.x - player_.pos.x;
        const float ey = e.pos.y - player_.pos.y;
        const float t = ex * d.x + ey * d.y;   // расстояние вдоль луча
        if (t <= 0.0f || t > cfg::WEAPON_RANGE) continue;

        const float cpx = player_.pos.x + d.x * t;
        const float cpy = player_.pos.y + d.y * t;
        const float ox = cpx - e.pos.x;
        const float oy = cpy - e.pos.y;
        const float hitRadius = cfg::ENEMY_RADIUS + cfg::HITBOX_PAD;
        if (ox * ox + oy * oy > hitRadius * hitRadius) continue;

        if (!map_.lineOfSight(player_.pos.x, player_.pos.y, e.pos.x, e.pos.y)) continue;

        if (t < bestT) {
            bestT = t;
            best = &e;
        }
    }

    // Куда летит трассер: во врага, в стену или в никуда (максимум дальности).
    Vector2 wallPoint{};
    float wallDist = map_.raycast(player_.pos.x, player_.pos.y, d.x, d.y, wallPoint);
    if (wallDist < 0.0f || wallDist > cfg::WEAPON_RANGE) wallDist = cfg::WEAPON_RANGE;

    if (best) {
        endPoint = best->pos;
    } else if (wallDist < cfg::WEAPON_RANGE) {
        endPoint = wallPoint;
    } else {
        endPoint = {player_.pos.x + d.x * cfg::WEAPON_RANGE,
                    player_.pos.y + d.y * cfg::WEAPON_RANGE};
    }

    if (best) {
        const bool wasAlive = best->alive();
        best->takeDamage(damage);
        anyHit = true;
        if (wasAlive && !best->alive()) player_.kills++;
    }
}

// Проецируем мировую точку на экран (обратное преобразование камеры).
Vector2 Game::projectPoint(Vector2 world) const
{
    const float relX = world.x - player_.pos.x;
    const float relY = world.y - player_.pos.y;
    const Vector2 d = player_.dir();
    const Vector2 pl = player_.plane();
    const float invDet = 1.0f / (pl.x * d.y - d.x * pl.y);
    const float tX = invDet * (d.y * relX - d.x * relY);
    const float tY = invDet * (-pl.y * relX + pl.x * relY);
    const float safeY = std::max(tY, 0.02f);
    const float sx = (cfg::SCREEN_WIDTH / 2.0f) * (1.0f + tX / safeY);
    return {sx, cfg::SCREEN_HEIGHT / 2.0f};
}

void Game::updatePickups(float dt)
{
    for (auto& pk : pickups_) {
        pk.bob += dt * 4.0f;
    }

    for (auto it = pickups_.begin(); it != pickups_.end();) {
        const float dx = it->pos.x - player_.pos.x;
        const float dy = it->pos.y - player_.pos.y;
        if (dx * dx + dy * dy <= cfg::PICKUP_COLLECT_RADIUS * cfg::PICKUP_COLLECT_RADIUS) {
            if (it->type == 0) {
                player_.health = std::min(cfg::PLAYER_MAX_HP, player_.health + cfg::HEALTH_PICKUP_AMOUNT);
                audio::playPickupHealth();
            } else {
                player_.armor = std::min(cfg::PLAYER_MAX_ARMOR, player_.armor + cfg::ARMOR_PICKUP_AMOUNT);
                audio::playPickupArmor();
            }
            it = pickups_.erase(it);
        } else {
            ++it;
        }
    }
}

void Game::draw()
{
    renderer_.render(map_, player_, enemies_, pickups_, textures_);
    drawTracers();
    drawGun();
    drawHUD();
    drawMinimap();

    switch (state_) {
        case State::LevelClear:
            drawOverlay("AREA CLEAR", "Press ENTER for the next area", GREEN);
            break;
        case State::GameOver:
            drawOverlay("YOU DIED", "Press R to try again", RED);
            break;
        default:
            break;
    }

    if (player_.damageFlash > 0.0f) {
        const unsigned char a = static_cast<unsigned char>(player_.damageFlash * 60.0f);
        DrawRectangle(0, 0, cfg::SCREEN_WIDTH, cfg::SCREEN_HEIGHT, Color{255, 0, 0, a});
    }
}

void Game::drawTracers()
{
    for (const auto& t : tracers_) {
        const float a = t.life / t.maxLife;
        DrawLineEx(t.start, t.end, 6.0f, Color{255, 190, 60, static_cast<unsigned char>(90.0f * a)});
        DrawLineEx(t.start, t.end, 2.0f, Color{255, 240, 150, static_cast<unsigned char>(255.0f * a)});
    }
}

void Game::drawHUD()
{
    const int w = player_.weapon;

    // ХП-бар (слева внизу).
    const int bx = 20;
    const int by = cfg::SCREEN_HEIGHT - 54;
    const int bw = 220;
    const int bh = 26;
    DrawRectangle(bx, by, bw, bh, Color{30, 30, 30, 210});
    const float hf = std::clamp(player_.health / cfg::PLAYER_MAX_HP, 0.0f, 1.0f);
    DrawRectangle(bx + 3, by + 3, static_cast<int>((bw - 6) * hf), bh - 6, RED);
    DrawRectangleLines(bx, by, bw, bh, Color{255, 255, 255, 120});
    DrawText(TextFormat("HP %d", static_cast<int>(player_.health)), bx + 8, by + 5, 14, WHITE);

    // Броня (синяя) над ХП.
    const int ay = by - 34;
    DrawRectangle(bx, ay, bw, bh, Color{20, 25, 40, 210});
    const float af = std::clamp(player_.armor / cfg::PLAYER_MAX_ARMOR, 0.0f, 1.0f);
    DrawRectangle(bx + 3, ay + 3, static_cast<int>((bw - 6) * af), bh - 6, Color{70, 130, 220, 255});
    DrawRectangleLines(bx, ay, bw, bh, Color{255, 255, 255, 120});
    DrawText(TextFormat("ARMOR %d", static_cast<int>(player_.armor)), bx + 8, ay + 5, 14, WHITE);

    // Статус (слева вверху).
    DrawText(TextFormat("RUN %d", runNumber_), 20, 20, 26, WHITE);
    const int remaining = static_cast<int>(std::count_if(
        enemies_.begin(), enemies_.end(), [](const Enemy& e) { return e.alive(); }));
    DrawText(TextFormat("ENEMIES %d", remaining), 20, 52, 20, WHITE);
    DrawText(TextFormat("KILLS %d", player_.kills), 20, 78, 20, WHITE);

    // Оружие + патроны (внизу по центру).
    const char* wname = cfg::WEAPON_NAME[w];
    const int wnw = MeasureText(wname, 24);
    DrawText(wname, cfg::SCREEN_WIDTH / 2 - wnw / 2, cfg::SCREEN_HEIGHT - 172, 24, Color{255, 200, 80, 255});

    const char* ammoStr = cfg::WEAPON_INFINITE[w]
        ? TextFormat("%d / INF", player_.ammo[w])
        : TextFormat("%d / %d", player_.ammo[w], player_.reserve[w]);
    const int aw = MeasureText(ammoStr, 22);
    DrawText(ammoStr, cfg::SCREEN_WIDTH / 2 - aw / 2, cfg::SCREEN_HEIGHT - 146, 22, WHITE);

    if (player_.isReloading()) {
        const char* rs = "RELOADING...";
        const int rw = MeasureText(rs, 18);
        DrawText(rs, cfg::SCREEN_WIDTH / 2 - rw / 2, cfg::SCREEN_HEIGHT - 120, 18, Color{255, 120, 120, 255});
    } else {
        const char* hint = "1/2/3 or wheel: weapon   R: reload";
        const int hw = MeasureText(hint, 15);
        DrawText(hint, cfg::SCREEN_WIDTH / 2 - hw / 2, cfg::SCREEN_HEIGHT - 120, 15, Color{255, 255, 255, 170});
    }

    // Прицел.
    const int cx = cfg::SCREEN_WIDTH / 2;
    const int cy = cfg::SCREEN_HEIGHT / 2;
    DrawLine(cx - 9, cy, cx + 9, cy, Color{0, 255, 0, 200});
    DrawLine(cx, cy - 9, cx, cy + 9, Color{0, 255, 0, 200});
}

void Game::drawMinimap()
{
    const int cell = cfg::MINIMAP_CELL;
    const int w = map_.width() * cell;
    const int h = map_.height() * cell;
    const int ox = cfg::SCREEN_WIDTH - w - 16;
    const int oy = 16;

    // Полупрозрачная подложка.
    DrawRectangle(ox - 4, oy - 4, w + 8, h + 8, Color{0, 0, 0, 130});
    DrawRectangle(ox, oy, w, h, Color{26, 26, 30, 210});

    // Стены.
    for (int y = 0; y < map_.height(); ++y) {
        for (int x = 0; x < map_.width(); ++x) {
            if (!map_.isWall(x, y)) continue;
            const int t = map_.wallType(x, y);
            Color c = Color{150, 150, 155, 255};
            if (t == 2) c = Color{170, 90, 70, 255};
            else if (t == 3) c = Color{110, 130, 150, 255};
            else if (t == 4) c = Color{150, 105, 65, 255};
            DrawRectangle(ox + x * cell, oy + y * cell, cell, cell, c);
        }
    }

    // Пикапы (зелёный — аптечка, синий — броня).
    for (const auto& pk : pickups_) {
        const Color c = (pk.type == 0) ? Color{80, 230, 80, 255} : Color{90, 160, 255, 255};
        DrawCircle(ox + static_cast<int>(pk.pos.x * cell),
                   oy + static_cast<int>(pk.pos.y * cell), 2, c);
    }

    // Враги (красные точки).
    for (const auto& e : enemies_) {
        if (!e.alive()) continue;
        DrawCircle(ox + static_cast<int>(e.pos.x * cell),
                   oy + static_cast<int>(e.pos.y * cell),
                   cell / 2, Color{230, 50, 50, 255});
    }

    // Игрок: точка + направление.
    const int px = ox + static_cast<int>(player_.pos.x * cell);
    const int py = oy + static_cast<int>(player_.pos.y * cell);
    const Vector2 d = player_.dir();
    DrawLine(px, py, px + static_cast<int>(d.x * cell * 2), py + static_cast<int>(d.y * cell * 2),
             Color{255, 255, 0, 255});
    DrawCircle(px, py, cell / 2, Color{255, 255, 0, 255});

    DrawRectangleLines(ox, oy, w, h, Color{255, 255, 255, 90});
}

void Game::drawGun()
{
    const int cx = cfg::SCREEN_WIDTH / 2;
    const int baseY = cfg::SCREEN_HEIGHT;

    switch (player_.weapon) {
        case 0: {   // пистолет
            DrawRectangle(cx - 14, baseY - 92, 28, 92, Color{42, 42, 46, 255});
            DrawRectangle(cx - 22, baseY - 58, 44, 58, Color{58, 58, 63, 255});
            DrawRectangle(cx - 24, baseY - 60, 48, 10, Color{72, 72, 78, 255});
            DrawRectangle(cx - 8, baseY - 28, 16, 28, Color{48, 48, 52, 255});
            break;
        }
        case 1: {   // дробовик
            DrawRectangle(cx - 10, baseY - 130, 20, 130, Color{70, 48, 30, 255});
            DrawRectangle(cx - 10, baseY - 130, 20, 6, Color{120, 120, 128, 255});
            DrawRectangle(cx - 26, baseY - 60, 52, 60, Color{90, 60, 36, 255});
            DrawRectangle(cx - 26, baseY - 92, 52, 32, Color{110, 74, 44, 255});
            DrawRectangle(cx - 8, baseY - 26, 16, 26, Color{80, 54, 32, 255});
            break;
        }
        case 2: {   // SMG
            DrawRectangle(cx - 10, baseY - 70, 20, 70, Color{45, 45, 50, 255});
            DrawRectangle(cx - 20, baseY - 50, 40, 50, Color{60, 60, 66, 255});
            DrawRectangle(cx - 12, baseY - 46, 24, 30, Color{80, 80, 88, 255});
            DrawRectangle(cx - 6, baseY - 22, 12, 22, Color{48, 48, 52, 255});
            break;
        }
    }

    // Вспышка (у дробовика побольше).
    if (player_.muzzleTimer > 0.0f) {
        const float dur = (player_.weapon == 1) ? 0.14f : 0.1f;
        const float a = player_.muzzleTimer / dur;
        const int r = static_cast<int>(26.0f + a * 48.0f);
        const int fy = (player_.weapon == 1) ? baseY - 140 : baseY - 102;
        DrawCircle(cx, fy, r, Color{255, 220, 80, static_cast<unsigned char>(190.0f * a)});
        DrawCircle(cx, fy, r / 2, Color{255, 255, 205, static_cast<unsigned char>(255.0f * a)});
    }
}

void Game::drawOverlay(const char* title, const char* subtitle, Color color)
{
    DrawRectangle(0, 0, cfg::SCREEN_WIDTH, cfg::SCREEN_HEIGHT, Color{0, 0, 0, 150});

    const int tw = MeasureText(title, 60);
    DrawText(title, cfg::SCREEN_WIDTH / 2 - tw / 2, cfg::SCREEN_HEIGHT / 2 - 70, 60, color);

    const int sw = MeasureText(subtitle, 24);
    DrawText(subtitle, cfg::SCREEN_WIDTH / 2 - sw / 2, cfg::SCREEN_HEIGHT / 2 + 20, 24, WHITE);
}
