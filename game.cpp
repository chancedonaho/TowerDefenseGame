#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <algorithm>
#include <string>
#include <iostream>
#include <queue>
#include <map>
#include <filesystem> // Include filesystem for path manipulation (optional)

using namespace std;
namespace fs = std::filesystem; // Namespace alias for filesystem

// Window dimensions
const int screenWidth = 800;
const int screenHeight = 600;

// Expanded Grid dimensions
const int gridColumns = 15; // Expanded columns
const int gridRows = 10;    // Expanded rows
const int tileWidth = screenWidth / gridColumns;
const int tileHeight = screenHeight / gridRows;

// Grid data (true if tile is valid for tower placement)
bool grid[gridRows][gridColumns];

// Structure to represent integer 2D coordinates for grid cells
struct Vector2Int {
    int x;
    int y;
};

// Tower types - More descriptive names and tiers
enum TowerType {
    NONE, // No tower selected
    TIER1_DEFAULT,  // Cost: 20, Default stats
    TIER2_FAST,     // Cost: 30, Higher fire rate
    TIER3_STRONG,   // Cost: 50, Higher damage & fire rate
    TOWER_TYPE_COUNT
};

// Tower struct - Modified to include Texture2D and animation variables
struct Tower {
    Vector2 position;
    Color color;
    float range;
    float fireRate;
    float fireCooldown;
    TowerType type;
    int damage;
    Texture2D texture; // Add texture for the tower sprite
    float rotationAngle; // For idle animation (rotation example)
    float rotationSpeed; // Rotation speed for idle animation
    Texture2D projectileTexture; // Texture for projectiles
};

// Enemy types
enum EnemyType {
    BASIC_ENEMY,
    FAST_ENEMY,
    ENEMY_TYPE_COUNT
};

// Enemy struct
struct Enemy {
    Vector2 position;
    float speed;
    bool active;
    int currentWaypoint;
    int hp;
    int maxHp;
    EnemyType type;
    Color color;
    vector<Vector2Int> waypointsPath; // Path from BFS, grid coordinates
    int pathIndex;                    // Index in the path
    Enemy() : pathIndex(0) {}         // Initialize pathIndex in constructor
    Texture2D texture; // Texture for enemy sprite
};

// Projectile struct
struct Projectile {
    Vector2 position;
    Enemy* targetEnemy;
    float speed;
    int damage;
    bool active;
    Texture2D texture; // Texture for projectile sprite
};

// EnemyWave struct
struct EnemyWave {
    int basicCount;
    int fastCount;
    float spawnInterval;
};

// Game states
enum GameState {
    MENU,
    PLAYING,
    GAME_OVER,
    WIN
};

// Global vectors and variables
vector<Tower> towers;
vector<Enemy> enemies;
vector<Projectile> projectiles;
vector<Vector2> waypoints;
vector<EnemyWave> waves = {
    {5, 0, 1.0f},
    {3, 2, 0.8f},
    {0, 5, 0.5f},
    {7, 0, 0.9f},
    {4, 3, 0.7f},
    {0, 8, 0.4f},
    {10, 0, 0.8f},
    {5, 5, 0.6f},
    {2, 10, 0.3f},
    {15, 5, 0.5f}
};

// Player resources and game state
int playerMoney = 100;
TowerType selectedTowerType = NONE; // Initially no tower selected
int currentWaveIndex = 0;
float waveTimer = 0.0f;
float waveDelay = 15.0f;
bool waveInProgress = false;
int spawnedEnemies = 0;
int defeatedEnemies = 0;
int enemiesReachedEnd = 0;
const int maxEnemiesReachedEnd = 10;
GameState currentState = MENU;

// UI elements
const int uiPadding = 10;
const int towerSelectionWidth = 80; // Wider tower selection buttons
const int towerSelectionHeight = 60; // Taller buttons
const int towerTypeTextSize = 18;
const int largeTextFontSize = 30;
const int regularTextFontSize = 18;
const Color textColor = DARKGRAY; // Define a textColor

// UI positions - Adjusted for cleaner layout
const int titleX = screenWidth / 2; // Center title
const int titleY = uiPadding;
const int moneyX = uiPadding;
const int moneyY = titleY;
const int escapedX = uiPadding;
const int escapedY = moneyY + 25; // Below money
const int waveInfoX = uiPadding;
const int waveInfoY = escapedY + 25; // Below escaped
const int nextWaveTimerY = screenHeight * 0.15f; // Centered vertically
const int towerMenuStartY = uiPadding + 70; // Below other UI elements
const int towerMenuStartX = uiPadding;       //  Defined here
const int towerMenuSpacingX = uiPadding + towerSelectionWidth; // Defined here
const int selectedTowerTextY = towerMenuStartY + towerSelectionHeight + 10;

// Global Textures - Load them once in main and reuse
Texture2D tier1TowerTexture;
Texture2D tier2TowerTexture;
Texture2D tier3TowerTexture;
Texture2D placeholderTexture;
Texture2D tier1ProjectileTexture;
Texture2D tier2ProjectileTexture;
Texture2D tier3ProjectileTexture;
Texture2D tier1EnemyTexture;
Texture2D tier2EnemyTexture;
Texture2D backgroundTexture; // New background texture


// Initialize the grid
void InitGrid() {
    for (int row = 0; row < gridRows; ++row) {
        for (int col = 0; col < gridColumns; ++col) {
            grid[row][col] = true;
        }
    }
}

// Initialize waypoints for enemy pathing (adjust for expanded grid if needed)
void InitWaypoints() {
    waypoints.clear(); // Clear existing waypoints
    // Define waypoints for a path from left edge, vertically centered to right
    int pathRow = gridRows / 2;                 // Vertical center path row

    for (int col = 0; col < gridColumns; ++col) {
        waypoints.push_back({ (float)(col * tileWidth + tileWidth / 2), (float)(pathRow * tileHeight + tileHeight / 2) });
    }
}

// --- Pathfinding related functions ---

// Function to convert pixel position to grid coordinates
Vector2Int GetGridCoords(Vector2 position) {
    return { (int)(position.x / tileWidth), (int)(position.y / tileHeight) };
}

// Function to convert grid coordinates to pixel position (center of tile)
Vector2 GetTileCenter(Vector2Int gridCoords) {
    return { (float)(gridCoords.x * tileWidth + tileWidth / 2), (float)(gridCoords.y * tileHeight + tileHeight / 2) };
}

// Structure to represent a grid cell for pathfinding
struct GridCell {
    Vector2Int coords;
    Vector2Int parentCoords; // For path reconstruction in BFS
    int distance;           // Distance from start node
};

// Breadth-First Search (BFS) Pathfinding Function
vector<Vector2Int> FindPathBFS(Vector2Int start, Vector2Int end) {
    if (!grid[end.y][end.x]) return {}; // End tile is blocked, no path

    queue<GridCell> cellQueue;
    map<pair<int, int>, bool> visited; // Keep track of visited cells

    cellQueue.push({start, {-1, -1}, 0}); // Start cell, no parent, distance 0
    visited[{start.x, start.y}] = true;

    while (!cellQueue.empty()) {
        GridCell currentCell = cellQueue.front();
        cellQueue.pop();

        if (currentCell.coords.x == end.x && currentCell.coords.y == end.y) {
            // Reconstruct path from end to start
            vector<Vector2Int> path;
            Vector2Int currentCoords = currentCell.coords;
            while (currentCoords.x != -1) { // While not the starting cell's dummy parent
                path.push_back(currentCoords);
                GridCell* parentCell = nullptr;
                // Find the parent cell (inefficient, but okay for BFS example)
                queue<GridCell> tempQueue = cellQueue; // Copy queue for searching
                tempQueue.push(currentCell); // Include current cell in search
                map<pair<int, int>, bool> tempVisited = visited; // Copy visited map
                tempVisited[{currentCell.coords.x, currentCell.coords.y}] = true;

                queue<GridCell> searchQueue;
                searchQueue.push({start, {-1, -1}, 0});
                map<pair<int, int>, bool> searchVisited;
                searchVisited[{start.x, start.y}] = true;

                while(!searchQueue.empty()){
                    GridCell tempCurrent = searchQueue.front();
                    searchQueue.pop();
                    if(tempCurrent.coords.x == currentCoords.x && tempCurrent.coords.y == currentCoords.y){
                        parentCell = &tempCurrent;
                        break;
                    }
                    // Explore neighbors (Up, Down, Left, Right)
                    int dx[] = {0, 0, 1, -1};
                    int dy[] = {-1, 1, 0, 0};

                    for (int i = 0; i < 4; ++i) {
                        Vector2Int neighborCoords = {tempCurrent.coords.x + dx[i], tempCurrent.coords.y + dy[i]};

                        if (neighborCoords.x >= 0 && neighborCoords.x < gridColumns && neighborCoords.y >= 0 && neighborCoords.y < gridRows &&
                            grid[neighborCoords.y][neighborCoords.x] && !searchVisited[{neighborCoords.x, neighborCoords.y}]) {
                            searchQueue.push({neighborCoords, tempCurrent.coords, tempCurrent.distance + 1});
                            searchVisited[{neighborCoords.x, neighborCoords.y}] = true;
                        }
                    }
                }


                if(parentCell) currentCoords = parentCell->parentCoords;
                else currentCoords = {-1,-1}; // Should not happen in a correct BFS

            }
            reverse(path.begin(), path.end()); // Path is reconstructed in reverse
            return path;
        }

        // Explore neighbors (Up, Down, Left, Right)
        int dx[] = {0, 0, 1, -1};
        int dy[] = {-1, 1, 0, 0};

        for (int i = 0; i < 4; ++i) {
            Vector2Int neighborCoords = {currentCell.coords.x + dx[i], currentCell.coords.y + dy[i]};

            if (neighborCoords.x >= 0 && neighborCoords.x < gridColumns && neighborCoords.y >= 0 && neighborCoords.y < gridRows &&
                grid[neighborCoords.y][neighborCoords.x] && !visited[{neighborCoords.x, neighborCoords.y}]) {
                cellQueue.push({neighborCoords, currentCell.coords, currentCell.distance + 1}); // Set parent
                visited[{neighborCoords.x, neighborCoords.y}] = true;
            }
        }
    }

    return {}; // No path found
}

// --- DrawPath function for debugging (optional) ---
void DrawPath(const vector<Vector2Int>& path, Color color) {
    if (path.empty()) return;
    for (size_t i = 0; i < path.size(); ++i) {
        DrawRectangle(path[i].x * tileWidth, path[i].y * tileHeight, tileWidth, tileHeight, ColorAlpha(color, 0.3f));
        if (i < path.size() - 1) {
            DrawLineV(GetTileCenter(path[i]), GetTileCenter(path[i + 1]), color);
        }
    }
}


// Create a tower with type-specific properties - Modified to load textures and animation params
Tower CreateTower(TowerType type, Vector2 position) {
    Tower newTower;
    newTower.position = position;
    newTower.type = type;
    newTower.fireCooldown = 0.0f;
    newTower.rotationAngle = 0.0f; // Initialize rotation angle
    newTower.rotationSpeed = 0.0f; // Initialize rotation speed
    newTower.texture = { 0 }; // Initialize texture to null, will be loaded based on type
    newTower.projectileTexture = { 0 }; // Initialize projectile texture

    switch (type) {
        case TIER1_DEFAULT:
            newTower.color = BLUE;
            newTower.range = 150.0f;
            newTower.fireRate = 1.0f;
            newTower.damage = 25;
            newTower.texture = tier1TowerTexture;
            newTower.projectileTexture = tier1ProjectileTexture;
            newTower.rotationSpeed = 10.0f;
            break;
        case TIER2_FAST:
            newTower.color = GREEN;
            newTower.range = 120.0f;
            newTower.fireRate = 1.5f;
            newTower.damage = 20;
            newTower.texture = tier2TowerTexture;
            newTower.projectileTexture = tier2ProjectileTexture;
            newTower.rotationSpeed = 25.0f; // Faster rotation for Tier 2
            break;
        case TIER3_STRONG:
            newTower.color = RED;
            newTower.range = 200.0f;
            newTower.fireRate = 1.2f;
            newTower.damage = 40;
            newTower.texture = tier3TowerTexture;
            newTower.projectileTexture = tier3ProjectileTexture;
            newTower.rotationSpeed = 5.0f; // Slower rotation for Tier 3 (more like pulsing energy)
            break;
        case NONE: // Ghost tower for preview
            newTower.color = ColorAlpha(WHITE, 0.5f);
            newTower.range = 0;
            newTower.fireRate = 0;
            newTower.damage = 0;
            break;
        default:
            TraceLog(LOG_WARNING, "Unknown tower type in CreateTower");
            newTower.color = GRAY;
            newTower.range = 100.0f;
            newTower.fireRate = 1.0f;
            newTower.damage = 10;
            break;
    }
    return newTower;
}

// Get tower cost based on type
int GetTowerCost(TowerType type) {
    switch (type) {
        case TIER1_DEFAULT: return 20;
        case TIER2_FAST: return 30;
        case TIER3_STRONG: return 50;
        default:
            TraceLog(LOG_WARNING, "Unknown tower type in GetTowerCost");
            return 0;
    }
}

// Get tower name for UI display
const char* GetTowerName(TowerType type) {
    switch (type) {
        case TIER1_DEFAULT: return "Tier 1";
        case TIER2_FAST: return "Tier 2";
        case TIER3_STRONG: return "Tier 3";
        default: return "Unknown";
    }
}


// Create an enemy with type-specific properties
Enemy CreateEnemy(EnemyType type, Vector2 startPosition) {
    Enemy newEnemy;
    newEnemy.position = startPosition;
    newEnemy.currentWaypoint = 0;
    newEnemy.active = true;
    newEnemy.type = type;
    newEnemy.texture = { 0 }; // Initialize enemy texture

    switch (type) {
        case BASIC_ENEMY:
            newEnemy.speed = 60.0f;
            newEnemy.hp = 80;
            newEnemy.maxHp = 80;
            newEnemy.color = RED;
            newEnemy.texture = tier1EnemyTexture;
            break;
        case FAST_ENEMY:
            newEnemy.speed = 90.0f;
            newEnemy.hp = 40;
            newEnemy.maxHp = 40;
            newEnemy.color = YELLOW;
            newEnemy.texture = tier2EnemyTexture;
            break;
        default:
            TraceLog(LOG_WARNING, "Unknown enemy type in CreateEnemy");
            newEnemy.speed = 75.0f;
            newEnemy.hp = 50;
            newEnemy.maxHp = 50;
            newEnemy.color = PINK;
            break;
    }
    return newEnemy;
}

// Reset game state
void ResetGame() {
    towers.clear();
    enemies.clear();
    projectiles.clear();
    playerMoney = 100;
    selectedTowerType = NONE; // Reset selected tower type
    currentWaveIndex = 0;
    waveTimer = 0.0f;
    waveDelay = 15.0f;
    waveInProgress = false;
    spawnedEnemies = 0;
    defeatedEnemies = 0;
    enemiesReachedEnd = 0;
    InitGrid(); // Reset grid to all valid tiles at game start/restart
    InitWaypoints(); // Reset waypoints as grid/path might change
    for(auto& enemy : enemies){ // Clear path data when resetting
        enemy.waypointsPath.clear();
        enemy.pathIndex = 0;
    }
}

// Draw the expanded grid
void DrawGrid() {
    for (int row = 0; row < gridRows; ++row) {
        for (int col = 0; col < gridColumns; ++col) {
            Rectangle tileRect = { (float)col * tileWidth, (float)row * tileHeight, (float)tileWidth, (float)tileHeight };
            DrawRectangleLinesEx(tileRect, 1.0f, LIGHTGRAY);
        }
    }
}

// Draw grid highlight under mouse
void DrawGridHighlight() {
    Vector2 mousePos = GetMousePosition();
    int gridCol = mousePos.x / tileWidth;
    int gridRow = mousePos.y / tileHeight;

    if (gridCol >= 0 && gridCol < gridColumns && gridRow >= 0 && gridRow < gridRows) {
        Rectangle highlightRect = { (float)gridCol * tileWidth, (float)gridRow * tileHeight, (float)tileWidth, (float)tileHeight };
        if (grid[gridRow][gridCol]) {
            DrawRectangleLinesEx(highlightRect, 3.0f, GOLD);
        } else {
            DrawRectangleRec(highlightRect, ColorAlpha(RED, 0.5f));
        }
    }
}

// Handle tower placement with type selection and cost
void HandleTowerPlacement() {
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && selectedTowerType != NONE) { // Use MouseButtonReleased for delayed placement
        Vector2 mousePos = GetMousePosition();
        int gridCol = mousePos.x / tileWidth;
        int gridRow = mousePos.y / tileHeight;

        if (gridCol >= 0 && gridCol < gridColumns && gridRow >= 0 && gridRow < gridRows && grid[gridRow][gridCol]) {
            bool towerAlreadyExists = false;
            for (const auto& tower : towers) {
                int towerGridCol = tower.position.x / tileWidth;
                int towerGridRow = tower.position.y / tileHeight;
                if (towerGridCol == gridCol && towerGridRow == gridRow) {
                    towerAlreadyExists = true;
                    break;
                }
            }
            int cost = GetTowerCost(selectedTowerType);
            if (!towerAlreadyExists && playerMoney >= cost) {
                Tower newTower = CreateTower(selectedTowerType, { (float)(gridCol * tileWidth + tileWidth / 2), (float)(gridRow * tileHeight + tileHeight / 2) });
                towers.push_back(newTower);
                playerMoney -= cost;
                grid[gridRow][gridCol] = false; // Block the grid tile after placing tower
                selectedTowerType = NONE; // Deselect tower after placement

                // Recalculate paths for all active enemies
                for (auto& enemy : enemies) {
                    if (enemy.active && enemy.currentWaypoint < waypoints.size()) {
                        Vector2Int startGrid = GetGridCoords(enemy.position);
                        Vector2Int endGrid = GetGridCoords(waypoints.back());
                        vector<Vector2Int> path = FindPathBFS(startGrid, endGrid);
                        if (!path.empty()) {
                            enemy.waypointsPath = path;
                            enemy.pathIndex = 0;
                        } else {
                            TraceLog(LOG_WARNING, "No path found for enemy after tower placement!");
                            enemy.active = false; // Remove enemy if path blocked
                            enemiesReachedEnd++;
                            if (enemiesReachedEnd >= maxEnemiesReachedEnd) currentState = GAME_OVER;
                        }
                    }
                }

            } else if (playerMoney < cost) {
                TraceLog(LOG_WARNING, "Not enough money to place tower type: %d", selectedTowerType);
                selectedTowerType = NONE;
            }
        } else {
             selectedTowerType = NONE;
        }
    }
}


// Draw towers - Modified to use DrawTexturePro for sprites
void DrawTowers() {
    for (const auto& tower : towers) {
        DrawCircleV(tower.position, tower.range, ColorAlpha(tower.color, 0.1f)); // Keep range circle for visual feedback
        if (tower.texture.id > 0) // Check if texture is loaded (valid ID > 0)
        {
            // Define source rectangle (full texture)
            Rectangle sourceRec = { 0.0f, 0.0f, (float)tower.texture.width, (float)tower.texture.height };
            // Define destination rectangle (position and size on screen)
            Rectangle destRec = { tower.position.x, tower.position.y, (float)tileWidth, (float)tileHeight };
            // Define origin (center of the sprite for rotation and centering)
            Vector2 origin = { (float)tileWidth / 2.0f, (float)tileHeight / 2.0f };

            DrawTexturePro(tower.texture, sourceRec, destRec, origin, tower.rotationAngle, WHITE);
        }
        else
        {
            // Fallback to drawing a circle if texture is not loaded (for other tower types for now)
            DrawCircleV(tower.position, tileWidth / 2.5f, tower.color);
        }
    }
}

// Handle tower firing
void HandleTowerFiring() {
    for (auto& tower : towers) {
        if (tower.fireCooldown > 0.0f) {
            tower.fireCooldown -= GetFrameTime();
            continue;
        }

        Enemy* target = nullptr;
        float minDistance = tower.range;
        for (auto& enemy : enemies) {
            if (!enemy.active) continue;
            float distance = Vector2Distance(tower.position, enemy.position);
            if (distance < minDistance) {
                minDistance = distance;
                target = &enemy;
            }
        }

        if (target != nullptr) {
            Projectile newProjectile;
            newProjectile.position = tower.position;
            newProjectile.targetEnemy = target;
            newProjectile.speed = 200.0f;
            newProjectile.damage = tower.damage;
            newProjectile.active = true;
            newProjectile.texture = tower.projectileTexture; // Assign projectile texture from tower
            projectiles.push_back(newProjectile);
            tower.fireCooldown = 1.0f / tower.fireRate;
        }
    }
}

// Update projectiles with damage
void UpdateProjectiles() {
    for (auto& projectile : projectiles) {
        if (!projectile.active) continue;
        if (!projectile.targetEnemy->active) {
            projectile.active = false;
            continue;
        }

        Vector2 direction = Vector2Subtract(projectile.targetEnemy->position, projectile.position);
        float distance = Vector2Length(direction);

        if (distance < 5.0f) {
            projectile.targetEnemy->hp -= projectile.damage;
            if (projectile.targetEnemy->hp <= 0) {
                projectile.targetEnemy->active = false;
                playerMoney += 10;
                defeatedEnemies++;
            }
            projectile.active = false;
        } else {
            Vector2 normalizedDir = Vector2Normalize(direction);
            Vector2 velocity = Vector2Scale(normalizedDir, projectile.speed * GetFrameTime());
            projectile.position = Vector2Add(projectile.position, velocity);
        }
    }
}

// Update enemies with waypoint pathing
void UpdateEnemies() {
    for (auto& enemy : enemies) {
        if (!enemy.active) continue;

        if (enemy.currentWaypoint < waypoints.size()) {
            Vector2 targetWaypoint = waypoints[enemy.currentWaypoint];
            Vector2 direction = Vector2Subtract(targetWaypoint, enemy.position);
            float distance = Vector2Length(direction);

            if (distance < 5.0f) {
                enemy.currentWaypoint++;
                if (enemy.currentWaypoint < waypoints.size()) {
                    // Recalculate path to next waypoint using BFS
                    Vector2Int startGrid = GetGridCoords(enemy.position);
                    Vector2Int endGrid = GetGridCoords(waypoints.back());
                    vector<Vector2Int> path = FindPathBFS(startGrid, endGrid);
                    if (!path.empty()) {
                        enemy.waypointsPath = path;
                        enemy.pathIndex = 0;
                    } else {
                        TraceLog(LOG_WARNING, "No path found for enemy!");
                        enemy.active = false; // If no path, remove enemy (for simplicity)
                        enemiesReachedEnd++;
                        if (enemiesReachedEnd >= maxEnemiesReachedEnd) currentState = GAME_OVER;
                        continue; // Skip movement if no path
                    }
                }
            }

            if (!enemy.waypointsPath.empty() && enemy.pathIndex < enemy.waypointsPath.size()) {
                Vector2 targetPos = GetTileCenter(enemy.waypointsPath[enemy.pathIndex]);
                Vector2 moveDirection = Vector2Normalize(Vector2Subtract(targetPos, enemy.position));
                enemy.position = Vector2Add(enemy.position, Vector2Scale(moveDirection, enemy.speed * GetFrameTime()));
                if (Vector2Distance(enemy.position, targetPos) < 5.0f) {
                    enemy.pathIndex++;
                }
            }
            else { // Fallback to original waypoint if no path or path ended
                Vector2 normalizedDir = Vector2Normalize(direction);
                Vector2 velocity = Vector2Scale(normalizedDir, enemy.speed * GetFrameTime());
                enemy.position = Vector2Add(enemy.position, velocity);
            }

        } else {
            enemy.active = false;
            enemiesReachedEnd++;
            if (enemiesReachedEnd >= maxEnemiesReachedEnd) {
                currentState = GAME_OVER;
            }
        }
    }
}

// Draw projectiles - Modified to use sprites
void DrawProjectiles() {
    for (const auto& projectile : projectiles) {
        if (projectile.active) {
            if (projectile.texture.id > 0) {
                Rectangle sourceRec = { 0.0f, 0.0f, (float)projectile.texture.width, (float)projectile.texture.height };
                Rectangle destRec = { projectile.position.x, projectile.position.y, 16, 16 }; // Adjust size as needed
                Vector2 origin = { 8, 8 }; // Center origin for rotation/positioning
                DrawTexturePro(projectile.texture, sourceRec, destRec, origin, 0.0f, WHITE);
            } else {
                DrawCircleV(projectile.position, 5.0f, YELLOW); // Fallback if no texture
            }
        }
    }
}

// Handle Tower Selection Menu clicks
void HandleTowerMenuClick() {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mousePos = GetMousePosition();

        // Tier 1 Tower Button
        Rectangle tier1Rec = { (float)towerMenuStartX, (float)towerMenuStartY, (float)towerSelectionWidth, (float)towerSelectionHeight };
        if (CheckCollisionPointRec(mousePos, tier1Rec)) {
            selectedTowerType = TIER1_DEFAULT;
            return; // Exit to prevent selecting multiple towers in one click
        }
        // Tier 2 Tower Button
        Rectangle tier2Rec = { (float)towerMenuStartX + towerMenuSpacingX, (float)towerMenuStartY, (float)towerSelectionWidth, (float)towerSelectionHeight };
        if (CheckCollisionPointRec(mousePos, tier2Rec)) {
            selectedTowerType = TIER2_FAST;
            return;
        }
        // Tier 3 Tower Button
        Rectangle tier3Rec = { (float)towerMenuStartX + towerMenuSpacingX * 2, (float)towerMenuStartY, (float)towerSelectionWidth, (float)towerSelectionHeight };
        if (CheckCollisionPointRec(mousePos, tier3Rec)) {
            selectedTowerType = TIER3_STRONG;
            return;
        }
        // Deselect if clicking on already selected tower type (toggle deselect)
        if (selectedTowerType != NONE) {
             Rectangle selectedTowerRec;
             if (selectedTowerType == TIER1_DEFAULT) selectedTowerRec = tier1Rec;
             else if (selectedTowerType == TIER2_FAST) selectedTowerRec = tier2Rec;
             else if (selectedTowerType == TIER3_STRONG) selectedTowerRec = tier3Rec;

             if (CheckCollisionPointRec(mousePos, selectedTowerRec)) {
                 selectedTowerType = NONE; // Deselect if clicked selected tower again
                 return;
             }
        }
    }
}


int main() {
    InitWindow(screenWidth, screenHeight, "Robust Tower Defense - v0.2");
    SetTargetFPS(60);

    // Load textures - Load them globally after InitWindow
    tier1TowerTexture = LoadTexture("tier1tower.png");
    if (tier1TowerTexture.id == 0) TraceLog(LOG_ERROR, "Failed to load texture: tier1tower.png (Global Load)");
    tier2TowerTexture = LoadTexture("tier2tower.png");
    if (tier2TowerTexture.id == 0) TraceLog(LOG_ERROR, "Failed to load texture: tier2tower.png (Global Load)");
    tier3TowerTexture = LoadTexture("tier3tower.png");
    if (tier3TowerTexture.id == 0) TraceLog(LOG_ERROR, "Failed to load texture: tier3tower.png (Global Load)");
    placeholderTexture = LoadTexture("placeholder.png");
    if (placeholderTexture.id == 0) TraceLog(LOG_ERROR, "Failed to load texture: placeholder.png (Global Load)");
    tier1ProjectileTexture = LoadTexture("tier1projectile.png");
    if (tier1ProjectileTexture.id == 0) TraceLog(LOG_ERROR, "Failed to load texture: tier1projectile.png (Global Load)");
    tier2ProjectileTexture = LoadTexture("tier2projectile.png");
    if (tier2ProjectileTexture.id == 0) TraceLog(LOG_ERROR, "Failed to load texture: tier2projectile.png (Global Load)");
    tier3ProjectileTexture = LoadTexture("tier3projectile.png");
    if (tier3ProjectileTexture.id == 0) TraceLog(LOG_ERROR, "Failed to load texture: tier3projectile.png (Global Load)");
    tier1EnemyTexture = LoadTexture("tier1enemy.png");
    if (tier1EnemyTexture.id == 0) TraceLog(LOG_ERROR, "Failed to load texture: tier1enemy.png (Global Load)");
    tier2EnemyTexture = LoadTexture("tier2enemy.png");
    if (tier2EnemyTexture.id == 0) TraceLog(LOG_ERROR, "Failed to load texture: tier2enemy.png (Global Load)");
    backgroundTexture = LoadTexture("towerdefensegrass.png"); // Load background texture
    if (backgroundTexture.id == 0) TraceLog(LOG_ERROR, "Failed to load texture: towerdefensegrass.png (Background)");


    InitGrid();
    InitWaypoints();

    while (!WindowShouldClose()) {
        if (currentState == PLAYING) {

            HandleTowerMenuClick(); // Handle tower selection menu clicks
            HandleTowerPlacement();
            UpdateEnemies();
            HandleTowerFiring();
            UpdateProjectiles();

            enemies.erase(remove_if(enemies.begin(), enemies.end(), [](const Enemy& e) { return !e.active; }), enemies.end());
            projectiles.erase(remove_if(projectiles.begin(), projectiles.end(), [](const Projectile& p) { return !p.active; }), projectiles.end());

            // Wave management
            if (!waveInProgress && currentWaveIndex < waves.size()) {
                waveDelay -= GetFrameTime();
                if (waveDelay <= 0.0f) {
                    waveInProgress = true;
                    waveTimer = 0.0f;
                    spawnedEnemies = 0;
                    defeatedEnemies = 0;
                }
            }

            if (waveInProgress) {
                waveTimer -= GetFrameTime();
                int totalEnemies = waves[currentWaveIndex].basicCount + waves[currentWaveIndex].fastCount;
                if (waveTimer <= 0.0f && spawnedEnemies < totalEnemies) {
                    EnemyType type = (spawnedEnemies < waves[currentWaveIndex].basicCount) ? BASIC_ENEMY : FAST_ENEMY;
                    // Spawn enemies from left edge, vertically centered
                    float spawnX = 0; // Left edge X
                    float spawnY = waypoints[0].y; // Vertically centered Y
                    enemies.push_back(CreateEnemy(type, {spawnX, spawnY}));
                    waveTimer = waves[currentWaveIndex].spawnInterval;
                    spawnedEnemies++;
                }
                if (spawnedEnemies >= totalEnemies && enemies.empty()) {
                    waveInProgress = false;
                    currentWaveIndex++;
                    if (currentWaveIndex >= waves.size()) {
                        currentState = WIN;
                    } else {
                        waveDelay = 15.0f;
                    }
                }
            }

            // Update tower animations (idle animations) - Place this in your game update loop, before drawing
            for (auto& tower : towers) {
                tower.rotationAngle += tower.rotationSpeed * GetFrameTime();
                if (tower.rotationAngle > 360.0f) tower.rotationAngle -= 360.0f; // Keep angle within 0-360
            }


        }

        BeginDrawing();
        ClearBackground(RAYWHITE); // We can keep this for UI background, or remove if background texture covers everything

        if (currentState == MENU) {
            if (backgroundTexture.id > 0)
            {
                Rectangle sourceRec = { 0.0f, 0.0f, (float)backgroundTexture.width, (float)backgroundTexture.height };
                Rectangle destRec = { 0.0f, 0.0f, (float)backgroundTexture.width, (float)backgroundTexture.height }; // Size of one tile
                Vector2 origin = { 0.0f, 0.0f };

                for (int y = 0; y < screenHeight; y += backgroundTexture.height)
                {
                    for (int x = 0; x < screenWidth; x += backgroundTexture.width)
                    {
                        destRec.x = (float)x;
                        destRec.y = (float)y;
                        DrawTexturePro(backgroundTexture, sourceRec, destRec, origin, 0.0f, WHITE);
                    }
                }
            }
            DrawText("Tower Defense Game", screenWidth / 2 - 150, screenHeight / 2 - 100, 40, textColor);
            Rectangle startButton = {screenWidth / 2 - 50, screenHeight / 2, 100, 40};
            DrawRectangleRec(startButton, LIGHTGRAY);
            DrawText("Start Game", screenWidth / 2 - 45, screenHeight / 2 + 10, 20, textColor);
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), startButton)) {
                ResetGame();
                currentState = PLAYING;
            }
        } else if (currentState == PLAYING) {
            if (backgroundTexture.id > 0)
            {
                Rectangle sourceRec = { 0.0f, 0.0f, (float)backgroundTexture.width, (float)backgroundTexture.height };
                Rectangle destRec = { 0.0f, 0.0f, (float)backgroundTexture.width, (float)backgroundTexture.height }; // Size of one tile
                Vector2 origin = { 0.0f, 0.0f };

                for (int y = 0; y < screenHeight; y += backgroundTexture.height)
                {
                    for (int x = 0; x < screenWidth; x += backgroundTexture.width)
                    {
                        destRec.x = (float)x;
                        destRec.y = (float)y;
                        DrawTexturePro(backgroundTexture, sourceRec, destRec, origin, 0.0f, WHITE);
                    }
                }
            }

            // Draw path indicator - Draw path lines on top of background if needed
            for (size_t i = 0; i < waypoints.size() - 1; ++i) {
                DrawLineV(waypoints[i], waypoints[i+1], LIGHTGRAY); // Path line
            }

            DrawGrid(); // Draw grid on top of background and path if needed
            DrawGridHighlight();

            // Draw ghost tower preview if a tower type is selected
            if (selectedTowerType != NONE) {
                Vector2 mousePos = GetMousePosition();
                int gridCol = mousePos.x / tileWidth;
                int gridRow = mousePos.y / tileHeight;
                if (gridCol >= 0 && gridCol < gridColumns && gridRow >= 0 && gridRow < gridRows && grid[gridRow][gridCol]) {
                    Tower ghostTower = CreateTower(NONE, { (float)(gridCol * tileWidth + tileWidth / 2), (float)(gridRow * tileHeight + tileHeight / 2) });
                    DrawCircleV(ghostTower.position, tileWidth / 2.5f, ghostTower.color); // Draw ghost tower
                }
            }

            DrawTowers(); // Draw towers - now using sprites (on top of background)
            for (const auto& enemy : enemies) {
                if (enemy.active) {
                    if (enemy.texture.id > 0) {
                        Rectangle sourceRec = { 0.0f, 0.0f, (float)enemy.texture.width, (float)enemy.texture.height };
                        Rectangle destRec = { enemy.position.x, enemy.position.y, (float)tileWidth, (float)tileHeight }; // Adjust size as needed
                        Vector2 origin = { (float)tileWidth / 2.0f, (float)tileHeight / 2.0f };
                        DrawTexturePro(enemy.texture, sourceRec, destRec, origin, 0.0f, WHITE);
                    } else {
                        DrawCircleV(enemy.position, 10, enemy.color); // Fallback if no texture
                    }

                    float barWidth = 20.0f;
                    float barHeight = 5.0f;
                    float currentBarWidth = (enemy.hp / (float)enemy.maxHp) * barWidth;
                    Vector2 barPosition = {enemy.position.x - barWidth / 2, enemy.position.y - 15};
                    DrawRectangleV(barPosition, {barWidth, barHeight}, DARKGRAY);
                    Color hpColor = (enemy.hp > enemy.maxHp * 0.5f) ? GREEN : (enemy.hp > enemy.maxHp * 0.25f) ? YELLOW : RED;
                    DrawRectangleV(barPosition, {currentBarWidth, barHeight}, hpColor);
                }
            }
            DrawProjectiles(); // Draw projectiles on top of enemies and background

            // UI - Cleaned up and reorganized (UI on top of everything)
            DrawText("Tower Defense Game - Tower Placement", titleX - MeasureText("Tower Defense Game - Tower Placement", regularTextFontSize) / 2, titleY, regularTextFontSize, textColor); // Centered title
            DrawText(TextFormat("Money: %d", playerMoney), moneyX, moneyY, regularTextFontSize, textColor);
            DrawText(TextFormat("Escaped: %d/%d", enemiesReachedEnd, maxEnemiesReachedEnd), escapedX, escapedY, regularTextFontSize, RED); // Escaped below Money


            // Wave information - Below escaped count
            if (waveInProgress) {
                int totalEnemies = waves[currentWaveIndex].basicCount + waves[currentWaveIndex].fastCount;
                int enemiesRemaining = totalEnemies - spawnedEnemies + (int)enemies.size();
                DrawText(TextFormat("Wave %d - Enemies Remaining: %d", currentWaveIndex + 1, enemiesRemaining), waveInfoX, waveInfoY, regularTextFontSize, textColor);
            } else if (currentWaveIndex < waves.size()) {
                string nextWaveText = "Next Wave in " + to_string((int)waveDelay + 1);
                DrawText(nextWaveText.c_str(), screenWidth / 2 - MeasureText(nextWaveText.c_str(), largeTextFontSize) / 2, nextWaveTimerY, largeTextFontSize, BLUE); // Centered timer
            } else {
                DrawText("All Waves Completed!", waveInfoX, waveInfoY, regularTextFontSize, GREEN);
            }


            // Tower Selection Menu UI - Positioned below other UI elements
            int currentMenuX = towerMenuStartX;
            int currentMenuY = towerMenuStartY;

            // Tier 1 Tower Button
            Rectangle tier1Rec = { (float)currentMenuX, (float)currentMenuY, (float)towerSelectionWidth, (float)towerSelectionHeight };
            DrawRectangleRec(tier1Rec, BLUE);
            DrawRectangleLinesEx(tier1Rec, 2.0f, selectedTowerType == TIER1_DEFAULT ? GOLD : DARKGRAY); // Highlight if selected
            DrawText(GetTowerName(TIER1_DEFAULT), currentMenuX + 10, currentMenuY + 10, towerTypeTextSize, WHITE);
            DrawText(TextFormat("$%d", GetTowerCost(TIER1_DEFAULT)), currentMenuX + 10, currentMenuY + towerSelectionHeight - 25, regularTextFontSize, WHITE);
            currentMenuX += towerMenuSpacingX;


            // Tier 2 Tower Button
            Rectangle tier2Rec = { (float)currentMenuX, (float)currentMenuY, (float)towerSelectionWidth, (float)towerSelectionHeight };
            DrawRectangleRec(tier2Rec, GREEN);
            DrawRectangleLinesEx(tier2Rec, 2.0f, selectedTowerType == TIER2_FAST ? GOLD : DARKGRAY); // Highlight if selected
            DrawText(GetTowerName(TIER2_FAST), currentMenuX + 10, currentMenuY + 10, towerTypeTextSize, WHITE);
            DrawText(TextFormat("$%d", GetTowerCost(TIER2_FAST)), currentMenuX + 10, currentMenuY + towerSelectionHeight - 25, regularTextFontSize, WHITE);
            currentMenuX += towerMenuSpacingX;


            // Tier 3 Tower Button
            Rectangle tier3Rec = { (float)currentMenuX, (float)currentMenuY, (float)towerSelectionWidth, (float)towerSelectionHeight };
            DrawRectangleRec(tier3Rec, RED);
            DrawRectangleLinesEx(tier3Rec, 2.0f, selectedTowerType == TIER3_STRONG ? GOLD : DARKGRAY); // Highlight if selected
            DrawText(GetTowerName(TIER3_STRONG), currentMenuX + 10, currentMenuY + 10, towerTypeTextSize, WHITE);
            DrawText(TextFormat("$%d", GetTowerCost(TIER3_STRONG)), currentMenuX + 10, currentMenuY + towerSelectionHeight - 25, regularTextFontSize, WHITE);


            // Visual indication of selected tower type - Below the menu
            if (selectedTowerType != NONE) {
                DrawText(TextFormat("Selected: %s", GetTowerName(selectedTowerType)), uiPadding, selectedTowerTextY, regularTextFontSize, GOLD);
            }

            // Debug path drawing - optional, remove in final game
             for(const auto& enemy : enemies){
                 if(enemy.active){
                     DrawPath(enemy.waypointsPath, LIME); // Draw enemy paths in lime
                 }
             }


        } else if (currentState == GAME_OVER) {
            if (backgroundTexture.id > 0)
            {
                Rectangle sourceRec = { 0.0f, 0.0f, (float)backgroundTexture.width, (float)backgroundTexture.height };
                Rectangle destRec = { 0.0f, 0.0f, (float)backgroundTexture.width, (float)backgroundTexture.height }; // Size of one tile
                Vector2 origin = { 0.0f, 0.0f };

                for (int y = 0; y < screenHeight; y += backgroundTexture.height)
                {
                    for (int x = 0; x < screenWidth; x += backgroundTexture.width)
                    {
                        destRec.x = (float)x;
                        destRec.y = (float)y;
                        DrawTexturePro(backgroundTexture, sourceRec, destRec, origin, 0.0f, WHITE);
                    }
                }
            }
            DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(DARKGRAY, 0.5f));
            DrawText("Game Over", screenWidth / 2 - 100, screenHeight / 2 - 50, 40, RED);
            DrawText(TextFormat("Enemies Escaped: %d/%d", enemiesReachedEnd, maxEnemiesReachedEnd), screenWidth / 2 - 150, screenHeight / 2 - 10, 20, RED);
            Rectangle restartButton = {screenWidth / 2 - 50, screenHeight / 2 + 30, 100, 40};
            DrawRectangleRec(restartButton, LIGHTGRAY);
            DrawText("Restart", screenWidth / 2 - 30, screenHeight / 2 + 40, 20, textColor);
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), restartButton)) {
                ResetGame();
                currentState = MENU;
            }
        } else if (currentState == WIN) {
            if (backgroundTexture.id > 0)
            {
                Rectangle sourceRec = { 0.0f, 0.0f, (float)backgroundTexture.width, (float)backgroundTexture.height };
                Rectangle destRec = { 0.0f, 0.0f, (float)backgroundTexture.width, (float)backgroundTexture.height }; // Size of one tile
                Vector2 origin = { 0.0f, 0.0f };

                for (int y = 0; y < screenHeight; y += backgroundTexture.height)
                {
                    for (int x = 0; x < screenWidth; x += backgroundTexture.width)
                    {
                        destRec.x = (float)x;
                        destRec.y = (float)y;
                        DrawTexturePro(backgroundTexture, sourceRec, destRec, origin, 0.0f, WHITE);
                    }
                } 
            }
            DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(DARKGRAY, 0.5f));
            DrawText("You Win!", screenWidth / 2 - 100, screenHeight / 2 - 50, 40, GREEN);
            Rectangle restartButton = {screenWidth / 2 - 50, screenHeight / 2 + 10, 100, 40};
            DrawRectangleRec(restartButton, LIGHTGRAY);
            DrawText("Restart", screenWidth / 2 - 30, screenHeight / 2 + 20, 20, textColor);
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), restartButton)) {
                ResetGame();
                currentState = MENU;
            }
        }

        EndDrawing();
    }

    // Unload textures before closing
    UnloadTexture(tier1TowerTexture);
    UnloadTexture(tier2TowerTexture);
    UnloadTexture(tier3TowerTexture);
    UnloadTexture(placeholderTexture);
    UnloadTexture(tier1ProjectileTexture);
    UnloadTexture(tier2ProjectileTexture);
    UnloadTexture(tier3ProjectileTexture);
    UnloadTexture(tier1EnemyTexture);
    UnloadTexture(tier2EnemyTexture);
    UnloadTexture(backgroundTexture); // Unload background texture

    CloseWindow();
    return 0;
} 