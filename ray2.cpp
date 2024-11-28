#include "raylib.h"
#include <vector>
#include <cmath>
#include <algorithm>

const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 600;
const int TILE_WIDTH = 80;
const int TILE_HEIGHT = 10;
const int TILE_SPEED = 1;
const int PLAYER_RADIUS = 15;
const int PLAYER_SPEED = 5;
const float GRAVITY = 0.5f;
const float JUMP_POWER = -10.0f;
const float SPAWN_INTERVAL = 0.5f;

struct Tile {
    int x, y, width, height;
    Color color;
};

struct Player {
    int x, y, radius;
    bool isJumping;
    float velocityY;
    bool isOnTile;
    Tile* currentTile;
};

float predictableRandom(unsigned int& seed) {
    seed = seed * 1664525 + 1013904223;  // Linear congruential generator
    return fmodf(seed / static_cast<float>(0xFFFFFFFF), 1.0f);
}

Tile generateTile(unsigned int& seed, float& lastX) {
    float offsetFactor = (predictableRandom(seed) * 2 - 1) * 200.0f;
    
    float newX = lastX + offsetFactor;
    newX = std::max(0.0f, std::min(static_cast<float>(SCREEN_WIDTH - TILE_WIDTH), newX));
    
    lastX = newX;
    
    return {
        static_cast<int>(newX),  // x position
        -TILE_HEIGHT,             // y position (start above screen)
        TILE_WIDTH,               
        TILE_HEIGHT,              
        BLUE       
    }
;
}

// Check collision between player and tile
bool checkCollisionWithTile(const Player& player, const Tile& tile) {
    return (player.x + player.radius > tile.x && 
            player.x - player.radius < tile.x + tile.width &&
            player.y + player.radius > tile.y && 
            player.y - player.radius < tile.y + tile.height);
}

int main(void) {
    // Initialize window and game state
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Predictable Tiles Game");
    SetTargetFPS(60);

    // Seed for predictable randomness
    unsigned int randomSeed = 12345;  // Fixed seed for consistent levels
    float lastTileX = SCREEN_WIDTH / 2.0f;

    // Initialize game objects
    std::vector<Tile> tiles;
    float spawnTimer = 0.0f;
    
    Player player = {
        SCREEN_WIDTH / 2,        // x position
        SCREEN_HEIGHT - 100,     // y position
        PLAYER_RADIUS,           // radius
        false,                   // isJumping
        0.0f,                    // velocityY
        false,                   // isOnTile
        nullptr                  // currentTile
    };

    // Main game loop
    while (!WindowShouldClose()) {
        // Update spawn timer
        spawnTimer += GetFrameTime();

        // Spawn new tiles
        if (spawnTimer >= SPAWN_INTERVAL) {
            spawnTimer = 0.0f;
            tiles.push_back(generateTile(randomSeed, lastTileX));
        }

        // Horizontal movement
        if (IsKeyDown(KEY_RIGHT)) {
            player.x += PLAYER_SPEED;
        }
        if (IsKeyDown(KEY_LEFT)) {
            player.x -= PLAYER_SPEED;
        }

        // Jumping mechanics
        if (IsKeyPressed(KEY_SPACE) && !player.isJumping) {
            player.velocityY = JUMP_POWER;
            player.isJumping = true;
        }

        // Apply gravity and handle jumping/falling
        if (player.isJumping) {
            player.velocityY += GRAVITY;
            player.y += static_cast<int>(player.velocityY);

            // Check tile collisions
            bool landed = false;
            for (auto& tile : tiles) {
                if (checkCollisionWithTile(player, tile) && player.velocityY > 0) {
                    player.y = tile.y - player.radius;
                    player.isJumping = false;
                    player.velocityY = 0;
                    player.isOnTile = true;
                    player.currentTile = &tile;
                    landed = true;
                    break;
                }
            }

            // Ground or screen bottom handling
            if (!landed && player.y > SCREEN_HEIGHT) {
                player.y = SCREEN_HEIGHT;
                player.isJumping = false;
                player.velocityY = 0;
                player.isOnTile = false;
                player.currentTile = nullptr;
            }
        }

        // Update and remove tiles
        for (auto& tile : tiles) {
            tile.y += TILE_SPEED;

            // Move player with tile if standing on it
            if (player.isOnTile && player.currentTile == &tile) {
                player.y += TILE_SPEED;
            }
        }

        // Remove off-screen tiles
        tiles.erase(
            std::remove_if(tiles.begin(), tiles.end(), 
                [](const Tile& t) { return t.y > SCREEN_HEIGHT; }), 
            tiles.end()
        );

        // Drawing
        BeginDrawing();
        ClearBackground(ORANGE);

        // Draw player
        DrawCircle(player.x, player.y, player.radius, GREEN);

        // Draw tiles
        for (const auto& tile : tiles) {
            DrawRectangle(tile.x, tile.y, tile.width, tile.height, tile.color);
        }

        // Draw title
        DrawText("Predictable Tiles Game", 10, 10, 20, GREEN);

        EndDrawing();
    }

    // Clean up
    CloseWindow();
    return 0;
}
