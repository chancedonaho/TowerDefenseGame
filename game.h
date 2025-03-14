#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <algorithm>
#include <string>
#include <iostream>
#include <queue>
#include <map>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

// Constants
const int screenWidth = 800;
const int screenHeight = 600;
const int gridColumns = 15;
const int gridRows = 10;
const int tileWidth = screenWidth / gridColumns;
const int tileHeight = screenHeight / gridRows;

// UI constants
const int uiPadding = 10;
const int towerSelectionWidth = 80;
const int towerSelectionHeight = 60;
const int towerTypeTextSize = 18;
const int largeTextFontSize = 30;
const int regularTextFontSize = 18;
const Color textColor = DARKGRAY;
const int titleX = screenWidth / 2;
const int titleY = uiPadding;
const int moneyX = uiPadding;
const int moneyY = titleY;
const int escapedX = uiPadding;
const int escapedY = moneyY + 25;
const int waveInfoX = uiPadding;
const int waveInfoY = escapedY + 25;
const int nextWaveTimerY = screenHeight * 0.15f;
const int towerMenuStartY = uiPadding + 70;
const int towerMenuStartX = uiPadding;
const int towerMenuSpacingX = uiPadding + towerSelectionWidth;
const int selectedTowerTextY = towerMenuStartY + towerSelectionHeight + 10;
const int selectedTowerInfoX = screenWidth - 200;
const int selectedTowerInfoY = 100;
const int upgradeButtonWidth = 120;
const int upgradeButtonHeight = 40;
const int infoSpacing = 25;
const int abilityButtonWidth = 150;
const int abilityButtonHeight = 40;
const int progressBarWidth = 200;
const int progressBarHeight = 15;
const int progressBarX = screenWidth - progressBarWidth - 20;
const int progressBarY = 60;
const int maxEnemiesReachedEnd = 10;

// Structs and Enums
struct Vector2Int {
    int x;
    int y;
};

enum TowerType {
    NONE,
    TIER1_DEFAULT,
    TIER2_FAST,
    TIER3_STRONG,
    TOWER_TYPE_COUNT
};

struct Tower {
    Vector2 position;
    Color color;
    float range;
    float fireRate;
    float fireCooldown;
    TowerType type;
    int damage;
    Texture2D texture;
    float rotationAngle;
    float rotationSpeed;
    Texture2D projectileTexture;
    int upgradeLevel;
    float abilityCooldownTimer;
    float abilityCooldownDuration;
    bool abilityActive;
    float abilityDuration;
    float abilityTimer;
    float originalFireRate;
    bool isPowerShotActive;
    float lastFiredTime;
    bool isMalfunctioning;
};

enum EnemyType {
    BASIC_ENEMY,
    FAST_ENEMY,
    ARMOURED_ENEMY,
    FAST_ARMOURED_ENEMY,
    ENEMY_TYPE_COUNT
};

struct Enemy {
    Vector2 position;
    float speed;
    bool active;
    int currentWaypoint;
    int hp;
    int maxHp;
    EnemyType type;
    Color color;
    vector<Vector2Int> waypointsPath;
    int pathIndex;
    float pathCheckTimer; // Add path check timer for each enemy
    Enemy() : pathIndex(0), pathCheckTimer(0.0f) {}
    Texture2D texture;
    float originalSpeed;
    float slowTimer;
    bool isSlowed;
    bool hasDotEffect;
    float dotTimer;
    float dotTickTimer;
    int dotDamage;
};

struct Projectile {
    Vector2 position;
    Enemy* targetEnemy;
    float speed;
    int damage;
    bool active;
    Texture2D texture;
    enum class Type { STANDARD, FLAMETHROWER } type;
    Vector2 sourcePosition;
    float effectRadius;
};

struct EnemyWave {
    int basicCount;
    int fastCount;
    int armouredCount;
    int fastArmouredCount;
    float spawnInterval;
};

enum GameState {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER,
    WIN
};

struct VisualEffect {
    Vector2 position;
    float lifespan;
    float timer;
    Color color;
    float radius;
    bool active;
};

struct LaserBeam {
    Vector2 start;
    Vector2 end;
    float timer;
    float duration;
    bool active;
    Color color;
    float thickness;
};

enum MapDifficulty { EASY, MEDIUM, HARD };
enum WeatherType { WEATHER_NONE, RAIN, SNOW };

struct WeatherParticle {
    Vector2 position;
    Vector2 velocity;
    float lifetime;
    float maxLifetime;
    float size;
    float alpha;
    float wobble;
    float wobbleSpeed;
    float targetHeight;
    bool isSplash;
};

// Global Variables (extern declarations)
extern bool grid[gridRows][gridColumns];
extern vector<Tower> towers;
extern vector<Enemy> enemies;
extern vector<Projectile> projectiles;
extern vector<Vector2> waypoints;
extern vector<EnemyWave> waves;
extern vector<VisualEffect> visualEffects;
extern vector<LaserBeam> laserBeams;
extern vector<WeatherParticle> weatherParticles;
extern int playerMoney;
extern TowerType selectedTowerType;
extern int currentWaveIndex;
extern float waveTimer;
extern float waveDelay;
extern bool waveInProgress;
extern int spawnedEnemies;
extern int defeatedEnemies;
extern int enemiesReachedEnd;
extern GameState currentState;
extern int selectedTowerIndex;
extern MapDifficulty currentDifficulty;
extern WeatherType currentWeather;
extern Texture2D tier1TowerTexture;
extern Texture2D tier2TowerTexture;
extern Texture2D tier3TowerTexture;
extern Texture2D placeholderTexture;
extern Texture2D tier1ProjectileTexture;
extern Texture2D tier2ProjectileTexture;
extern Texture2D tier3ProjectileTexture;
extern Texture2D tier1EnemyTexture;
extern Texture2D tier2EnemyTexture;
extern Texture2D backgroundTexture;
extern Texture2D tier3EnemyTexture;
extern Texture2D tier4EnemyTexture;
extern Texture2D bottomGridTexture;
extern Texture2D leftGridTexture;
extern Texture2D topGridTexture;
extern Texture2D secondRightmostTexture;
extern Texture2D rightGridTexture;
extern Rectangle pauseButton;
extern bool isPaused;
extern Rectangle skipWaveButton;
extern bool showSkipButton;

// Function Prototypes
void InitGrid();
void InitWaypoints();
Vector2Int GetGridCoords(Vector2 position);
Vector2 GetTileCenter(Vector2Int gridCoords);
vector<Vector2Int> FindPathBFS(Vector2Int start, Vector2Int end);
void DrawPath(const vector<Vector2Int>& path, Color color);
Tower CreateTower(TowerType type, Vector2 position);
int GetTowerCost(TowerType type);
const char* GetTowerName(TowerType type);
int GetTowerUpgradeCost(TowerType type, int currentLevel);
void ApplyTowerUpgrade(Tower& tower);
void HandleTowerPlacement();
bool IsMouseOverTowerUI();
void HandleTowerSelection();
void HandleTowerUpgrade();
void DrawTowers();
void HandleTowerFiring();
void ActivateTowerAbility(Tower& tower);
void HandleTowerAbilityButton();
void RepairTower(Tower& tower);
Enemy CreateEnemy(EnemyType type, Vector2 startPosition);
void UpdateEnemies();
void DrawEnemies();
void UpdateProjectiles();
void DrawProjectiles();
void ResetGame();
void DrawGridHighlight();
void UpdateGameElements();
void HandleTowerMenuClick();
void DrawMenuScreen();
void DrawGameElements();
void DrawSelectedTowerInfo();
void DrawWaveProgressBar();
void DrawPauseButton();
void DrawSkipWaveButton();
void HandlePauseButton();
void HandleSkipWaveButton();
void DrawPauseScreen();
void DrawVisualEffects();
void DrawTowerTooltip(TowerType type, Vector2 position);
void UpdateWeatherParticles();
void DrawWeatherParticles();
void UpdateTowerMalfunctions();
void DrawRainyAtmosphereOverlay();

#endif // GAME_H