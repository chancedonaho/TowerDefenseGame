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

// Forward declarations for types used in prototypes:
struct Tower;  // NEW: Forward declaration added for Tower
void DrawVisualEffects();
void DrawTowerTooltip(TowerType type, Vector2 position);
void HandleTowerAbilityButton();
// NEW: Forward declaration for tower repair function
void RepairTower(Tower &tower);

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
    int upgradeLevel; // Add upgrade level tracking (0, 1, 2)
    
    // Tower ability fields
    float abilityCooldownTimer;     // Current cooldown time remaining
    float abilityCooldownDuration;  // Total cooldown duration
    bool abilityActive;             // Whether the ability is currently active
    float abilityDuration;          // How long the ability lasts
    float abilityTimer;             // Current ability duration time remaining
    float originalFireRate;         // Store original fire rate for Speed Boost ability
    bool isPowerShotActive;         // Flag for Power Shot ability

    // NEW: Malfunction fields
    float lastFiredTime;    // Timestamp of the last time the tower fired
    bool isMalfunctioning;  // Flag indicating if the tower is frozen/malfunctioning
};

// Enemy types
enum EnemyType {
    BASIC_ENEMY,
    FAST_ENEMY,
    ARMOURED_ENEMY,     // New armored enemy type
    FAST_ARMOURED_ENEMY, // New fast armored enemy type
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
    Texture2D texture;                // Texture for enemy sprite
    
    // Enemy slow effect fields
    float originalSpeed;              // Store original speed when slowed
    float slowTimer;                  // Duration of slow effect remaining
    bool isSlowed;                    // Whether enemy is currently slowed
    
    // Damage over time effect
    bool hasDotEffect;                // Whether enemy has a DOT effect
    float dotTimer;                   // Duration of DOT effect remaining
    float dotTickTimer;               // Timer for next DOT damage tick
    int dotDamage;                    // Damage per tick
};

// Projectile struct
struct Projectile {
    Vector2 position;
    Enemy* targetEnemy;
    float speed;
    int damage;
    bool active;
    Texture2D texture; // Texture for projectile sprite
    
    // New fields for special projectile types
    enum class Type {
        STANDARD,
        FLAMETHROWER
    } type;
    
    Vector2 sourcePosition;  // Starting position (for drawing effects)
    float effectRadius;      // Radius for AOE effects
};

// EnemyWave struct
struct EnemyWave {
    int basicCount;
    int fastCount;
    int armouredCount;      // Number of armored enemies in wave
    int fastArmouredCount;  // Number of fast armored enemies in wave
    float spawnInterval;
};

// Game states
enum GameState {
    MENU,
    PLAYING,
    PAUSED,    // New paused state
    GAME_OVER,
    WIN
};

// Structure for visual effects
struct VisualEffect {
    Vector2 position;
    float lifespan;    // Total lifetime in seconds
    float timer;       // Remaining time 
    Color color;
    float radius;      // Effect size
    bool active;
};

// New struct for laser beam visual effect
struct LaserBeam {
    Vector2 start;
    Vector2 end;
    float timer;
    float duration;
    bool active;
    Color color;
    float thickness;
};

// NEW: Map Difficulty and Weather enums
enum MapDifficulty { EASY, MEDIUM, HARD };
enum WeatherType { WEATHER_NONE, RAIN, SNOW };

// NEW: Global variables for current map difficulty and weather setting
MapDifficulty currentDifficulty = EASY;
WeatherType currentWeather = WEATHER_NONE;

// NEW: Structure for weather particles (for rain and snow effects)
struct WeatherParticle {
    Vector2 position;
    Vector2 velocity;
    float lifetime;
    float maxLifetime;    // Store the initial lifetime for fade calculations
    float size;           // Current size of the particle
    float alpha;          // Transparency value
    float wobble;         // Phase for side-to-side motion
    float wobbleSpeed;    // How fast the snow wobbles
    float targetHeight;   // NEW: Target height for raindrops to hit and splash
    bool isSplash;        // NEW: Flag to identify if this is a splash effect
};
vector<WeatherParticle> weatherParticles;

// Global vectors and variables
vector<Tower> towers;
vector<Enemy> enemies;
vector<Projectile> projectiles;
vector<Vector2> waypoints;
vector<EnemyWave> waves = {
    {5, 0, 0, 0, 1.0f},    // Wave 1: 5 basic
    {3, 2, 0, 0, 0.8f},    // Wave 2: 3 basic, 2 fast
    {0, 5, 0, 0, 0.5f},    // Wave 3: 5 fast
    {5, 0, 2, 0, 0.9f},    // Wave 4: 5 basic, 2 armored
    {2, 2, 2, 0, 0.7f},    // Wave 5: 2 each (except fast armored)
    {0, 5, 0, 2, 0.4f},    // Wave 6: 5 fast, 2 fast armored
    {5, 0, 5, 0, 0.8f},    // Wave 7: 5 basic, 5 armored
    {0, 5, 0, 3, 0.6f},    // Wave 8: 5 fast, 3 fast armored
    {0, 0, 5, 5, 0.3f},    // Wave 9: 5 armored, 5 fast armored
    {10, 5, 5, 5, 0.5f}    // Wave 10: 10 basic, 5 of each other type
};

// New vector for visual effects
vector<VisualEffect> visualEffects;

// Add vector to store laser beam effects
vector<LaserBeam> laserBeams;

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
int selectedTowerIndex = -1; // Add variable to track selected tower (-1 means none)

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

// Tower selection UI positions
const int selectedTowerInfoX = screenWidth - 200;
const int selectedTowerInfoY = 100;
const int upgradeButtonWidth = 120;
const int upgradeButtonHeight = 40;
const int infoSpacing = 25;
const int abilityButtonWidth = 150;
const int abilityButtonHeight = 40;

// Wave progress bar position
const int progressBarWidth = 200;
const int progressBarHeight = 15;
const int progressBarX = screenWidth - progressBarWidth - 20;
const int progressBarY = 60;

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
Texture2D tier3EnemyTexture; // For armored enemy
Texture2D tier4EnemyTexture; // For fast armored enemy

// Add border grid textures
Texture2D bottomGridTexture;
Texture2D leftGridTexture;
Texture2D topGridTexture;
Texture2D secondRightmostTexture;
Texture2D rightGridTexture;

// Adding pause button variables
Rectangle pauseButton = { 10, 10, 30, 30 };
bool isPaused = false;

// Add wave skip button variables
Rectangle skipWaveButton;
bool showSkipButton = false;

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
        DrawRectangle(path[i].x * tileWidth, path[i].y * tileHeight, tileWidth, tileHeight, ColorAlpha(color, 0.1f)); // Reduced opacity from 0.3f to 0.1f
        if (i < path.size() - 1) {
            DrawLineV(GetTileCenter(path[i]), GetTileCenter(path[i + 1]), ColorAlpha(color, 0.5f)); // Added transparency to lines
        }
    }
}

// Get tower upgrade cost based on tower type and current level
int GetTowerUpgradeCost(TowerType type, int currentLevel) {
    if (currentLevel >= 2) return 0; // Max level reached
    
    switch (type) {
        case TIER1_DEFAULT:
            return currentLevel == 0 ? 30 : 60; // Level 0->1: 30, Level 1->2: 60
        case TIER2_FAST:
            return currentLevel == 0 ? 40 : 80; // Level 0->1: 40, Level 1->2: 80
        case TIER3_STRONG:
            return currentLevel == 0 ? 60 : 120; // Level 0->1: 60, Level 1->2: 120
        default:
            return 0;
    }
}

// Apply tower upgrade stats based on type and level
void ApplyTowerUpgrade(Tower& tower) {
    // Base stats multiplier for each upgrade level
    float damageMultiplier = 1.0f;
    float rangeMultiplier = 1.0f;
    float fireRateMultiplier = 1.0f;
    
    // Apply multipliers based on upgrade level
    if (tower.upgradeLevel == 1) {
        damageMultiplier = 1.5f;  // +50% damage
        rangeMultiplier = 1.2f;   // +20% range
        fireRateMultiplier = 1.2f; // +20% fire rate
    } else if (tower.upgradeLevel == 2) {
        damageMultiplier = 2.5f;  // +150% damage
        rangeMultiplier = 1.5f;   // +50% range
        fireRateMultiplier = 1.5f; // +50% fire rate
    }
    
    // Apply type-specific multipliers
    switch (tower.type) {
        case TIER1_DEFAULT: // Balanced upgrades
            tower.damage = (int)(25 * damageMultiplier);
            tower.range = 150.0f * rangeMultiplier;
            tower.fireRate = 1.0f * fireRateMultiplier;
            break;
            
        case TIER2_FAST: // More focus on fire rate
            tower.damage = (int)(20 * damageMultiplier);
            tower.range = 120.0f * rangeMultiplier;
            tower.fireRate = 1.5f * (fireRateMultiplier + 0.1f * tower.upgradeLevel); // Extra fire rate boost
            break;
            
        case TIER3_STRONG: // More focus on damage and range
            tower.damage = (int)(40 * (damageMultiplier + 0.2f * tower.upgradeLevel)); // Extra damage boost
            tower.range = 200.0f * (rangeMultiplier + 0.1f * tower.upgradeLevel); // Extra range boost
            tower.fireRate = 1.2f * fireRateMultiplier;
            break;
            
        default:
            break;
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
    newTower.upgradeLevel = 0; // Initialize upgrade level to 0 (base level)
    
    // Initialize ability fields
    newTower.abilityCooldownTimer = 0.0f;
    newTower.abilityActive = false;
    newTower.abilityTimer = 0.0f;
    newTower.isPowerShotActive = false;

    // Initialize malfunction fields
    newTower.lastFiredTime = 0.0f;
    newTower.isMalfunctioning = false;

    switch (type) {
        case TIER1_DEFAULT:
            newTower.color = BLUE;
            newTower.range = 150.0f;
            newTower.fireRate = 1.0f;
            newTower.damage = 25;
            newTower.texture = tier1TowerTexture;
            newTower.projectileTexture = tier1ProjectileTexture;
            newTower.rotationSpeed = 10.0f;
            newTower.abilityCooldownDuration = 15.0f; // Area Slow: 15s cooldown
            newTower.abilityDuration = 3.0f;          // Area Slow: 3s duration
            newTower.originalFireRate = newTower.fireRate;
            break;
        case TIER2_FAST:
            newTower.color = GREEN;
            newTower.range = 120.0f;
            newTower.fireRate = 1.5f;
            newTower.damage = 20;
            newTower.texture = tier2TowerTexture;
            newTower.projectileTexture = tier2ProjectileTexture;
            newTower.rotationSpeed = 25.0f; // Faster rotation for Tier 2
            newTower.abilityCooldownDuration = 10.0f; // Speed Boost: 10s cooldown
            newTower.abilityDuration = 5.0f;          // Speed Boost: 5s duration
            newTower.originalFireRate = newTower.fireRate;
            break;
        case TIER3_STRONG:
            newTower.color = RED;
            newTower.range = 200.0f;
            newTower.fireRate = 1.2f;
            newTower.damage = 40;
            newTower.texture = tier3TowerTexture;
            newTower.projectileTexture = tier3ProjectileTexture;
            newTower.rotationSpeed = 5.0f; // Slower rotation for Tier 3 (more like pulsing energy)
            newTower.abilityCooldownDuration = 8.0f; // Power Shot: 8s cooldown
            newTower.abilityDuration = 0.0f;         // Power Shot: instant, no duration
            newTower.originalFireRate = newTower.fireRate;
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

// Forward declaration of HandleTowerAbilityButton
void HandleTowerAbilityButton();

// Create an enemy with type-specific properties
Enemy CreateEnemy(EnemyType type, Vector2 startPosition) {
    Enemy newEnemy;
    newEnemy.position = startPosition;
    newEnemy.currentWaypoint = 0;
    newEnemy.active = true;
    newEnemy.type = type;
    newEnemy.texture = { 0 }; // Initialize enemy texture
    newEnemy.slowTimer = 0.0f;
    newEnemy.isSlowed = false;
    
    // Initialize DOT effect fields
    newEnemy.hasDotEffect = false;
    newEnemy.dotTimer = 0.0f;
    newEnemy.dotTickTimer = 0.0f;
    newEnemy.dotDamage = 0;

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
        case ARMOURED_ENEMY:
            newEnemy.speed = 60.0f;     // Same speed as basic enemy
            newEnemy.hp = 150;          // Higher HP
            newEnemy.maxHp = 150;
            newEnemy.color = DARKGRAY;  // Armored appearance
            newEnemy.texture = tier3EnemyTexture;
            break;
        case FAST_ARMOURED_ENEMY:
            newEnemy.speed = 90.0f;     // Same speed as fast enemy
            newEnemy.hp = 100;          // Medium HP
            newEnemy.maxHp = 100;
            newEnemy.color = GOLD;      // Fast armored appearance
            newEnemy.texture = tier4EnemyTexture;
            break;
        default:
            TraceLog(LOG_WARNING, "Unknown enemy type in CreateEnemy");
            newEnemy.speed = 75.0f;
            newEnemy.hp = 50;
            newEnemy.maxHp = 50;
            newEnemy.color = PINK;
            break;
    }
    
    newEnemy.originalSpeed = newEnemy.speed; // Store original speed for slow effects
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
    selectedTowerIndex = -1; // Reset selected tower when game resets
    visualEffects.clear(); // Clear any active visual effects
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
            // Draw a subtle shadow rectangle for valid placement
            DrawRectangleRec(highlightRect, ColorAlpha(WHITE, 0.2f));
            DrawRectangleLinesEx(highlightRect, 1.0f, ColorAlpha(WHITE, 0.5f));
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

// Add a new helper function to check if mouse is over tower UI elements
bool IsMouseOverTowerUI() {
    // Only check if there's a selected tower
    if (selectedTowerIndex >= 0 && selectedTowerIndex < towers.size()) {
        Vector2 mousePos = GetMousePosition();
        
        // Check upgrade button
        Rectangle upgradeButton = { 
            (float)selectedTowerInfoX, 
            (float)(selectedTowerInfoY + infoSpacing * 6), 
            (float)upgradeButtonWidth, 
            (float)upgradeButtonHeight 
        };
        
        // Check ability button
        Rectangle abilityButton = { 
            (float)selectedTowerInfoX, 
            (float)(selectedTowerInfoY + infoSpacing * 8), 
            (float)abilityButtonWidth, 
            (float)abilityButtonHeight 
        };
        
        // Check if mouse is over the tower info panel area
        Rectangle infoPanel = {
            (float)selectedTowerInfoX - 10,
            (float)selectedTowerInfoY - 10,
            200,
            250
        };
        
        return CheckCollisionPointRec(mousePos, upgradeButton) || 
               CheckCollisionPointRec(mousePos, abilityButton) ||
               CheckCollisionPointRec(mousePos, infoPanel);
    }
    
    return false;
}

// Handle tower selection when clicking on towers - Modified to prevent deselection when clicking UI
void HandleTowerSelection() {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        // If mouse is over tower UI elements, don't change selection
        if (IsMouseOverTowerUI()) {
            return; // Keep current tower selection
        }
        
        Vector2 mousePos = GetMousePosition();
        
        // First check if we clicked on a tower
        selectedTowerIndex = -1; // Reset selection
        for (int i = 0; i < towers.size(); i++) {
            float distance = Vector2Distance(mousePos, towers[i].position);
            if (distance <= tileWidth / 2.0f) { // If clicked within tower radius
                selectedTowerIndex = i;
                break; // Found a tower to select
            }
        }
    }
}

// Handle tower upgrade button click
void HandleTowerUpgrade() {
    if (selectedTowerIndex >= 0 && selectedTowerIndex < towers.size()) {
        Tower& selectedTower = towers[selectedTowerIndex];
        int upgradeCost = GetTowerUpgradeCost(selectedTower.type, selectedTower.upgradeLevel);
        
        // Draw the upgrade button
        Rectangle upgradeButton = { 
            (float)selectedTowerInfoX, 
            (float)(selectedTowerInfoY + infoSpacing * 6), 
            (float)upgradeButtonWidth, 
            (float)upgradeButtonHeight 
        };
        
        // Only enable button if tower can be upgraded and player has enough money
        bool canUpgrade = (selectedTower.upgradeLevel < 2) && (playerMoney >= upgradeCost);
        Color buttonColor = canUpgrade ? GREEN : GRAY;
        
        DrawRectangleRec(upgradeButton, buttonColor);
        DrawRectangleLinesEx(upgradeButton, 2.0f, BLACK);
        DrawText(TextFormat("Upgrade: $%d", upgradeCost), 
                 selectedTowerInfoX + 10, 
                 selectedTowerInfoY + infoSpacing * 6 + 10, 
                 20, 
                 BLACK);
        
        // Handle button click - using IsMouseButtonReleased for more reliable detection
        if (canUpgrade && IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && 
            CheckCollisionPointRec(GetMousePosition(), upgradeButton)) {
            // Process upgrade
            playerMoney -= upgradeCost;
            selectedTower.upgradeLevel++;
            ApplyTowerUpgrade(selectedTower);
            
            // Debug output to confirm upgrade was processed
            TraceLog(LOG_INFO, "Tower upgraded to level %d", selectedTower.upgradeLevel);
        }
    }
}

// Draw towers - Modified to make range circles only visible on hover or selection
void DrawTowers() {
    // First get mouse position once for efficiency
    Vector2 mousePos = GetMousePosition();
    
    for (int i = 0; i < towers.size(); i++) {
        const auto& tower = towers[i];
        
        // Check if mouse is hovering over this tower
        float distanceToMouse = Vector2Distance(mousePos, tower.position);
        bool isHovered = distanceToMouse <= tileWidth / 2.0f;
        bool isSelected = (i == selectedTowerIndex);
        
        // Only draw range circle if tower is selected or being hovered over
        if (isSelected || isHovered) {
            Color rangeColor = isSelected ? GOLD : tower.color;
            float rangeAlpha = isSelected ? 0.2f : 0.15f;
            DrawCircleV(tower.position, tower.range, ColorAlpha(rangeColor, rangeAlpha));
        }
        
        // Highlight selected tower with enhanced visual feedback
        if (isSelected) {
            // Pulsating effect for selected tower
            float pulseScale = 1.0f + 0.1f * sin(GetTime() * 5.0f);
            
            // Draw selection ring
            DrawCircleV(tower.position, tileWidth/2.0f * pulseScale, ColorAlpha(GOLD, 0.5f));
            DrawCircleLinesV(tower.position, tileWidth/2.0f * pulseScale, GOLD);
        } else if (isHovered) {
            // Add a simple highlight for hovered towers
            DrawCircleLinesV(tower.position, tileWidth/2.0f, ColorAlpha(WHITE, 0.8f));
        }
        
        if (tower.texture.id > 0) // Check if texture is loaded (valid ID > 0)
        {
            // Define source rectangle (full texture)
            Rectangle sourceRec = { 0.0f, 0.0f, (float)tower.texture.width, (float)tower.texture.height };
            // Define destination rectangle (position and size on screen)
            Rectangle destRec = { tower.position.x, tower.position.y, (float)tileWidth, (float)tileHeight };
            // Define origin (center of the sprite for rotation and centering)
            Vector2 origin = { (float)tileWidth / 2.0f, (float)tileHeight / 2.0f };

            DrawTexturePro(tower.texture, sourceRec, destRec, origin, tower.rotationAngle, WHITE);
            
            // Draw upgrade level indicators (small dots or stars above the tower)
            if (tower.upgradeLevel > 0) {
                for (int lvl = 0; lvl < tower.upgradeLevel; lvl++) {
                    DrawCircle(
                        tower.position.x - 10 + lvl * 10, 
                        tower.position.y - tileHeight/2 - 5, 
                        3, 
                        GOLD);
                }
            }
        }
        else
        {
            // Fallback to drawing a circle if texture is not loaded
            DrawCircleV(tower.position, tileWidth / 2.5f, tower.color);
            
            // Draw upgrade level indicators for fallback visual
            if (tower.upgradeLevel > 0) {
                for (int lvl = 0; lvl < tower.upgradeLevel; lvl++) {
                    DrawCircle(
                        tower.position.x - 10 + lvl * 10, 
                        tower.position.y - tileHeight/2 - 5, 
                        3, 
                        GOLD);
                }
            }
        }
    }
}

// Handle tower firing - Modified to add special projectile behavior for Level 2 towers
void HandleTowerFiring() {
    for (auto& tower : towers) {
        if (tower.isMalfunctioning) continue; // Skip firing if tower is malfunctioning

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
            // Special behavior for Level 2 towers
            if (tower.upgradeLevel == 2) {
                // Level 2 Tier 1 Tower: Laser Beam
                if (tower.type == TIER1_DEFAULT) {
                    // Immediately apply damage - instant hit
                    int actualDamage = tower.damage;
                    
                    // Apply armor reduction if needed
                    if (target->type == ARMOURED_ENEMY || target->type == FAST_ARMOURED_ENEMY) {
                        actualDamage = (int)(actualDamage * 0.7f);
                    }
                    
                    // Apply damage to the target
                    target->hp -= actualDamage;
                    if (target->hp <= 0) {
                        target->active = false;
                        playerMoney += 10;
                        defeatedEnemies++;
                    }
                    
                    // Create laser beam visual effect
                    LaserBeam laser;
                    laser.start = tower.position;
                    laser.end = target->position;
                    laser.timer = 0.1f; // Short duration
                    laser.duration = 0.1f;
                    laser.active = true;
                    laser.color = ColorAlpha(SKYBLUE, 0.8f); // Blue laser
                    laser.thickness = 2.0f;
                    laserBeams.push_back(laser);
                    
                    // Add a flash effect at the impact point
                    VisualEffect impactEffect;
                    impactEffect.position = target->position;
                    impactEffect.lifespan = 0.2f;
                    impactEffect.timer = impactEffect.lifespan;
                    impactEffect.color = ColorAlpha(WHITE, 0.9f);
                    impactEffect.radius = 8.0f;
                    impactEffect.active = true;
                    visualEffects.push_back(impactEffect);
                    
                    // Rapid fire - shorter cooldown
                    tower.fireCooldown = 0.2f; // 5 shots per second
                }
                // Level 2 Tier 2 Tower: Flamethrower
                else if (tower.type == TIER2_FAST) {
                    // Create flamethrower projectile
                    Projectile newProjectile;
                    newProjectile.position = tower.position;
                    newProjectile.sourcePosition = tower.position;
                    newProjectile.targetEnemy = target;
                    newProjectile.speed = 150.0f; // Slightly slower than standard
                    newProjectile.damage = tower.damage;
                    newProjectile.active = true;
                    newProjectile.texture = tower.projectileTexture;
                    newProjectile.type = Projectile::Type::FLAMETHROWER;
                    newProjectile.effectRadius = 50.0f; // AOE radius
                    projectiles.push_back(newProjectile);
                    
                    tower.fireCooldown = 1.0f / tower.fireRate;
                }
                // Standard behavior for other Level 2 towers
                else {
                    // Use Power Shot ability if active
                    int projectileDamage;
                    if (tower.isPowerShotActive && tower.type == TIER3_STRONG) {
                        projectileDamage = tower.damage * 3;
                        tower.isPowerShotActive = false;
                    } else {
                        projectileDamage = tower.damage;
                    }
                    
                    // Create standard projectile
                    Projectile newProjectile;
                    newProjectile.position = tower.position;
                    newProjectile.sourcePosition = tower.position;
                    newProjectile.targetEnemy = target;
                    newProjectile.speed = 200.0f;
                    newProjectile.damage = projectileDamage;
                    newProjectile.active = true;
                    newProjectile.texture = tower.projectileTexture;
                    newProjectile.type = Projectile::Type::STANDARD;
                    projectiles.push_back(newProjectile);
                    
                    tower.fireCooldown = 1.0f / tower.fireRate;
                }
            }
            // Standard projectile behavior for non-Level 2 towers
            else {
                // Apply Power Shot ability if active
                int projectileDamage;
                if (tower.isPowerShotActive && tower.type == TIER3_STRONG) {
                    projectileDamage = tower.damage * 3;
                    tower.isPowerShotActive = false;
                } else {
                    projectileDamage = tower.damage;
                }
                
                // Create standard projectile
                Projectile newProjectile;
                newProjectile.position = tower.position;
                newProjectile.sourcePosition = tower.position;
                newProjectile.targetEnemy = target;
                newProjectile.speed = 200.0f;
                newProjectile.damage = projectileDamage;
                newProjectile.active = true;
                newProjectile.texture = tower.projectileTexture;
                newProjectile.type = Projectile::Type::STANDARD;
                projectiles.push_back(newProjectile);
                
                tower.fireCooldown = 1.0f / tower.fireRate;
            }
            
            // Add tower firing visual effect
            VisualEffect fireEffect;
            fireEffect.position = tower.position;
            fireEffect.lifespan = 0.2f;
            fireEffect.timer = fireEffect.lifespan;
            
            // Different colors for different tower types
            switch (tower.type) {
                case TIER1_DEFAULT:
                    fireEffect.color = ColorAlpha(SKYBLUE, 0.8f);
                    break;
                case TIER2_FAST:
                    fireEffect.color = ColorAlpha(LIME, 0.8f);
                    break;
                case TIER3_STRONG:
                    fireEffect.color = ColorAlpha(RED, 0.8f);
                    break;
                default:
                    fireEffect.color = ColorAlpha(WHITE, 0.8f);
                    break;
            }
            
            // Adjust effect size based on tower type and ability
            if (tower.isPowerShotActive && tower.type == TIER3_STRONG) {
                fireEffect.radius = 25.0f;
                fireEffect.color = ColorAlpha(ORANGE, 0.9f);
            } else if (tower.type == TIER2_FAST && tower.upgradeLevel == 2) {
                fireEffect.radius = 20.0f;
                fireEffect.color = ColorAlpha(ORANGE, 0.8f); // Flamethrower effect
            } else if (tower.type == TIER1_DEFAULT && tower.upgradeLevel == 2) {
                fireEffect.radius = 18.0f;
                fireEffect.color = ColorAlpha(SKYBLUE, 0.9f); // Laser effect
            } else {
                fireEffect.radius = 15.0f;
            }
            
            fireEffect.active = true;
            visualEffects.push_back(fireEffect);

            // Record firing time for malfunction system
            tower.lastFiredTime = GetTime();
        }
    }
}

// Update projectiles with damage - Modified to handle special projectiles
void UpdateProjectiles() {
    for (auto& projectile : projectiles) {
        if (!projectile.active) continue;
        if (!projectile.targetEnemy->active) {
            projectile.active = false;
            continue;
        }

        Vector2 direction = Vector2Subtract(projectile.targetEnemy->position, projectile.position);
        float distance = Vector2Length(direction);

        // Handle projectile hit
        if (distance < 5.0f) {
            // Standard projectile behavior
            if (projectile.type == Projectile::Type::STANDARD) {
                // Calculate damage with armor reduction
                int actualDamage = projectile.damage;
                
                // Apply 30% damage reduction for armored enemy types
                if (projectile.targetEnemy->type == ARMOURED_ENEMY || 
                    projectile.targetEnemy->type == FAST_ARMOURED_ENEMY) {
                    actualDamage = (int)(actualDamage * 0.7f);
                }
                
                projectile.targetEnemy->hp -= actualDamage;
                if (projectile.targetEnemy->hp <= 0) {
                    projectile.targetEnemy->active = false;
                    playerMoney += 10;
                    defeatedEnemies++;
                }
                projectile.active = false;
            }
            // Flamethrower projectile behavior (AOE DOT effect)
            else if (projectile.type == Projectile::Type::FLAMETHROWER) {
                // Create explosion visual effect
                VisualEffect explosion;
                explosion.position = projectile.position;
                explosion.lifespan = 0.5f;
                explosion.timer = explosion.lifespan;
                explosion.color = ColorAlpha(ORANGE, 0.8f);
                explosion.radius = projectile.effectRadius;
                explosion.active = true;
                visualEffects.push_back(explosion);
                
                // Apply DOT effect to all enemies within radius
                for (auto& enemy : enemies) {
                    if (!enemy.active) continue;
                    
                    float enemyDistance = Vector2Distance(projectile.position, enemy.position);
                    if (enemyDistance <= projectile.effectRadius) {
                        // Initial impact damage (reduced)
                        int initialDamage = projectile.damage / 3;
                        if (enemy.type == ARMOURED_ENEMY || enemy.type == FAST_ARMOURED_ENEMY) {
                            initialDamage = (int)(initialDamage * 0.7f);
                        }
                        enemy.hp -= initialDamage;
                        
                        // Add DOT effect
                        enemy.hasDotEffect = true;
                        enemy.dotTimer = 4.0f; // 4 second effect
                        enemy.dotTickTimer = 0.5f; // Damage every 0.5 seconds
                        enemy.dotDamage = projectile.damage / 8; // Total of 2x damage over 4 seconds
                        
                        if (enemy.hp <= 0) {
                            enemy.active = false;
                            playerMoney += 10;
                            defeatedEnemies++;
                        }
                    }
                }
                
                projectile.active = false;
            }
        } else {
            // Move projectiles toward target
            Vector2 normalizedDir = Vector2Normalize(direction);
            Vector2 velocity = Vector2Scale(normalizedDir, projectile.speed * GetFrameTime());
            projectile.position = Vector2Add(projectile.position, velocity);
            
            // Add flame particles for flamethrower projectile
            if (projectile.type == Projectile::Type::FLAMETHROWER) {
                // Create random flame particles along path
                // (This will be handled in DrawProjectiles instead for visual-only effects)
            }
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

// Update DOT effects on enemies
void UpdateDOTEffects() {
    float deltaTime = GetFrameTime();
    
    for (auto& enemy : enemies) {
        if (!enemy.active || !enemy.hasDotEffect) continue;
        
        // Update DOT timer
        enemy.dotTimer -= deltaTime;
        if (enemy.dotTimer <= 0.0f) {
            enemy.hasDotEffect = false;
            continue;
        }
        
        // Apply damage at intervals
        enemy.dotTickTimer -= deltaTime;
        if (enemy.dotTickTimer <= 0.0f) {
            enemy.hp -= enemy.dotDamage;
            
            // Create small flame visual on enemy
            VisualEffect dotEffect;
            dotEffect.position = enemy.position;
            dotEffect.lifespan = 0.3f;
            dotEffect.timer = dotEffect.lifespan;
            dotEffect.color = ColorAlpha(RED, 0.7f);
            dotEffect.radius = 10.0f;
            dotEffect.active = true;
            visualEffects.push_back(dotEffect);
            
            enemy.dotTickTimer = 0.5f; // Reset tick timer
            
            // Check if enemy died from DOT
            if (enemy.hp <= 0) {
                enemy.active = false;
                playerMoney += 10;
                defeatedEnemies++;
            }
        }
    }
}

// Update enemy slow effects
void UpdateEnemySlowEffects() {
    float deltaTime = GetFrameTime();
    
    for (auto& enemy : enemies) {
        if (enemy.isSlowed) {
            enemy.slowTimer -= deltaTime;
            if (enemy.slowTimer <= 0.0f) {
                // Reset speed when slow effect ends
                enemy.speed = enemy.originalSpeed;
                enemy.isSlowed = false;
            }
        }
    }
}

// Update tower abilities (cooldowns and duration)
void UpdateTowerAbilities() {
    float deltaTime = GetFrameTime();
    
    for (auto& tower : towers) {
        // Update cooldown timer
        if (tower.abilityCooldownTimer > 0.0f) {
            tower.abilityCooldownTimer -= deltaTime;
            if (tower.abilityCooldownTimer < 0.0f) tower.abilityCooldownTimer = 0.0f;
        }
        
        // Update ability duration
        if (tower.abilityActive) {
            tower.abilityTimer -= deltaTime;
            if (tower.abilityTimer <= 0.0f) {
                // Deactivate ability when duration ends
                tower.abilityActive = false;
                
                // Reset effects based on tower type
                switch (tower.type) {
                    case TIER2_FAST: // Reset fire rate when Speed Boost ends
                        tower.fireRate = tower.originalFireRate;
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

// Update visual effects
void UpdateVisualEffects() {
    float deltaTime = GetFrameTime();
    
    for (auto& effect : visualEffects) {
        if (!effect.active) continue;
        
        effect.timer -= deltaTime;
        if (effect.timer <= 0) {
            effect.active = false;
        }
    }
    
    // Remove inactive effects
    visualEffects.erase(
        remove_if(visualEffects.begin(), visualEffects.end(), 
                 [](const VisualEffect& e) { return !e.active; }), 
        visualEffects.end());
}

// Function to update laser beams
void UpdateLaserBeams() {
    float deltaTime = GetFrameTime();
    
    for (auto& laser : laserBeams) {
        if (!laser.active) continue;
        
        laser.timer -= deltaTime;
        if (laser.timer <= 0.0f) {
            laser.active = false;
        }
    }
    
    // Remove inactive laser beams
    laserBeams.erase(
        std::remove_if(laserBeams.begin(), laserBeams.end(),
                     [](const LaserBeam& l) { return !l.active; }),
        laserBeams.end());
}

// Update game elements
void UpdateGameElements() {
    UpdateEnemySlowEffects();   
    UpdateTowerAbilities();     
    UpdateVisualEffects();
    UpdateLaserBeams();         // Update laser beams
    UpdateDOTEffects();         // Update DOT effects
    UpdateEnemies();
    HandleTowerFiring();
    UpdateProjectiles();
    
    // Remove inactive enemies and projectiles
    enemies.erase(remove_if(enemies.begin(), enemies.end(), [](const Enemy& e) { return !e.active; }), enemies.end());
    projectiles.erase(remove_if(projectiles.begin(), projectiles.end(), [](const Projectile& p) { return !p.active; }), projectiles.end());
}

// Draw projectiles - Modified to handle special visual effects
void DrawProjectiles() {
    for (const auto& projectile : projectiles) {
        if (!projectile.active) continue;
        
        // Draw standard projectiles with texture or fallback
        if (projectile.type == Projectile::Type::STANDARD) {
            if (projectile.texture.id > 0) {
                Rectangle sourceRec = { 0.0f, 0.0f, (float)projectile.texture.width, (float)projectile.texture.height };
                Rectangle destRec = { projectile.position.x, projectile.position.y, 16, 16 };
                Vector2 origin = { 8, 8 };
                DrawTexturePro(projectile.texture, sourceRec, destRec, origin, 0.0f, WHITE);
            } else {
                DrawCircleV(projectile.position, 5.0f, YELLOW);
            }
        }
        // Draw flamethrower projectiles and flame trail
        else if (projectile.type == Projectile::Type::FLAMETHROWER) {
            // Draw the actual projectile
            if (projectile.texture.id > 0) {
                Rectangle sourceRec = { 0.0f, 0.0f, (float)projectile.texture.width, (float)projectile.texture.height };
                Rectangle destRec = { projectile.position.x, projectile.position.y, 20, 20 };
                Vector2 origin = { 10, 10 };
                DrawTexturePro(projectile.texture, sourceRec, destRec, origin, 0.0f, WHITE);
            } else {
                DrawCircleV(projectile.position, 6.0f, ORANGE);
            }
            
            // Draw flame trail from tower to projectile
            Vector2 direction = Vector2Subtract(projectile.position, projectile.sourcePosition);
            float distance = Vector2Length(direction);
            Vector2 normalized = Vector2Normalize(direction);
            
            // Generate random flame particles along the path
            const int flameParticles = 15;
            for (int i = 0; i < flameParticles; i++) {
                float t = (float)i / flameParticles;
                float randomOffset = ((float)rand() / RAND_MAX - 0.5f) * 6.0f;
                float randomOffsetY = ((float)rand() / RAND_MAX - 0.5f) * 6.0f;
                
                // Calculate base position along the line
                Vector2 basePos = Vector2Add(
                    projectile.sourcePosition,
                    Vector2Scale(direction, t)
                );
                
                // Add random offset perpendicular to direction
                Vector2 perpendicular = { -normalized.y, normalized.x };
                Vector2 offset = Vector2Scale(perpendicular, randomOffset);
                Vector2 finalPos = Vector2Add(basePos, offset);
                finalPos.y += randomOffsetY;
                
                // Size decreases from start to end
                float size = 5.0f * (1.0f - t * 0.5f);
                
                // Color transitions from ORANGE to RED
                Color flameColor;
                if (i < flameParticles / 2) {
                    flameColor = ColorAlpha(ORANGE, 0.7f - t * 0.3f);
                } else {
                    flameColor = ColorAlpha(RED, 0.7f - t * 0.3f);
                }
                
                DrawCircleV(finalPos, size, flameColor);
            }
        }
    }
    
    // Draw active laser beams
    for (const auto& laser : laserBeams) {
        if (!laser.active) continue;
        
        // Calculate alpha based on remaining time
        float alpha = laser.timer / laser.duration;
        Color laserColor = ColorAlpha(laser.color, alpha);
        
        // Draw thick laser line
        DrawLineEx(laser.start, laser.end, laser.thickness, laserColor);
        
        // Add glow effect at beginning and end
        float glowRadius = laser.thickness * 2.0f;
        DrawCircleV(laser.start, glowRadius, ColorAlpha(laser.color, alpha * 0.5f));
        DrawCircleV(laser.end, glowRadius, ColorAlpha(WHITE, alpha * 0.7f));
    }
}

// Draw visual indicators for enemies with DOT effect
void DrawEnemyEffects() {
    for (const auto& enemy : enemies) {
        if (!enemy.active) continue;
        
        // Draw DOT effect indicator
        if (enemy.hasDotEffect) {
            // Draw flames around the enemy
            const int flameCount = 6;
            for (int i = 0; i < flameCount; i++) {
                float angle = (GetTime() * 2.0f) + (i * (2.0f * PI / flameCount));
                float distance = 8.0f + sin(GetTime() * 5.0f + i) * 2.0f;
                
                Vector2 flamePos = {
                    enemy.position.x + cos(angle) * distance,
                    enemy.position.y + sin(angle) * distance
                };
                
                float flameSize = 3.0f + sin(GetTime() * 10.0f + i * 2.0f) * 1.0f;
                Color flameColor = ColorAlpha(
                    (i % 2 == 0) ? ORANGE : RED,
                    0.7f + sin(GetTime() * 8.0f + i) * 0.2f
                );
                
                DrawCircleV(flamePos, flameSize, flameColor);
            }
        }
    }
}

// Draw enemies
void DrawEnemies() {
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
}

// Draw ability effects
void DrawAbilityEffects() {
    for (const auto& tower : towers) {
        if (tower.abilityActive) {
            switch (tower.type) {
                case TIER1_DEFAULT: // Visual effect for Area Slow
                    DrawCircleV(tower.position, tower.range, ColorAlpha(SKYBLUE, 0.2f));
                    DrawCircleLinesV(tower.position, tower.range, ColorAlpha(BLUE, 0.8f));
                    break;
                    
                case TIER2_FAST: // Visual effect for Speed Boost
                    DrawCircleV(tower.position, tileWidth / 2.0f, ColorAlpha(GREEN, 0.3f));
                    break;
                    
                default:
                    break;
            }
        }
        
        // Visual effect for Power Shot ready
        if (tower.isPowerShotActive && tower.type == TIER3_STRONG) {
            DrawCircleV(tower.position, tileWidth / 2.0f, ColorAlpha(RED, 0.5f));
        }
    }
}

// Draw game elements
void DrawGameElements() {
    DrawAbilityEffects();
    DrawVisualEffects();
    DrawTowers();
    DrawEnemies();
    DrawEnemyEffects();
    DrawProjectiles();
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

// Draw selected tower info
void DrawSelectedTowerInfo() {
    if (selectedTowerIndex >= 0 && selectedTowerIndex < towers.size()) {
        const Tower& tower = towers[selectedTowerIndex];
        
        // Draw info background
        DrawRectangle(selectedTowerInfoX - 10, selectedTowerInfoY - 10, 200, 250, ColorAlpha(LIGHTGRAY, 0.8f));
        
        // Draw tower info
        DrawText("Tower Info:", selectedTowerInfoX, selectedTowerInfoY, 20, BLACK);
        DrawText(TextFormat("Type: %s", GetTowerName(tower.type)), selectedTowerInfoX, selectedTowerInfoY + infoSpacing, 18, BLACK);
        DrawText(TextFormat("Level: %d/2", tower.upgradeLevel), selectedTowerInfoX, selectedTowerInfoY + infoSpacing * 2, 18, BLACK);
        DrawText(TextFormat("Damage: %d", tower.damage), selectedTowerInfoX, selectedTowerInfoY + infoSpacing * 3, 18, BLACK);
        DrawText(TextFormat("Range: %.1f", tower.range), selectedTowerInfoX, selectedTowerInfoY + infoSpacing * 4, 18, BLACK);
        DrawText(TextFormat("Fire Rate: %.1f", tower.fireRate), selectedTowerInfoX, selectedTowerInfoY + infoSpacing * 5, 18, BLACK);
        
        // Draw the upgrade button and handle upgrades
        HandleTowerUpgrade();
        
        // Draw the ability button and handle ability activation
        HandleTowerAbilityButton();

        // NEW: If in HARD mode and the selected tower is malfunctioning, draw a repair button.
        if(currentDifficulty == HARD && towers[selectedTowerIndex].isMalfunctioning) {
            Rectangle repairButton = { (float)selectedTowerInfoX, (float)(selectedTowerInfoY + infoSpacing * 10), 150, 40 };
            DrawRectangleRec(repairButton, ORANGE);
            DrawRectangleLinesEx(repairButton, 2.0f, BLACK);
            DrawText("Repair ($50)", repairButton.x + 10, repairButton.y + 10, 18, BLACK);
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), repairButton)) {
                // Since towers is non-const in our game loop context, call:
                RepairTower(towers[selectedTowerIndex]);
            }
        }
    }
}

// Get the description of a tower's ability based on tower type
const char* GetTowerAbilityDescription(TowerType type) {
    switch (type) {
        case TIER1_DEFAULT: return "Area Slow (3s)";
        case TIER2_FAST: return "Speed Boost (5s)";
        case TIER3_STRONG: return "Power Shot (3x DMG)";
        default: return "No Ability";
    }
}

// Activate tower ability based on tower type
void ActivateTowerAbility(Tower& tower) {
    // If ability is on cooldown or already active, do nothing
    if (tower.abilityCooldownTimer > 0.0f || tower.abilityActive) return;
    
    // Set ability cooldown timer
    tower.abilityCooldownTimer = tower.abilityCooldownDuration;
    
    switch (tower.type) {
        case TIER1_DEFAULT: // Area Slow
            tower.abilityActive = true;
            tower.abilityTimer = tower.abilityDuration;
            
            // Apply slow effect to all enemies in range
            for (auto& enemy : enemies) {
                if (enemy.active) {
                    float distance = Vector2Distance(tower.position, enemy.position);
                    if (distance <= tower.range) {
                        enemy.isSlowed = true;
                        enemy.slowTimer = tower.abilityDuration;
                        enemy.speed = enemy.originalSpeed * 0.5f; // 50% slow effect
                    }
                }
            }
            break;
            
        case TIER2_FAST: // Speed Boost
            tower.abilityActive = true;
            tower.abilityTimer = tower.abilityDuration;
            tower.originalFireRate = tower.fireRate; // Store current fire rate
            tower.fireRate = tower.fireRate * 1.5f;  // 50% fire rate boost
            break;
            
        case TIER3_STRONG: // Power Shot
            tower.isPowerShotActive = true; // Next shot will be a power shot
            // This is instant, so no ability duration or active state
            break;
            
        default:
            break;
    }
}

// Handle tower ability button click
void HandleTowerAbilityButton() {
    if (selectedTowerIndex >= 0 && selectedTowerIndex < towers.size()) {
        Tower& selectedTower = towers[selectedTowerIndex];
        
        // Draw the ability button
        Rectangle abilityButton = { 
            (float)selectedTowerInfoX, 
            (float)(selectedTowerInfoY + infoSpacing * 8), 
            (float)abilityButtonWidth, 
            (float)abilityButtonHeight 
        };
        
        // Button enabled only if ability is not on cooldown
        bool canActivate = (selectedTower.abilityCooldownTimer <= 0.0f && !selectedTower.abilityActive);
        Color buttonColor = canActivate ? BLUE : GRAY;
        
        // Show different text based on ability state
        const char* buttonText;
        if (selectedTower.abilityActive) {
            buttonText = TextFormat("Active: %.1fs", selectedTower.abilityTimer);
        } else if (selectedTower.abilityCooldownTimer > 0.0f) {
            buttonText = TextFormat("Cooldown: %.1fs", selectedTower.abilityCooldownTimer);
        } else {
            buttonText = "Activate Ability";
        }
        
        DrawRectangleRec(abilityButton, buttonColor);
        DrawRectangleLinesEx(abilityButton, 2.0f, BLACK);
        
        // Center the text on the button
        int textWidth = MeasureText(buttonText, 18);
        int textX = selectedTowerInfoX + (abilityButtonWidth - textWidth) / 2;
        DrawText(buttonText, textX, selectedTowerInfoY + infoSpacing * 8 + 10, 18, WHITE);
        
        // Display ability description
        DrawText(GetTowerAbilityDescription(selectedTower.type), 
                 selectedTowerInfoX, 
                 selectedTowerInfoY + infoSpacing * 7, 
                 16, 
                 BLACK);
        
        // Handle button click
        if (canActivate && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && 
            CheckCollisionPointRec(GetMousePosition(), abilityButton)) {
            ActivateTowerAbility(selectedTower);
        }
    }
}

// Check if mouse is hovering over tower buttons and draw tooltip
void HandleTowerTooltips() {
    Vector2 mousePos = GetMousePosition();
    bool tooltipShown = false;
    
    // Check each tower button
    // Tier 1 Tower Button
    Rectangle tier1Rec = { (float)towerMenuStartX, (float)towerMenuStartY, (float)towerSelectionWidth, (float)towerSelectionHeight };
    if (CheckCollisionPointRec(mousePos, tier1Rec)) {
        DrawTowerTooltip(TIER1_DEFAULT, mousePos);
        tooltipShown = true;
    }
    
    // Tier 2 Tower Button
    Rectangle tier2Rec = { (float)towerMenuStartX + towerMenuSpacingX, (float)towerMenuStartY, (float)towerSelectionWidth, (float)towerSelectionHeight };
    if (!tooltipShown && CheckCollisionPointRec(mousePos, tier2Rec)) {
        DrawTowerTooltip(TIER2_FAST, mousePos);
        tooltipShown = true;
    }
    
    // Tier 3 Tower Button
    Rectangle tier3Rec = { (float)towerMenuStartX + towerMenuSpacingX * 2, (float)towerMenuStartY, (float)towerSelectionWidth, (float)towerSelectionHeight };
    if (!tooltipShown && CheckCollisionPointRec(mousePos, tier3Rec)) {
        DrawTowerTooltip(TIER3_STRONG, mousePos);
        tooltipShown = true;
    }
}

// Draw tower tooltip with stats
void DrawTowerTooltip(TowerType type, Vector2 position) {
    // Create a dummy tower to get stats
    Tower dummyTower = CreateTower(type, {0, 0});
    
    // Calculate tooltip dimensions based on content
    int tooltipWidth = 200;
    int tooltipHeight = 150;
    int padding = 10;
    int fontSize = 15;
    int lineHeight = fontSize + 2;
    
    // Position tooltip near the mouse but ensure it stays on screen
    float tooltipX = position.x + 20;
    if (tooltipX + tooltipWidth > screenWidth)
        tooltipX = screenWidth - tooltipWidth - 5;
        
    float tooltipY = position.y;
    if (tooltipY + tooltipHeight > screenHeight)
        tooltipY = screenHeight - tooltipHeight - 5;
        
    // Draw tooltip background with border
    DrawRectangle(tooltipX, tooltipY, tooltipWidth, tooltipHeight, ColorAlpha(LIGHTGRAY, 0.9f));
    DrawRectangleLinesEx((Rectangle){tooltipX, tooltipY, (float)tooltipWidth, (float)tooltipHeight}, 2, BLACK);
    
    // Draw tooltip content
    int textY = tooltipY + padding;
    DrawText(GetTowerName(type), tooltipX + padding, textY, fontSize + 2, BLACK);
    textY += lineHeight + 5;
    
    DrawText(TextFormat("Cost: $%d", GetTowerCost(type)), tooltipX + padding, textY, fontSize, BLACK);
    textY += lineHeight;
    
    DrawText(TextFormat("Damage: %d", dummyTower.damage), tooltipX + padding, textY, fontSize, BLACK);
    textY += lineHeight;
    
    DrawText(TextFormat("Range: %.1f", dummyTower.range), tooltipX + padding, textY, fontSize, BLACK);
    textY += lineHeight;
    
    DrawText(TextFormat("Fire Rate: %.1f", dummyTower.fireRate), tooltipX + padding, textY, fontSize, BLACK);
    textY += lineHeight;
    
    DrawText("Ability:", tooltipX + padding, textY, fontSize, BLACK);
    textY += lineHeight;
    DrawText(GetTowerAbilityDescription(type), tooltipX + padding + 10, textY, fontSize - 2, DARKGRAY);
}

// Function to draw menu screen
void DrawMenuScreen() {
    ClearBackground(BLACK);
    
    // Draw title
    const char* title = "Tower Defense Game";
    int titleFontSize = 40;
    int titleWidth = MeasureText(title, titleFontSize);
    DrawText(title, screenWidth/2 - titleWidth/2, screenHeight/4, titleFontSize, WHITE);
    
    // Draw start button
    Rectangle startButton = { screenWidth/2 - 100, screenHeight/2, 200, 60 };
    DrawRectangleRec(startButton, DARKGRAY);
    DrawRectangleLinesEx(startButton, 2, WHITE);
    
    const char* startText = "Start Game";
    int startTextWidth = MeasureText(startText, 20);
    DrawText(startText, startButton.x + (startButton.width - startTextWidth)/2, startButton.y + 20, 20, WHITE);
    
    // Draw instruction text
    DrawText("Click to place towers. Defend against waves of enemies!", 
             screenWidth/2 - MeasureText("Click to place towers. Defend against waves of enemies!", 15)/2, 
             screenHeight * 3/4, 15, LIGHTGRAY);
    
    // NEW: Difficulty selection buttons
    Rectangle easyButton = { screenWidth/4 - 50, screenHeight*0.8f, 100, 40 };
    Rectangle mediumButton = { screenWidth/2 - 50, screenHeight*0.8f, 100, 40 };
    Rectangle hardButton = { screenWidth*3/4 - 50, screenHeight*0.8f, 100, 40 };
    DrawRectangleRec(easyButton, LIGHTGRAY); DrawRectangleLinesEx(easyButton,2,WHITE); DrawText("Easy", easyButton.x+30, easyButton.y+10,20, textColor);
    DrawRectangleRec(mediumButton, LIGHTGRAY); DrawRectangleLinesEx(mediumButton,2,WHITE); DrawText("Medium", mediumButton.x+15, mediumButton.y+10,20, textColor);
    DrawRectangleRec(hardButton, LIGHTGRAY); DrawRectangleLinesEx(hardButton,2,WHITE); DrawText("Hard", hardButton.x+30, hardButton.y+10,20, textColor);
    
    // NEW: Handle button click to set difficulty
    Vector2 mp = GetMousePosition();
    if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if(CheckCollisionPointRec(mp, easyButton)) {
            currentDifficulty = EASY;
            currentWeather = WEATHER_NONE;
            currentState = PLAYING;
            ResetGame();
        }
        else if(CheckCollisionPointRec(mp, mediumButton)) {
            currentDifficulty = MEDIUM;
            currentWeather = RAIN;
            currentState = PLAYING;
            ResetGame();
        }
        else if(CheckCollisionPointRec(mp, hardButton)) {
            currentDifficulty = HARD;
            currentWeather = SNOW;
            currentState = PLAYING;
            ResetGame();
        }
    }
    
    // Check for button click
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && 
        CheckCollisionPointRec(GetMousePosition(), startButton)) {
        currentState = PLAYING;
        ResetGame();
    }
}

// Function to create fallback texture when a texture fails to load
Texture2D CreateFallbackTexture(Color color) {
    Image img = GenImageColor(64, 64, color);
    Texture2D texture = LoadTextureFromImage(img);
    UnloadImage(img);
    return texture;
}

// Draw wave progress bar
void DrawWaveProgressBar() {
    if (!waveInProgress || currentWaveIndex >= waves.size()) return;
    
    // Calculate total enemies in current wave
    const EnemyWave& currentWave = waves[currentWaveIndex];
    int totalEnemies = currentWave.basicCount + currentWave.fastCount + currentWave.armouredCount + currentWave.fastArmouredCount;
    
    if (totalEnemies <= 0) return;
    
    // Calculate progress
    float spawnProgress = (float)spawnedEnemies / totalEnemies;
    float defeatProgress = (float)defeatedEnemies / totalEnemies;
    
    // Draw background
    DrawRectangle(progressBarX - 5, progressBarY - 25, progressBarWidth + 10, progressBarHeight + 30, ColorAlpha(LIGHTGRAY, 0.7f));
    
    // Draw title
    DrawText(TextFormat("Wave %d Progress", currentWaveIndex + 1), 
             progressBarX, progressBarY - 20, 15, BLACK);
    
    // Draw empty bar
    DrawRectangle(progressBarX, progressBarY, progressBarWidth, progressBarHeight, DARKGRAY);
    
    // Draw spawn progress
    DrawRectangle(progressBarX, progressBarY, (int)(progressBarWidth * spawnProgress), progressBarHeight, BLUE);
    
    // Draw defeat progress (overlay on spawn progress)
    DrawRectangle(progressBarX, progressBarY, (int)(progressBarWidth * defeatProgress), progressBarHeight, GREEN);
    
    // Draw border
    DrawRectangleLinesEx((Rectangle){(float)progressBarX, (float)progressBarY, (float)progressBarWidth, (float)progressBarHeight}, 2, BLACK);
    
    // Draw text information
    DrawText(TextFormat("Spawned: %d/%d", spawnedEnemies, totalEnemies), 
             progressBarX, progressBarY + progressBarHeight + 5, 15, BLUE);
    DrawText(TextFormat("Defeated: %d/%d", defeatedEnemies, totalEnemies), 
             progressBarX + 120, progressBarY + progressBarHeight + 5, 15, GREEN);
}

void DrawPauseButton() {
    // Draw pause button in top-left corner
    DrawRectangleRec(pauseButton, DARKGRAY);
    DrawRectangleLinesEx(pauseButton, 2.0f, WHITE);
    
    // Draw pause icon or play icon depending on state
    if (currentState != PAUSED) {
        // Draw pause icon (two vertical bars)
        DrawRectangle(pauseButton.x + 8, pauseButton.y + 7, 5, 16, WHITE);
        DrawRectangle(pauseButton.x + 18, pauseButton.y + 7, 5, 16, WHITE);
    } else {
        // Draw play icon (triangle)
        Vector2 points[3] = {
            {pauseButton.x + 8, pauseButton.y + 7},
            {pauseButton.x + 8, pauseButton.y + 23},
            {pauseButton.x + 23, pauseButton.y + 15}
        };
        DrawTriangle(points[0], points[1], points[2], WHITE);
    }
}

// Draw wave skip button
void DrawSkipWaveButton() {
    // Skip Wave button
    if (!waveInProgress && currentWaveIndex < waves.size() && waveDelay > 0.5f) {
        showSkipButton = true;
        skipWaveButton = (Rectangle){ screenWidth - 120, 10, 110, 30 };
        
        DrawRectangleRec(skipWaveButton, DARKBLUE);
        DrawRectangleLinesEx(skipWaveButton, 2.0f, WHITE);
        DrawText("Skip Wait", skipWaveButton.x + 10, skipWaveButton.y + 7, 18, WHITE);
    } else {
        showSkipButton = false;
    }
}

void HandlePauseButton() {
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && 
        CheckCollisionPointRec(GetMousePosition(), pauseButton)) {
        if (currentState == PLAYING) {
            currentState = PAUSED;
        } else if (currentState == PAUSED) {
            currentState = PLAYING;
        }
    }
}

void HandleSkipWaveButton() {
    if (showSkipButton && IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && 
        CheckCollisionPointRec(GetMousePosition(), skipWaveButton)) {
        waveDelay = 0.0f; // Immediately start the next wave
    }
}

// Draw pause screen overlay
void DrawPauseScreen() {
    // Semi-transparent overlay
    DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(BLACK, 0.7f));
    
    // Pause text
    const char* pauseText = "GAME PAUSED";
    int fontSize = 40;
    int textWidth = MeasureText(pauseText, fontSize);
    DrawText(pauseText, screenWidth/2 - textWidth/2, screenHeight/2 - 40, fontSize, WHITE);
    
    // Instructions
    const char* instructionText = "Click the pause button to resume";
    int instructionWidth = MeasureText(instructionText, 20);
    DrawText(instructionText, screenWidth/2 - instructionWidth/2, screenHeight/2 + 20, 20, LIGHTGRAY);
}

// Draw visual effects
void DrawVisualEffects() {
    for (const auto& effect : visualEffects) {
        if (!effect.active) continue;
        
        // Calculate alpha and scale based on remaining time
        float alpha = effect.timer / effect.lifespan;
        float scale = 1.0f + (1.0f - alpha) * 0.5f; // Grow slightly as it fades
        
        // Draw effect with fading and scaling
        Color fadingColor = ColorAlpha(effect.color, alpha);
        DrawCircleV(effect.position, effect.radius * scale, fadingColor);
    }
}

// NEW: Weather particle system update function
void UpdateWeatherParticles() {
    float dt = GetFrameTime();
    if(currentDifficulty == MEDIUM) {
        // Spawn more rain particles with varied properties
        if(GetRandomValue(0,100) < 40) { // Increased spawn rate
            WeatherParticle p;
            // Randomize starting position across and above screen
            p.position = { (float)GetRandomValue(-50, screenWidth + 50), (float)GetRandomValue(-60, -5) };
            // Add slight angle to rain for more realistic appearance
            p.velocity = { 
                (float)GetRandomValue(-30, -10), // Horizontal velocity (left direction)
                (float)GetRandomValue(280, 350) // Varied falling speeds
            };
            p.lifetime = (float)GetRandomValue(15, 25) / 10.0f; // 1.5-2.5 seconds lifetime
            p.maxLifetime = p.lifetime;
            p.size = (float)GetRandomValue(10, 25) / 10.0f; // Varied sizes
            p.alpha = (float)GetRandomValue(75, 90) / 100.0f; // Higher opacity
            p.wobble = 0.0f; // Rain doesn't wobble like snow
            p.wobbleSpeed = 0.0f;
            
            // NEW: Assign a random target height for the raindrop to hit within the gameplay area
            // Ensure we have sufficient target heights throughout the gameplay area
            // 20% chance of hitting the top third of the screen
            // 30% chance of hitting the middle third
            // 50% chance of hitting the bottom third
            int heightSelector = GetRandomValue(1, 100);
            if (heightSelector <= 20) {
                // Top third of the screen
                p.targetHeight = (float)GetRandomValue(50, screenHeight/3);
            } else if (heightSelector <= 50) {
                // Middle third of the screen
                p.targetHeight = (float)GetRandomValue(screenHeight/3, 2*screenHeight/3);
            } else {
                // Bottom third of the screen
                p.targetHeight = (float)GetRandomValue(2*screenHeight/3, screenHeight - 10);
            }
            
            // Flag as not being a splash
            p.isSplash = false;
            
            weatherParticles.push_back(p);
        }
    }
    else if(currentDifficulty == HARD) {
        // Create a more natural snow effect with varied particles
        if(GetRandomValue(0,100) < 25) {
            WeatherParticle p;
            // Spawn snow particles from a higher position above the screen
            p.position = { (float)GetRandomValue(-30, screenWidth + 30), (float)GetRandomValue(-50, -5) };
            // Varied falling speeds and slight horizontal drift
            p.velocity = { (float)GetRandomValue(-15, 15) / 10.0f, (float)GetRandomValue(40, 80) };
            p.maxLifetime = (float)GetRandomValue(40, 80) / 10.0f; // 4-8 seconds
            p.lifetime = p.maxLifetime;
            p.size = (float)GetRandomValue(15, 30) / 10.0f; // Size between 1.5-3.0
            p.alpha = (float)GetRandomValue(70, 95) / 100.0f; // Alpha between 0.7-0.95
            p.wobble = (float)GetRandomValue(0, 628) / 100.0f; // Random phase (0-2π)
            p.wobbleSpeed = (float)GetRandomValue(5, 20) / 10.0f; // Different wobble speeds
            weatherParticles.push_back(p);
        }
    }
    
    // Update particles
    for(auto &p : weatherParticles) {
        if(currentDifficulty == MEDIUM) {
            if (!p.isSplash) { // Only update movement for actual raindrops, not splashes
                // Update rain with slight acceleration (simulating gravity)
                p.velocity.y += 10.0f * dt; // Rain accelerates as it falls
                p.position = Vector2Add(p.position, Vector2Scale(p.velocity, dt));
                
                // NEW: Create splash when raindrop hits its target height
                if(p.position.y >= p.targetHeight) {
                    p.lifetime = 0; // Mark for removal
                    
                    // Create a splash effect (small circle that quickly fades)
                    // Higher chance of splash for gameplay area impacts (90%), less for edges (70%)
                    int splashChance = (p.position.x > 0 && p.position.x < screenWidth) ? 9 : 7;
                    if(GetRandomValue(0, 10) < splashChance) {
                        WeatherParticle splash;
                        splash.position = { p.position.x, p.position.y }; // Splash at impact position
                        splash.velocity = { 0, 0 }; // Splashes don't move
                        splash.lifetime = 0.3f;
                        splash.maxLifetime = splash.lifetime;
                        // Splash size proportional to raindrop size for visual consistency
                        splash.size = p.size * 1.2f;
                        splash.alpha = 0.8f;
                        splash.wobble = 0.0f;
                        splash.wobbleSpeed = 0.0f;
                        splash.isSplash = true; // Flag as splash
                        splash.targetHeight = p.targetHeight; // Not used for splashes but set for completeness
                        weatherParticles.push_back(splash);
                    }
                }
            } else {
                // Update splash lifetime
                p.lifetime -= dt;
            }
        }
        else if(currentDifficulty == HARD) {
            // Update snow position with wobbling motion
            p.wobble += p.wobbleSpeed * dt;
            if(p.wobble > 2*PI) p.wobble -= 2*PI; // Keep wobble in a reasonable range
            
            // Add gentle side-to-side motion for snow
            float wobbleOffset = sinf(p.wobble) * 0.7f;
            Vector2 adjustedVelocity = p.velocity;
            adjustedVelocity.x += wobbleOffset;
            
            p.position = Vector2Add(p.position, Vector2Scale(adjustedVelocity, dt));
            
            // Calculate ground proximity (0 at top, 1 at bottom of screen)
            float groundProximity = p.position.y / screenHeight;
            
            // Gradually fade out and shrink as snow approaches the ground
            if(groundProximity > 0.85f) {
                // Calculate how close to the ground (1.0 = at ground, 0.0 = at 85% of screen height)
                float groundFactor = (1.0f - (groundProximity - 0.85f) / 0.15f);
                
                // Apply fading effects
                p.alpha *= groundFactor;
                p.size *= groundFactor;
                
                // Slow down as it reaches the ground
                p.velocity.y *= 0.98f;
            }
        }
        
        // Update lifetime for all particles
        if (!p.isSplash) { // Only decrement lifetime for non-splashes here (splashes handle their own)
            p.lifetime -= dt;
        }
    }
    
    // Remove particles that are no longer visible
    weatherParticles.erase(remove_if(weatherParticles.begin(), weatherParticles.end(),
        [](const WeatherParticle &p){ 
            return p.lifetime <= 0 || 
                   p.position.y > screenHeight || 
                   p.alpha < 0.05f || 
                   p.position.x < -50 || 
                   p.position.x > screenWidth + 50; 
        }), weatherParticles.end());
}

// NEW: Weather particle system draw function
void DrawWeatherParticles() {
    for(const auto &p : weatherParticles) {
        if(currentDifficulty == MEDIUM) {
            if (!p.isSplash) { // Regular raindrop
                // Calculate end point based on velocity for motion blur effect
                Vector2 endPoint = {
                    p.position.x + p.velocity.x * 0.03f,
                    p.position.y + p.velocity.y * 0.03f
                };
                
                // Draw the rain streak with varying length based on velocity
                float length = Vector2Length(p.velocity) * 0.05f;
                if(length < 5.0f) length = 5.0f;
                if(length > 15.0f) length = 15.0f;
                
                // Create more saturated rain color
                Color rainColor = ColorAlpha(SKYBLUE, p.alpha);
                
                // Draw the main raindrop streak
                DrawLineEx(p.position, endPoint, p.size, rainColor);
                
                // Add subtle highlight to create depth perception
                Color highlightColor = ColorAlpha(WHITE, p.alpha * 0.5f);
                Vector2 highlightPos = {p.position.x + 0.5f, p.position.y + 0.5f};
                Vector2 highlightEnd = {endPoint.x + 0.5f, endPoint.y + 0.5f};
                DrawLineEx(highlightPos, highlightEnd, p.size * 0.4f, highlightColor);
            }
            else { // Splash effect
                // Draw splash as a small circle that grows and fades
                float splashProgress = p.lifetime / p.maxLifetime; // 1.0 at start, 0.0 at end
                float expansionFactor = 1.0f + (1.0f - splashProgress) * 3.0f; // More dramatic expansion
                float splashSize = p.size * expansionFactor;
                
                // Splash gets more transparent as it expands
                Color splashColor = ColorAlpha(SKYBLUE, p.alpha * splashProgress * 0.8f);
                DrawCircleV(p.position, splashSize, splashColor);
                
                // Add ripple rings for bigger splashes
                if (p.size > 1.0f && splashProgress < 0.8f) {
                    float outerRingSize = splashSize * 1.5f;
                    float innerRingAlpha = splashProgress * 0.4f;
                    DrawCircleLines(p.position.x, p.position.y, outerRingSize, 
                                   ColorAlpha(SKYBLUE, innerRingAlpha));
                    
                    // For large splashes, add second ripple
                    if (p.size > 1.8f && splashProgress < 0.6f) {
                        DrawCircleLines(p.position.x, p.position.y, outerRingSize * 0.7f, 
                                       ColorAlpha(SKYBLUE, innerRingAlpha * 1.3f));
                    }
                }
            }
        }
        else if(currentDifficulty == HARD) {
            // Draw snowflakes with varying sizes and opacities
            Color snowColor = ColorAlpha(WHITE, p.alpha);
            
            // Draw main snowflake circle
            DrawCircleV(p.position, p.size, snowColor);
            
            // Draw a smaller, brighter center for more depth
            float innerSize = p.size * 0.6f;
            float innerBrightness = p.alpha * 1.3f;
            if(innerBrightness > 1.0f) innerBrightness = 1.0f;
            
            DrawCircleV(p.position, innerSize, ColorAlpha(WHITE, innerBrightness));
            
            // For larger snowflakes, add subtle "spokes" to mimic snowflake structure
            if(p.size > 2.0f && p.alpha > 0.5f) {
                float spokeLength = p.size * 1.2f;
                float spokeAlpha = p.alpha * 0.4f;
                
                for(int i = 0; i < 4; i++) {
                    float angle = p.wobble + (PI/4 * i);
                    Vector2 spokeEnd = {
                        p.position.x + cosf(angle) * spokeLength,
                        p.position.y + sinf(angle) * spokeLength
                    };
                    DrawLineEx(p.position, spokeEnd, 0.5f, ColorAlpha(WHITE, spokeAlpha));
                }
            }
        }
    }
}

// NEW: Tower malfunction update function
void UpdateTowerMalfunctions() {
    if(currentDifficulty != HARD) return;
    float now = GetTime();
    for(auto &tower : towers) {
        if(tower.type == NONE) continue;
        if(!tower.isMalfunctioning && (now - tower.lastFiredTime) >= 30.0f) {
            tower.isMalfunctioning = true;
            // Visual feedback: change tower color to gray
            tower.color = GRAY;
        }
    }
}

// NEW: Tower repair function
void RepairTower(Tower &tower) {
    if(tower.isMalfunctioning && playerMoney >= 50) {
        playerMoney -= 50;
        tower.isMalfunctioning = false;
        tower.lastFiredTime = GetTime();
        // Restore color based on tower type
        if(tower.type == TIER1_DEFAULT) tower.color = BLUE;
        else if(tower.type == TIER2_FAST) tower.color = GREEN;
        else if(tower.type == TIER3_STRONG) tower.color = RED;
    }
}

// Draw semi-transparent blue tint overlay for rainy atmosphere in medium difficulty
void DrawRainyAtmosphereOverlay() {
    if (currentDifficulty == MEDIUM) {
        // Draw a very subtle blue tint across the entire screen
        DrawRectangle(0, 0, screenWidth, screenHeight, 
                     ColorAlpha(DARKBLUE, 0.07f)); // Very low alpha for subtle effect
        
        // Optional: Add some darker areas at the top to simulate storm clouds
        for (int i = 0; i < 5; i++) {
            float yPos = i * 20.0f - 50.0f;
            float alpha = 0.03f - (i * 0.005f); // Decreasing alpha for fade effect
            if (alpha > 0) {
                DrawRectangle(0, yPos, screenWidth, 100, 
                             ColorAlpha(DARKBLUE, alpha));
            }
        }
    }
}

int main() {
    InitWindow(screenWidth, screenHeight, "Robust Tower Defense - v0.2");
    SetTargetFPS(60);

    // Load textures - Load them globally after InitWindow
    tier1TowerTexture = LoadTexture("tier1tower.png");
    if (tier1TowerTexture.id == 0) {
        TraceLog(LOG_ERROR, "Failed to load texture: tier1tower.png (Global Load)");
        tier1TowerTexture = CreateFallbackTexture(BLUE);
    }
    tier2TowerTexture = LoadTexture("tier2tower.png");
    if (tier2TowerTexture.id == 0) {
        TraceLog(LOG_ERROR, "Failed to load texture: tier2tower.png (Global Load)");
        tier2TowerTexture = CreateFallbackTexture(GREEN);
    }
    tier3TowerTexture = LoadTexture("tier3tower.png");
    if (tier3TowerTexture.id == 0) {
        TraceLog(LOG_ERROR, "Failed to load texture: tier3tower.png (Global Load)");
        tier3TowerTexture = CreateFallbackTexture(RED);
    }
    placeholderTexture = LoadTexture("placeholder.png");
    if (placeholderTexture.id == 0) {
        TraceLog(LOG_ERROR, "Failed to load texture: placeholder.png (Global Load)");
        placeholderTexture = CreateFallbackTexture(WHITE);
    }
    tier1ProjectileTexture = LoadTexture("tier1projectile.png");
    if (tier1ProjectileTexture.id == 0) {
        TraceLog(LOG_ERROR, "Failed to load texture: tier1projectile.png (Global Load)");
        tier1ProjectileTexture = CreateFallbackTexture(SKYBLUE);
    }
    tier2ProjectileTexture = LoadTexture("tier2projectile.png");
    if (tier2ProjectileTexture.id == 0) {
        TraceLog(LOG_ERROR, "Failed to load texture: tier2projectile.png (Global Load)");
        tier2ProjectileTexture = CreateFallbackTexture(LIME);
    }
    tier3ProjectileTexture = LoadTexture("tier3projectile.png");
    if (tier3ProjectileTexture.id == 0) {
        TraceLog(LOG_ERROR, "Failed to load texture: tier3projectile.png (Global Load)");
        tier3ProjectileTexture = CreateFallbackTexture(ORANGE);
    }
    tier1EnemyTexture = LoadTexture("tier1enemy.png");
    if (tier1EnemyTexture.id == 0) {
        TraceLog(LOG_ERROR, "Failed to load texture: tier1enemy.png (Global Load)");
        tier1EnemyTexture = CreateFallbackTexture(RED);
    }
    tier2EnemyTexture = LoadTexture("tier2enemy.png");
    if (tier2EnemyTexture.id == 0) {
        TraceLog(LOG_ERROR, "Failed to load texture: tier2enemy.png (Global Load)");
        tier2EnemyTexture = CreateFallbackTexture(YELLOW);
    }
    backgroundTexture = LoadTexture("towerdefensegrass.png"); // Load background texture
    if (backgroundTexture.id == 0) {
        TraceLog(LOG_ERROR, "Failed to load texture: towerdefensegrass.png (Background)");
        backgroundTexture = CreateFallbackTexture(DARKGREEN);
    }
    // Load new enemy textures
    tier3EnemyTexture = LoadTexture("tier3enemy.png"); // For armored enemy
    if (tier3EnemyTexture.id == 0) {
        TraceLog(LOG_ERROR, "Failed to load texture: tier3enemy.png (Global Load)");
        tier3EnemyTexture = CreateFallbackTexture(DARKGRAY);
    }
    tier4EnemyTexture = LoadTexture("tier4enemy.png"); // For fast armored enemy
    if (tier4EnemyTexture.id == 0) {
        TraceLog(LOG_ERROR, "Failed to load texture: tier4enemy.png (Global Load)");
        tier4EnemyTexture = CreateFallbackTexture(GOLD);
    }
    
    // Load border grid textures
    bottomGridTexture = LoadTexture("bottomsidegrid.png");
    if (bottomGridTexture.id == 0) {
        TraceLog(LOG_ERROR, "Failed to load texture: bottomsidegrid.png");
        bottomGridTexture = CreateFallbackTexture(BROWN);
    }
    
    leftGridTexture = LoadTexture("leftsidegrid.png");
    if (leftGridTexture.id == 0) {
        TraceLog(LOG_ERROR, "Failed to load texture: leftsidegrid.png");
        leftGridTexture = CreateFallbackTexture(DARKBLUE);
    }
    
    topGridTexture = LoadTexture("topsidegrid.png");
    if (topGridTexture.id == 0) {
        TraceLog(LOG_ERROR, "Failed to load texture: topsidegrid.png");
        topGridTexture = CreateFallbackTexture(PURPLE);
    }
    
    secondRightmostTexture = LoadTexture("secondrightmost.png");
    if (secondRightmostTexture.id == 0) {
        TraceLog(LOG_ERROR, "Failed to load texture: secondrightmost.png");
        secondRightmostTexture = CreateFallbackTexture(GRAY);
    }
    
    rightGridTexture = LoadTexture("rightsidegrid.png");
    if (rightGridTexture.id == 0) {
        TraceLog(LOG_ERROR, "Failed to load texture: rightsidegrid.png");
        rightGridTexture = CreateFallbackTexture(MAROON);
    }

    InitGrid();
    InitWaypoints();

    while (!WindowShouldClose()) {
        // Debug key to force state change
        if (IsKeyPressed(KEY_P)) {
            currentState = PLAYING;
            ResetGame();
        }
        
        if (currentState == PLAYING || currentState == PAUSED) {
            // Handle pause button regardless of state
            HandlePauseButton();
            
            // Only handle skip wave button if not paused
            if (currentState == PLAYING) {
                HandleSkipWaveButton();
            }
            
            // Only update game logic if not paused
            if (currentState == PLAYING) {
                HandleTowerMenuClick();
                HandleTowerSelection();
                HandleTowerPlacement();
                UpdateGameElements();

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
                    int totalEnemies = waves[currentWaveIndex].basicCount + waves[currentWaveIndex].fastCount + 
                                    waves[currentWaveIndex].armouredCount + waves[currentWaveIndex].fastArmouredCount;
                                    
                    if (waveTimer <= 0.0f && spawnedEnemies < totalEnemies) {
                        EnemyType type;
                        
                        // Determine which enemy type to spawn next
                        if (spawnedEnemies < waves[currentWaveIndex].basicCount) {
                            type = BASIC_ENEMY;
                        } else if (spawnedEnemies < waves[currentWaveIndex].basicCount + waves[currentWaveIndex].fastCount) {
                            type = FAST_ENEMY;
                        } else if (spawnedEnemies < waves[currentWaveIndex].basicCount + waves[currentWaveIndex].fastCount + 
                                                waves[currentWaveIndex].armouredCount) {
                            type = ARMOURED_ENEMY;
                        } else {
                            type = FAST_ARMOURED_ENEMY;
                        }
                        
                        // Spawn enemies from left edge, vertically centered
                        float spawnX = 0; // Left edge X
                        float spawnY = waypoints[0].y; // Vertically centered Y
                        enemies.push_back(CreateEnemy(type, {spawnX, spawnY}));
                        waveTimer = waves[currentWaveIndex].spawnInterval;
                        spawnedEnemies++;

                        // NEW: Adjust enemy HP based on difficulty
                        Enemy &e = enemies.back();
                        if(currentDifficulty == MEDIUM) {
                            e.maxHp = (int)(e.maxHp * 1.2f);
                            e.hp = e.maxHp;
                        } else if(currentDifficulty == HARD) {
                            e.maxHp = (int)(e.maxHp * 1.4f);
                            e.hp = e.maxHp;
                        }
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

                // Update tower animations (idle animations)
                for (auto& tower : towers) {
                    tower.rotationAngle += tower.rotationSpeed * GetFrameTime();
                    if (tower.rotationAngle > 360.0f) tower.rotationAngle -= 360.0f; // Keep angle within 0-360
                }

                // NEW: Update weather particles and tower malfunctions
                UpdateWeatherParticles();
                if(currentDifficulty == HARD) { UpdateTowerMalfunctions(); }
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);

        if (currentState == MENU) {
            DrawMenuScreen();
        }
        else if (currentState == PLAYING || currentState == PAUSED) {
            // Draw game elements regardless of pause state
            
            // Replace the background drawing code with this:
            // Draw background with border textures
            for (int row = 0; row < gridRows; row++) {
                for (int col = 0; col < gridColumns; col++) {
                    Rectangle sourceRec = { 0.0f, 0.0f, (float)backgroundTexture.width, (float)backgroundTexture.height };
                    Rectangle destRec = { 
                        (float)(col * tileWidth), 
                        (float)(row * tileHeight), 
                        (float)tileWidth, 
                        (float)tileHeight 
                    };
                    Vector2 origin = { 0.0f, 0.0f };
                    
                    // Default texture
                    Texture2D* textureToUse = &backgroundTexture;
                    
                    // Apply border textures based on position - prioritizing columns over rows for corners
                    if (col == 0) { // Left column (highest priority)
                        textureToUse = &leftGridTexture;
                    } else if (col == gridColumns - 1) { // Rightmost column
                        textureToUse = &rightGridTexture;
                    } else if (col == gridColumns - 2) { // Second-rightmost column
                        textureToUse = &secondRightmostTexture;
                    } else if (row == 0) { // Top row
                        textureToUse = &topGridTexture;
                    } else if (row == gridRows - 1) { // Bottom row (lowest priority)
                        textureToUse = &bottomGridTexture;
                    }
                    
                    DrawTexturePro(*textureToUse, sourceRec, destRec, origin, 0.0f, WHITE);
                }
            }

            // Draw path indicator
            for (size_t i = 0; i < waypoints.size() - 1; ++i) {
                DrawLineV(waypoints[i], waypoints[i+1], ColorAlpha(LIGHTGRAY, 0.5f)); // Made path lines semi-transparent
            }

            // Remove DrawGrid() call and only keep highlight
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

            // Draw all game elements
            DrawGameElements();
            
            // NEW: Draw rainy atmosphere overlay for medium difficulty
            DrawRainyAtmosphereOverlay();

            // NEW: Draw weather particles
            if(currentDifficulty == MEDIUM || currentDifficulty == HARD) {
                DrawWeatherParticles();
            }

            // UI - Cleaned up and reorganized (UI on top of everything)
            DrawText("Tower Defense Game - Tower Placement", titleX - MeasureText("Tower Defense Game - Tower Placement", regularTextFontSize) / 2, titleY, regularTextFontSize, textColor); // Centered title
            DrawText(TextFormat("Money: %d", playerMoney), moneyX, moneyY, regularTextFontSize, textColor);
            DrawText(TextFormat("Escaped: %d/%d", enemiesReachedEnd, maxEnemiesReachedEnd), escapedX, escapedY, regularTextFontSize, RED); // Escaped below Money

            // Wave information - Below escaped count
            if (waveInProgress) {
                int totalEnemies = waves[currentWaveIndex].basicCount + waves[currentWaveIndex].fastCount + waves[currentWaveIndex].armouredCount + waves[currentWaveIndex].fastArmouredCount;
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

            // Draw selected tower info if a tower is selected
            DrawSelectedTowerInfo();

            // Draw wave progress bar
            DrawWaveProgressBar();

            // Tower tooltip handling - must be drawn last to appear on top
            HandleTowerTooltips();

            // Draw pause button and skip wave button (new features)
            DrawPauseButton();
            DrawSkipWaveButton();
            
            // If game is paused, draw pause screen overlay
            if (currentState == PAUSED) {
                DrawPauseScreen();
            }

            // NEW: Draw weather particles
            if(currentDifficulty == MEDIUM || currentDifficulty == HARD) {
                DrawWeatherParticles();
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
    UnloadTexture(tier3EnemyTexture);
    UnloadTexture(tier4EnemyTexture);
    // Unload border grid textures
    UnloadTexture(bottomGridTexture);
    UnloadTexture(leftGridTexture);
    UnloadTexture(topGridTexture);
    UnloadTexture(secondRightmostTexture);
    UnloadTexture(rightGridTexture);

    CloseWindow();
    return 0;
}