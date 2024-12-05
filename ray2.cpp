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
    bool hasbomb; 
};

struct Particle {
    Vector2 position;
    Vector2 velocity;
    float alpha;
    float size;
    Color color;
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
    Color color;
};
 struct bomb {
    int x,y ,radius;
    Color color ;
    float vecocity_x ;
};
std::vector<Trail> playerTrail;
std::vector<Particle> particles;


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
    bool hasbomb = (rand() % 5) == 0; // 10% chance for
    // Add lifeline randomly to tiles
    bool hasLifeline = (rand() % 10) == 0; // 10% chance for lifeline
    return {static_cast<int>(newX), SCREEN_HEIGHT, TILE_WIDTH, TILE_HEIGHT, RandomColor(seed), hasLifeline , hasbomb };
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

void UpdateParticles(float deltaTime) {
    for (auto& p : particles) {
        p.position.x += p.velocity.x * deltaTime;
        p.position.y += p.velocity.y * deltaTime;
        p.alpha -= deltaTime * 0.5f;  // Slow down fade
        p.size -= deltaTime * 2.0f;   // Slow down shrink
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
                                   [](const Particle& p) { return p.alpha <= 0.0f || p.size <= 0.0f; }),
                    particles.end());
}

void GenerateExplosion(Vector2 position, int count) {
    for (int i = 0; i < count; i++) {
        Particle p;
        float angle = static_cast<float>(rand() % 360) * DEG2RAD; // Random direction
        float speed = static_cast<float>((rand() % 50) + 50) / 10.0f; // Random speed
        p.position = position;
        p.velocity = { static_cast<float>(cos(angle)) * speed, static_cast<float>(sin(angle)) * speed };
        p.alpha = 1.0f;
        p.size = 8.0f + static_cast<float>(rand() % 4); // Base size
        p.color = (i % 2 == 0) ? RED : YELLOW;
        particles.push_back(p);
    }
}


void DrawParticles() {
    for (const auto& p : particles) {
        DrawCircleV(p.position, p.size, Fade(p.color, p.alpha));
    }
}
void CheckBombCollision(Player& player, std::vector<bomb>& bombs, bool& gameOver, int& lifelines) {
    for (const auto& b : bombs) {
        if (CheckCollisionCircles(Vector2{static_cast<float>(player.x), static_cast<float>(player.y)}, player.radius,
                                  Vector2{static_cast<float>(b.x), static_cast<float>(b.y)}, b.radius)) {
            GenerateExplosion({ static_cast<float>(b.x), static_cast<float>(b.y) }, 20); // Trigger explosion
            if (lifelines > 0) {
                lifelines--;
                player.y = SCREEN_HEIGHT - SCREEN_HEIGHT / 2;
                player.velocityY = 0;
                player.isOnTile = false;
                player.currentTile = nullptr;
            } else {
                gameOver = true;
            }
        }
    }
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

std::vector<bomb> bombs;

void drawbomb() {
    static Texture2D bomb = LoadTexture("bomb2.png");

    for (const auto& b : bombs) {

            // Draw the bomb texture centered at the bomb's position
            DrawTexture(bomb, 
                b.x - bomb.width/2, 
                b.y - bomb.height/2, 
                b.color);
      
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

    DrawPlayerTrail();
    
       DrawParticles();
       drawbomb(); // Render particles here
     // Draw trail before the player
    DrawCircle(player.x, player.y, player.radius, GREEN); 
    for (const auto& tile : tiles) {
        DrawRectangle(tile.x, tile.y, tile.width, tile.height, tile.color);
    static Texture2D lifelineimage=   LoadTexture("newgojo.jpg");
        if (tile.hasLifeline) {
            DrawTexture( lifelineimage, tile.x + 25, tile.y -12, RED);
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



void UpdateBombs(std::vector<bomb>& bombs, float deltaTime) {
    for (auto& b : bombs) {
        b.y += static_cast<int>(b.vecocity_x * deltaTime);
        if (b.y > SCREEN_HEIGHT) {
            b.y = 0;
            b.x = rand() % SCREEN_WIDTH;
        }
    }
}



void SpawnBomb(std::vector<bomb>& bombs, unsigned int& randomSeed) {
    bomb newBomb;
    newBomb.x = rand() % SCREEN_WIDTH;
    newBomb.y = 0;
    newBomb.radius = 10;
    newBomb.color = RED;
    newBomb.vecocity_x = 100.0f + PredictableRandom(randomSeed) * 200.0f;
    bombs.push_back(newBomb);
}

void RestartBombs(std::vector<bomb>& bombs) {
    bombs.clear();
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

    Player player = {SCREEN_WIDTH / 2, SCREEN_HEIGHT - SCREEN_HEIGHT / 2, PLAYER_RADIUS, false, 0.0f, nullptr};
    tiles.push_back(GenerateTile(randomSeed, lastTileX, true));
    int score = 0;
    bool gameOver = false;

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        spawnTimer += deltaTime;

        if (spawnTimer >= SPAWN_INTERVAL) {
            spawnTimer = 0.0f;
            tiles.push_back(GenerateTile(randomSeed, lastTileX));
            if (rand() % 5 == 0) { // 20% chance to spawn a bomb
                SpawnBomb(bombs, randomSeed);
            }
        }

        UpdatePlayerMovement(player);
        ApplyGravity(player, tiles, gameOver, lifelines);
        UpdateTiles(tiles, player, gameOver);
        UpdatePlayerTrail(player);
        UpdateBombs(bombs, deltaTime);
        CheckBombCollision(player, bombs, gameOver, lifelines);
UpdateParticles(deltaTime); // Update particle animations
DrawParticles(); // Draw particles

        if (!gameOver) {
            score += static_cast<int>(deltaTime * 100);
        }

        DrawTexture(background, 0, 0, WHITE);
        DrawGame(player, tiles, score, lifelines);

        if (gameOver) {
            DrawText("GAME OVER! Press 'R' to Restart", SCREEN_WIDTH / 4, SCREEN_HEIGHT / 2, 30, RED);

            if (IsKeyPressed(KEY_R)) {
                RestartGame(player, tiles, randomSeed, lastTileX, score, gameOver, lifelines);
                RestartBombs(bombs);
            }
        }
    }

    CloseWindow();
    return 0;
}
