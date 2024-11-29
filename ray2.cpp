#include "raylib.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdlib>

const int SCREEN_WIDTH = 720;
const int SCREEN_HEIGHT = 600;
const int TILE_WIDTH = 80;
const int TILE_HEIGHT = 10;
const int TILE_SPEED = 3;
const int PLAYER_RADIUS = 15;
const int PLAYER_SPEED = 5;
const float GRAVITY = 0.5f;
const float SPAWN_INTERVAL = 0.5f;
const int MAX_LIFELINES = 5;

struct Tile {
    int x, y, width, height;
    Color color;
    bool hasLifeline; 
};

struct Player {
    int x, y, radius;
    bool isOnTile;
    float velocityY;
    Tile* currentTile;
};

struct Trail {
    int x,y;
    float alpha;
};
std::vector<Trail> playerTrail;


float PredictableRandom(unsigned int& seed) {
    seed = seed * 1664525 + 1013904223; 
    return fmodf(seed / static_cast<float>(0xFFFFFFFF), 1.0f);

}
Color RandomColor(unsigned int& seed) {
    return Color{
        static_cast<unsigned char>(PredictableRandom(seed) * 255),
        static_cast<unsigned char>(PredictableRandom(seed) * 255),
        static_cast<unsigned char>(PredictableRandom(seed) * 255),
        255
    };
}



Tile GenerateTile(unsigned int& seed, float& lastX, bool isFirstTile = false) {
    if (isFirstTile) {
        return {SCREEN_WIDTH / 2 - TILE_WIDTH / 2, SCREEN_HEIGHT, TILE_WIDTH, TILE_HEIGHT, BLUE, false};
    }
    
    float offsetFactor = (PredictableRandom(seed) * 2 - 1) * 200.0f;
    float newX = std::clamp(lastX + offsetFactor, 0.0f, static_cast<float>(SCREEN_WIDTH - TILE_WIDTH));
    lastX = newX;

    // Add lifeline randomly to tiles
    bool hasLifeline = (rand() % 10) == 0; // 10% chance for lifeline
    return {static_cast<int>(newX), SCREEN_HEIGHT, TILE_WIDTH, TILE_HEIGHT, RandomColor(seed), hasLifeline};
}

bool CheckCollisionWithTile(const Player& player, const Tile& tile) {
    return (player.x + player.radius > tile.x &&
            player.x - player.radius < tile.x + tile.width &&
            player.y + player.radius > tile.y &&
            player.y - player.radius < tile.y + tile.height);
}

void UpdatePlayerTrail(Player player){
Trail newTrail = {player.x, player.y, 1.0f};
    playerTrail.push_back(newTrail);
  
    for (auto & t : playerTrail) {
        t.alpha -= 0.05f;
    }
       playerTrail.erase(std::remove_if(playerTrail.begin(), playerTrail.end(),
                                     [](const Trail& t) { return t.alpha <= 0.0f; }),
                      playerTrail.end());

} 

void DrawPlayerTrail() {
    for (const auto& t : playerTrail) {
        DrawCircle(t.x, t.y, PLAYER_RADIUS * t.alpha, Fade(GREEN, t.alpha));
    }
}
void UpdatePlayerMovement(Player& player) {
    if (IsKeyDown(KEY_RIGHT)) {
        player.x = std::min(player.x + PLAYER_SPEED, SCREEN_WIDTH - player.radius);
    }
    if (IsKeyDown(KEY_LEFT)) {
        player.x = std::max(player.x - PLAYER_SPEED, player.radius);
    }
}

void ApplyGravity(Player& player,std::vector<Tile>& tiles, bool& gameOver, int& lifelines) {
    if (tiles.size() < 4) return; 

    player.velocityY += GRAVITY;  
    player.y += static_cast<int>(player.velocityY);  
    bool landed = false;
    for ( auto& tile : tiles) {
        if (CheckCollisionWithTile(player, tile) && player.velocityY > 0) {
            player.y = tile.y - player.radius;  
            player.velocityY = 0;    
            player.isOnTile = true;
            player.currentTile = &tile;
           
            if (tile.hasLifeline) {
                lifelines = std::min(lifelines + 1, MAX_LIFELINES);  
                 tile.hasLifeline = false; // not using const become we need to modify tile lifetime has read to add only +1 
            }
            landed = true;
            break;
        }
    }

    if (!landed) {
        player.isOnTile = false;
    }
    if ( player.y -player.radius < 0 || player.y - player.radius > SCREEN_HEIGHT) {

      if (lifelines > 0) {
            lifelines--;
            player.y = SCREEN_HEIGHT - SCREEN_HEIGHT/2;
            player.velocityY = 0;
            player.isOnTile = false;
            player.currentTile = nullptr;
        } else {
            gameOver = true;
        }
    }
}

void UpdateTiles(std::vector<Tile>& tiles, Player& player, bool& gameOver) {
    for (auto& tile : tiles) {
        tile.y -= TILE_SPEED; 
        if (player.isOnTile && player.currentTile == &tile) {
            player.y -= TILE_SPEED;
        }
    }
    tiles.erase(
        std::remove_if(tiles.begin(), tiles.end(), [](const Tile& tile) { return tile.y + TILE_HEIGHT < 0; }),
        tiles.end()
    );

}

void DrawGame(const Player& player, const std::vector<Tile>& tiles, int score, int lifelines) {
    BeginDrawing();
    ClearBackground(ORANGE);

    DrawPlayerTrail(); // Draw trail before the player
    DrawCircle(player.x, player.y, player.radius, GREEN); 
    for (const auto& tile : tiles) {
        DrawRectangle(tile.x, tile.y, tile.width, tile.height, tile.color);
        if (tile.hasLifeline) {
            DrawText("L", tile.x + 25, tile.y -12, 20, RED);
        }
    }

    DrawText(TextFormat("Score: %d", score), 10, 40, 20, GREEN);
    DrawText(TextFormat("Lifelines: %d", lifelines), 10, 70, 20, GREEN);

    EndDrawing();
}

void RestartGame(Player& player, std::vector<Tile>& tiles, unsigned int& randomSeed, float& lastTileX, int& score, bool& gameOver, int& lifelines) {
    player.x = SCREEN_WIDTH / 2;
    player.y = SCREEN_HEIGHT - SCREEN_HEIGHT/2;
    player.velocityY = 0;
    player.isOnTile = false;
    player.currentTile = nullptr;
    
    tiles.clear();
    tiles.push_back(GenerateTile(randomSeed, lastTileX, true));      
    score = 0;
    gameOver = false;
    lifelines = MAX_LIFELINES; // Reset lifelines
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "no name decided ");
    SetTargetFPS(60);

    Texture2D background = LoadTexture("lol.jpg");
    unsigned int randomSeed = 12345;
    float lastTileX = SCREEN_WIDTH / 2.0f;

    std::vector<Tile> tiles;
    float spawnTimer = 0.0f;
    int lifelines = MAX_LIFELINES;

    Player player = {SCREEN_WIDTH / 2, SCREEN_HEIGHT - SCREEN_HEIGHT/2, PLAYER_RADIUS, false, 0.0f, nullptr};
    tiles.push_back(GenerateTile(randomSeed, lastTileX, true));  
    int score = 0;
    bool gameOver = false;

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        spawnTimer += deltaTime;

        if (spawnTimer >= SPAWN_INTERVAL) {
            spawnTimer = 0.0f;
            tiles.push_back(GenerateTile(randomSeed, lastTileX));         
        }

        UpdatePlayerMovement(player);
        ApplyGravity(player, tiles, gameOver, lifelines);
        UpdateTiles(tiles, player, gameOver);
        UpdatePlayerTrail(player);
       if (!gameOver){
        score += static_cast<int>(deltaTime * 100);  
        }



        DrawTexture(background, 0, 0, WHITE);


        DrawGame(player, tiles, score, lifelines);

        if (gameOver) {
            DrawText("GAME OVER! Press 'R' to Restart", SCREEN_WIDTH / 4, SCREEN_HEIGHT / 2, 30, RED);

            if (IsKeyPressed(KEY_R)) {
                RestartGame(player, tiles, randomSeed, lastTileX, score, gameOver, lifelines);
            }
        }

    
    }

  

    CloseWindow();
    return 0;
}

