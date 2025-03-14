#include "game.h"

// Define global variables
bool grid[gridRows][gridColumns];
vector<Tower> towers;
vector<Enemy> enemies;
vector<Projectile> projectiles;
vector<Vector2> waypoints;
vector<EnemyWave> waves = {
    {5, 0, 0, 0, 1.0f}, {3, 2, 0, 0, 0.8f}, {0, 5, 0, 0, 0.5f},
    {5, 0, 2, 0, 0.9f}, {2, 2, 2, 0, 0.7f}, {0, 5, 0, 2, 0.4f},
    {5, 0, 5, 0, 0.8f}, {0, 5, 0, 3, 0.6f}, {0, 0, 5, 5, 0.3f},
    {10, 5, 5, 5, 0.5f}
};
vector<VisualEffect> visualEffects;
vector<LaserBeam> laserBeams;
vector<WeatherParticle> weatherParticles;
int playerMoney = 100;
TowerType selectedTowerType = NONE;
int currentWaveIndex = 0;
float waveTimer = 0.0f;
float waveDelay = 15.0f;
bool waveInProgress = false;
int spawnedEnemies = 0;
int defeatedEnemies = 0;
int enemiesReachedEnd = 0;
GameState currentState = MENU;
int selectedTowerIndex = -1;
MapDifficulty currentDifficulty = EASY;
WeatherType currentWeather = WEATHER_NONE;
Texture2D tier1TowerTexture;
Texture2D tier2TowerTexture;
Texture2D tier3TowerTexture;
Texture2D placeholderTexture;
Texture2D tier1ProjectileTexture;
Texture2D tier2ProjectileTexture;
Texture2D tier3ProjectileTexture;
Texture2D tier1EnemyTexture;
Texture2D tier2EnemyTexture;
Texture2D backgroundTexture;
Texture2D tier3EnemyTexture;
Texture2D tier4EnemyTexture;
Texture2D bottomGridTexture;
Texture2D leftGridTexture;
Texture2D topGridTexture;
Texture2D secondRightmostTexture;
Texture2D rightGridTexture;
Rectangle pauseButton = { 10, 10, 30, 30 };
bool isPaused = false;
Rectangle skipWaveButton;
bool showSkipButton = false;

void InitGrid() {
    for (int row = 0; row < gridRows; ++row) {
        for (int col = 0; col < gridColumns; ++col) {
            grid[row][col] = true;
        }
    }
    if (currentDifficulty == MEDIUM) {
        for (int row = 2; row < 4; row++) {
            for (int col = 2; col < 6; col++) {
                grid[row][col] = false;
            }
        }
        grid[6][4] = false; grid[6][5] = false;
        grid[7][4] = false; grid[7][5] = false;
    } else if (currentDifficulty == HARD) {
        for (int col = 2; col < 5; col++) grid[2][col] = false;
        for (int col = 10; col < 13; col++) grid[2][col] = false;
        for (int col = 4; col < 7; col++) grid[4][col] = false;
        for (int col = 8; col < 11; col++) grid[6][col] = false;
        for (int row = 6; row < 8; row++) grid[row][3] = false;
    }
}

void InitWaypoints() {
    waypoints.clear();
    
    // Use the same "easy" path for all difficulty levels
    int pathRow = gridRows / 2;
    for (int col = 0; col < gridColumns; ++col) {
        waypoints.push_back({ (float)(col * tileWidth + tileWidth / 2), (float)(pathRow * tileHeight + tileHeight / 2) });
    }
    
    // Original code with different paths per difficulty has been replaced
}

void ResetGame() {
    towers.clear();
    enemies.clear();
    projectiles.clear();
    weatherParticles.clear();
    laserBeams.clear();
    visualEffects.clear();
    if (currentDifficulty == EASY) playerMoney = 120;
    else if (currentDifficulty == MEDIUM) playerMoney = 100;
    else if (currentDifficulty == HARD) playerMoney = 80;
    selectedTowerType = NONE;
    currentWaveIndex = 0;
    waveTimer = 0.0f;
    waveDelay = 15.0f;
    waveInProgress = false;
    spawnedEnemies = 0;
    defeatedEnemies = 0;
    enemiesReachedEnd = 0;
    selectedTowerIndex = -1;
    InitGrid();
    InitWaypoints();
}

void DrawGridHighlight() {
    Vector2 mousePos = GetMousePosition();
    int gridCol = mousePos.x / tileWidth;
    int gridRow = mousePos.y / tileHeight;
    if (gridCol >= 0 && gridCol < gridColumns && gridRow >= 0 && gridRow < gridRows) {
        Rectangle highlightRect = { (float)gridCol * tileWidth, (float)gridRow * tileHeight, (float)tileWidth, (float)tileHeight };
        if (grid[gridRow][gridCol]) {
            DrawRectangleRec(highlightRect, ColorAlpha(WHITE, 0.2f));
            DrawRectangleLinesEx(highlightRect, 1.0f, ColorAlpha(WHITE, 0.5f));
        } else {
            DrawRectangleRec(highlightRect, ColorAlpha(RED, 0.5f));
        }
    }
}

void UpdateGameElements() {
    UpdateEnemies();
    HandleTowerFiring();
    UpdateProjectiles();
    for (auto& effect : visualEffects) {
        if (effect.active) {
            effect.timer -= GetFrameTime();
            if (effect.timer <= 0.0f) effect.active = false;
        }
    }
    for (auto& beam : laserBeams) {
        if (beam.active) {
            beam.timer -= GetFrameTime();
            if (beam.timer <= 0.0f) beam.active = false;
        }
    }
    visualEffects.erase(remove_if(visualEffects.begin(), visualEffects.end(), [](const VisualEffect& e) { return !e.active; }), visualEffects.end());
    laserBeams.erase(remove_if(laserBeams.begin(), laserBeams.end(), [](const LaserBeam& b) { return !b.active; }), laserBeams.end());
    for (auto& tower : towers) {
        if (tower.abilityCooldownTimer > 0.0f) tower.abilityCooldownTimer -= GetFrameTime();
        if (tower.abilityActive) {
            tower.abilityTimer -= GetFrameTime();
            if (tower.abilityTimer <= 0.0f) {
                tower.abilityActive = false;
                if (tower.type == TIER2_FAST) tower.fireRate = tower.originalFireRate;
            }
        }
    }
    for (auto& enemy : enemies) {
        if (!enemy.active) continue;
        if (enemy.isSlowed) {
            enemy.slowTimer -= GetFrameTime();
            if (enemy.slowTimer <= 0.0f) {
                enemy.isSlowed = false;
                enemy.speed = enemy.originalSpeed;
            }
        }
        if (enemy.hasDotEffect) {
            enemy.dotTimer -= GetFrameTime();
            enemy.dotTickTimer -= GetFrameTime();
            if (enemy.dotTickTimer <= 0.0f) {
                enemy.hp -= enemy.dotDamage;
                enemy.dotTickTimer = 0.5f;
                if (enemy.hp <= 0) {
                    enemy.active = false;
                    playerMoney += 10;
                    defeatedEnemies++;
                }
            }
            if (enemy.dotTimer <= 0.0f) enemy.hasDotEffect = false;
        }
    }
    enemies.erase(remove_if(enemies.begin(), enemies.end(), [](const Enemy& e) { return !e.active; }), enemies.end());
    projectiles.erase(remove_if(projectiles.begin(), projectiles.end(), [](const Projectile& p) { return !p.active; }), projectiles.end());
}

void HandleTowerMenuClick() {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mousePos = GetMousePosition();
        int currentMenuX = towerMenuStartX;
        Rectangle tier1Rec = { (float)currentMenuX, (float)towerMenuStartY, (float)towerSelectionWidth, (float)towerSelectionHeight };
        currentMenuX += towerMenuSpacingX;
        Rectangle tier2Rec = { (float)currentMenuX, (float)towerMenuStartY, (float)towerSelectionWidth, (float)towerSelectionHeight };
        currentMenuX += towerMenuSpacingX;
        Rectangle tier3Rec = { (float)currentMenuX, (float)towerMenuStartY, (float)towerSelectionWidth, (float)towerSelectionHeight };
        if (CheckCollisionPointRec(mousePos, tier1Rec)) selectedTowerType = TIER1_DEFAULT;
        else if (CheckCollisionPointRec(mousePos, tier2Rec)) selectedTowerType = TIER2_FAST;
        else if (CheckCollisionPointRec(mousePos, tier3Rec)) selectedTowerType = TIER3_STRONG;
    }
}

void DrawMenuScreen() {
    ClearBackground(BLACK);
    const char* title = "Tower Defense Game";
    int titleFontSize = 40;
    int titleWidth = MeasureText(title, titleFontSize);
    DrawText(title, screenWidth / 2 - titleWidth / 2, screenHeight / 4, titleFontSize, WHITE);
    
    // Draw instruction text
    DrawText("Select a difficulty level to start:", 
             screenWidth / 2 - MeasureText("Select a difficulty level to start:", 20) / 2, 
             screenHeight * 0.4f, 20, WHITE);
    
    // Make difficulty buttons larger and more prominent
    float buttonWidth = 160;
    float buttonHeight = 60;
    float buttonSpacing = 30;
    float totalWidth = 3 * buttonWidth + 2 * buttonSpacing;
    float startX = (screenWidth - totalWidth) / 2;
    float buttonY = screenHeight * 0.55f;
    
    // Easy button
    Rectangle easyButton = { startX, buttonY, buttonWidth, buttonHeight };
    Color easyColor = (currentDifficulty == EASY) ? GREEN : DARKGREEN;
    DrawRectangleRec(easyButton, easyColor);
    DrawRectangleLinesEx(easyButton, 3, WHITE);
    
    const char* easyText = "Play Easy";
    int easyTextWidth = MeasureText(easyText, 24);
    DrawText(easyText, easyButton.x + (buttonWidth - easyTextWidth)/2, easyButton.y + buttonHeight/2 - 12, 24, WHITE);
    
    // Medium button
    Rectangle mediumButton = { startX + buttonWidth + buttonSpacing, buttonY, buttonWidth, buttonHeight };
    Color mediumColor = (currentDifficulty == MEDIUM) ? GREEN : DARKGREEN;
    DrawRectangleRec(mediumButton, mediumColor);
    DrawRectangleLinesEx(mediumButton, 3, WHITE);
    
    const char* mediumText = "Play Medium";
    int mediumTextWidth = MeasureText(mediumText, 24);
    DrawText(mediumText, mediumButton.x + (buttonWidth - mediumTextWidth)/2, mediumButton.y + buttonHeight/2 - 12, 24, WHITE);
    
    // Hard button
    Rectangle hardButton = { startX + 2 * (buttonWidth + buttonSpacing), buttonY, buttonWidth, buttonHeight };
    Color hardColor = (currentDifficulty == HARD) ? GREEN : DARKGREEN;
    DrawRectangleRec(hardButton, hardColor);
    DrawRectangleLinesEx(hardButton, 3, WHITE);
    
    const char* hardText = "Play Hard";
    int hardTextWidth = MeasureText(hardText, 24);
    DrawText(hardText, hardButton.x + (buttonWidth - hardTextWidth)/2, hardButton.y + buttonHeight/2 - 12, 24, WHITE);
    
    // Display description of selected difficulty
    float descY = buttonY + buttonHeight + 40;
    if (currentDifficulty == EASY) {
        DrawText("Easy: Standard path, more starting money, normal enemies", 
                 screenWidth / 2 - MeasureText("Easy: Standard path, more starting money, normal enemies", 18) / 2, 
                 descY, 18, GREEN);
    } else if (currentDifficulty == MEDIUM) {
        DrawText("Medium: Curved path with water obstacles, rain affects visibility", 
                 screenWidth / 2 - MeasureText("Medium: Curved path with water obstacles, rain affects visibility", 18) / 2, 
                 descY, 18, YELLOW);
    } else if (currentDifficulty == HARD) {
        DrawText("Hard: Complex path, less money, towers can malfunction, snowstorm", 
                 screenWidth / 2 - MeasureText("Hard: Complex path, less money, towers can malfunction, snowstorm", 18) / 2, 
                 descY, 18, RED);
    }
    
    // Handle button clicks - start game immediately when difficulty is selected
    Vector2 mp = GetMousePosition();
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mp, easyButton)) {
            currentDifficulty = EASY;
            currentWeather = WEATHER_NONE;
            currentState = PLAYING;
            ResetGame();
        } else if (CheckCollisionPointRec(mp, mediumButton)) {
            currentDifficulty = MEDIUM;
            currentWeather = RAIN;
            currentState = PLAYING;
            ResetGame();
        } else if (CheckCollisionPointRec(mp, hardButton)) {
            currentDifficulty = HARD;
            currentWeather = SNOW;
            currentState = PLAYING;
            ResetGame();
        }
    }
    
    // Highlight button on hover for better UX
    if (CheckCollisionPointRec(mp, easyButton)) {
        DrawRectangleLinesEx(easyButton, 3, YELLOW);
    } else if (CheckCollisionPointRec(mp, mediumButton)) {
        DrawRectangleLinesEx(mediumButton, 3, YELLOW);
    } else if (CheckCollisionPointRec(mp, hardButton)) {
        DrawRectangleLinesEx(hardButton, 3, YELLOW);
    }
}

void DrawGameElements() {
    DrawTowers();
    DrawEnemies();
    DrawProjectiles();
    DrawVisualEffects();
    for (const auto& beam : laserBeams) {
        if (beam.active) DrawLineEx(beam.start, beam.end, beam.thickness, beam.color);
    }
}

void DrawSelectedTowerInfo() {
    if (selectedTowerIndex >= 0 && selectedTowerIndex < towers.size()) {
        Tower& tower = towers[selectedTowerIndex];
        DrawText(GetTowerName(tower.type), selectedTowerInfoX, selectedTowerInfoY, 20, BLACK);
        DrawText(TextFormat("Damage: %d", tower.damage), selectedTowerInfoX, selectedTowerInfoY + infoSpacing, 18, BLACK);
        DrawText(TextFormat("Range: %.0f", tower.range), selectedTowerInfoX, selectedTowerInfoY + infoSpacing * 2, 18, BLACK);
        DrawText(TextFormat("Fire Rate: %.1f", tower.fireRate), selectedTowerInfoX, selectedTowerInfoY + infoSpacing * 3, 18, BLACK);
        DrawText(TextFormat("Level: %d", tower.upgradeLevel + 1), selectedTowerInfoX, selectedTowerInfoY + infoSpacing * 4, 18, BLACK);
        HandleTowerUpgrade();
        HandleTowerAbilityButton();
        if (tower.isMalfunctioning) {
            Rectangle repairButton = { (float)selectedTowerInfoX, (float)(selectedTowerInfoY + infoSpacing * 10), 150, 40 };
            DrawRectangleRec(repairButton, ORANGE);
            DrawRectangleLinesEx(repairButton, 2.0f, BLACK);
            DrawText("Repair ($50)", repairButton.x + 10, repairButton.y + 10, 18, BLACK);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), repairButton)) {
                RepairTower(tower);
            }
        }
    }
}

void DrawWaveProgressBar() {
    if (!waveInProgress || currentWaveIndex >= waves.size()) return;
    const EnemyWave& currentWave = waves[currentWaveIndex];
    int totalEnemies = currentWave.basicCount + currentWave.fastCount + currentWave.armouredCount + currentWave.fastArmouredCount;
    if (totalEnemies <= 0) return;
    float spawnProgress = (float)spawnedEnemies / totalEnemies;
    float defeatProgress = (float)defeatedEnemies / totalEnemies;
    DrawRectangle(progressBarX - 5, progressBarY - 25, progressBarWidth + 10, progressBarHeight + 30, ColorAlpha(LIGHTGRAY, 0.7f));
    DrawText(TextFormat("Wave %d Progress", currentWaveIndex + 1), progressBarX, progressBarY - 20, 15, BLACK);
    DrawRectangle(progressBarX, progressBarY, progressBarWidth, progressBarHeight, DARKGRAY);
    DrawRectangle(progressBarX, progressBarY, (int)(progressBarWidth * spawnProgress), progressBarHeight, BLUE);
    DrawRectangle(progressBarX, progressBarY, (int)(progressBarWidth * defeatProgress), progressBarHeight, GREEN);
    DrawRectangleLinesEx((Rectangle){(float)progressBarX, (float)progressBarY, (float)progressBarWidth, (float)progressBarHeight}, 2, BLACK);
    DrawText(TextFormat("Spawned: %d/%d", spawnedEnemies, totalEnemies), progressBarX, progressBarY + progressBarHeight + 5, 15, BLUE);
    DrawText(TextFormat("Defeated: %d/%d", defeatedEnemies, totalEnemies), progressBarX + 120, progressBarY + progressBarHeight + 5, 15, GREEN);
}

void DrawPauseButton() {
    DrawRectangleRec(pauseButton, DARKGRAY);
    DrawRectangleLinesEx(pauseButton, 2.0f, WHITE);
    if (currentState != PAUSED) {
        DrawRectangle(pauseButton.x + 8, pauseButton.y + 7, 5, 16, WHITE);
        DrawRectangle(pauseButton.x + 18, pauseButton.y + 7, 5, 16, WHITE);
    } else {
        Vector2 points[3] = { {pauseButton.x + 8, pauseButton.y + 7}, {pauseButton.x + 8, pauseButton.y + 23}, {pauseButton.x + 23, pauseButton.y + 15} };
        DrawTriangle(points[0], points[1], points[2], WHITE);
    }
}

void DrawSkipWaveButton() {
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
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), pauseButton)) {
        if (currentState == PLAYING) currentState = PAUSED;
        else if (currentState == PAUSED) currentState = PLAYING;
    }
}

void HandleSkipWaveButton() {
    if (showSkipButton && IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), skipWaveButton)) {
        waveDelay = 0.0f;
    }
}

void DrawPauseScreen() {
    DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(BLACK, 0.7f));
    const char* pauseText = "GAME PAUSED";
    int fontSize = 40;
    int textWidth = MeasureText(pauseText, fontSize);
    DrawText(pauseText, screenWidth / 2 - textWidth / 2, screenHeight / 2 - 40, fontSize, WHITE);
    const char* instructionText = "Click the pause button to resume";
    int instructionWidth = MeasureText(instructionText, 20);
    DrawText(instructionText, screenWidth / 2 - instructionWidth / 2, screenHeight / 2 + 20, 20, LIGHTGRAY);
}

void DrawVisualEffects() {
    for (const auto& effect : visualEffects) {
        if (!effect.active) continue;
        float alpha = effect.timer / effect.lifespan;
        float scale = 1.0f + (1.0f - alpha) * 0.5f;
        Color fadingColor = ColorAlpha(effect.color, alpha);
        DrawCircleV(effect.position, effect.radius * scale, fadingColor);
    }
}

void UpdateWeatherParticles() {
    float dt = GetFrameTime();
    if (currentDifficulty == MEDIUM) {
        if (GetRandomValue(0, 100) < 40) {
            WeatherParticle p;
            p.position = { (float)GetRandomValue(-50, screenWidth + 50), (float)GetRandomValue(-60, -5) };
            p.velocity = { (float)GetRandomValue(-30, -10), (float)GetRandomValue(280, 350) };
            p.lifetime = (float)GetRandomValue(15, 25) / 10.0f;
            p.maxLifetime = p.lifetime;
            p.size = (float)GetRandomValue(10, 25) / 10.0f;
            p.alpha = (float)GetRandomValue(75, 90) / 100.0f;
            p.wobble = 0.0f;
            p.wobbleSpeed = 0.0f;
            int heightSelector = GetRandomValue(1, 100);
            if (heightSelector <= 20) p.targetHeight = (float)GetRandomValue(50, screenHeight / 3);
            else if (heightSelector <= 50) p.targetHeight = (float)GetRandomValue(screenHeight / 3, 2 * screenHeight / 3);
            else p.targetHeight = (float)GetRandomValue(2 * screenHeight / 3, screenHeight - 10);
            p.isSplash = false;
            weatherParticles.push_back(p);
        }
    } else if (currentDifficulty == HARD) {
        if (GetRandomValue(0, 100) < 25) {
            WeatherParticle p;
            p.position = { (float)GetRandomValue(-30, screenWidth + 30), (float)GetRandomValue(-50, -5) };
            p.velocity = { (float)GetRandomValue(-15, 15) / 10.0f, (float)GetRandomValue(40, 80) };
            p.maxLifetime = (float)GetRandomValue(40, 80) / 10.0f;
            p.lifetime = p.maxLifetime;
            p.size = (float)GetRandomValue(15, 30) / 10.0f;
            p.alpha = (float)GetRandomValue(70, 95) / 100.0f;
            p.wobble = (float)GetRandomValue(0, 628) / 100.0f;
            p.wobbleSpeed = (float)GetRandomValue(5, 20) / 10.0f;
            weatherParticles.push_back(p);
        }
    }
    for (auto& p : weatherParticles) {
        if (currentDifficulty == MEDIUM) {
            if (!p.isSplash) {
                p.velocity.y += 10.0f * dt;
                p.position = Vector2Add(p.position, Vector2Scale(p.velocity, dt));
                if (p.position.y >= p.targetHeight) {
                    p.lifetime = 0;
                    int splashChance = (p.position.x > 0 && p.position.x < screenWidth) ? 9 : 7;
                    if (GetRandomValue(0, 10) < splashChance) {
                        WeatherParticle splash;
                        splash.position = { p.position.x, p.position.y };
                        splash.velocity = { 0, 0 };
                        splash.lifetime = 0.3f;
                        splash.maxLifetime = splash.lifetime;
                        splash.size = p.size * 1.2f;
                        splash.alpha = 0.8f;
                        splash.wobble = 0.0f;
                        splash.wobbleSpeed = 0.0f;
                        splash.isSplash = true;
                        splash.targetHeight = p.targetHeight;
                        weatherParticles.push_back(splash);
                    }
                }
            } else {
                p.lifetime -= dt;
            }
        } else if (currentDifficulty == HARD) {
            p.wobble += p.wobbleSpeed * dt;
            if (p.wobble > 2 * PI) p.wobble -= 2 * PI;
            float wobbleOffset = sinf(p.wobble) * 0.7f;
            Vector2 adjustedVelocity = p.velocity;
            adjustedVelocity.x += wobbleOffset;
            p.position = Vector2Add(p.position, Vector2Scale(adjustedVelocity, dt));
            float groundProximity = p.position.y / screenHeight;
            if (groundProximity > 0.85f) {
                float groundFactor = (1.0f - (groundProximity - 0.85f) / 0.15f);
                p.alpha *= groundFactor;
                p.size *= groundFactor;
                p.velocity.y *= 0.98f;
            }
        }
        if (!p.isSplash) p.lifetime -= dt;
    }
    weatherParticles.erase(remove_if(weatherParticles.begin(), weatherParticles.end(),
        [](const WeatherParticle& p) {
            return p.lifetime <= 0 || p.position.y > screenHeight || p.alpha < 0.05f ||
                   p.position.x < -50 || p.position.x > screenWidth + 50;
        }), weatherParticles.end());
}

void DrawWeatherParticles() {
    for (const auto& p : weatherParticles) {
        if (currentDifficulty == MEDIUM) {
            if (!p.isSplash) {
                Vector2 endPoint = { p.position.x + p.velocity.x * 0.03f, p.position.y + p.velocity.y * 0.03f };
                float length = Vector2Length(p.velocity) * 0.05f;
                if (length < 5.0f) length = 5.0f;
                if (length > 15.0f) length = 15.0f;
                Color rainColor = ColorAlpha(SKYBLUE, p.alpha);
                DrawLineEx(p.position, endPoint, p.size, rainColor);
                Color highlightColor = ColorAlpha(WHITE, p.alpha * 0.5f);
                Vector2 highlightPos = { p.position.x + 0.5f, p.position.y + 0.5f };
                Vector2 highlightEnd = { endPoint.x + 0.5f, endPoint.y + 0.5f };
                DrawLineEx(highlightPos, highlightEnd, p.size * 0.4f, highlightColor);
            } else {
                float splashProgress = p.lifetime / p.maxLifetime;
                float expansionFactor = 1.0f + (1.0f - splashProgress) * 3.0f;
                float splashSize = p.size * expansionFactor;
                Color splashColor = ColorAlpha(SKYBLUE, p.alpha * splashProgress * 0.8f);
                DrawCircleV(p.position, splashSize, splashColor);
                if (p.size > 1.0f && splashProgress < 0.8f) {
                    float outerRingSize = splashSize * 1.5f;
                    float innerRingAlpha = splashProgress * 0.4f;
                    DrawCircleLines(p.position.x, p.position.y, outerRingSize, ColorAlpha(SKYBLUE, innerRingAlpha));
                    if (p.size > 1.8f && splashProgress < 0.6f) {
                        DrawCircleLines(p.position.x, p.position.y, outerRingSize * 0.7f, ColorAlpha(SKYBLUE, innerRingAlpha * 1.3f));
                    }
                }
            }
        } else if (currentDifficulty == HARD) {
            Color snowColor = ColorAlpha(WHITE, p.alpha);
            DrawCircleV(p.position, p.size, snowColor);
            float innerSize = p.size * 0.6f;
            float innerBrightness = p.alpha * 1.3f;
            if (innerBrightness > 1.0f) innerBrightness = 1.0f;
            DrawCircleV(p.position, innerSize, ColorAlpha(WHITE, innerBrightness));
            if (p.size > 2.0f && p.alpha > 0.5f) {
                float spokeLength = p.size * 1.2f;
                float spokeAlpha = p.alpha * 0.4f;
                for (int i = 0; i < 4; i++) {
                    float angle = p.wobble + (PI / 4 * i);
                    Vector2 spokeEnd = { p.position.x + cosf(angle) * spokeLength, p.position.y + sinf(angle) * spokeLength };
                    DrawLineEx(p.position, spokeEnd, 0.5f, ColorAlpha(WHITE, spokeAlpha));
                }
            }
        }
    }
}

void UpdateTowerMalfunctions() {
    if (currentDifficulty != HARD) return;
    float now = GetTime();
    for (auto& tower : towers) {
        if (tower.type == NONE) continue;
        if (!tower.isMalfunctioning && (now - tower.lastFiredTime) >= 30.0f) {
            tower.isMalfunctioning = true;
            tower.color = GRAY;
        }
    }
}

void DrawRainyAtmosphereOverlay() {
    if (currentDifficulty == MEDIUM) {
        DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(DARKBLUE, 0.07f));
        for (int i = 0; i < 5; i++) {
            float yPos = i * 20.0f - 50.0f;
            float alpha = 0.03f - (i * 0.005f);
            if (alpha > 0) DrawRectangle(0, yPos, screenWidth, 100, ColorAlpha(DARKBLUE, alpha));
        }
    }
}

Texture2D CreateFallbackTexture(Color color) {
    Image img = GenImageColor(64, 64, color);
    Texture2D texture = LoadTextureFromImage(img);
    UnloadImage(img);
    return texture;
}

int main() {
    InitWindow(screenWidth, screenHeight, "Robust Tower Defense - v0.2");
    SetTargetFPS(60);

    Texture2D mediummaptopTexture;
    Texture2D mediummapgridTexture;
    Texture2D hardmapgridTexture;
    Texture2D hardmaprightmostTexture;

    tier1TowerTexture = LoadTexture("tier1tower.png");
    if (tier1TowerTexture.id == 0) tier1TowerTexture = CreateFallbackTexture(BLUE);
    tier2TowerTexture = LoadTexture("tier2tower.png");
    if (tier2TowerTexture.id == 0) tier2TowerTexture = CreateFallbackTexture(GREEN);
    tier3TowerTexture = LoadTexture("tier3tower.png");
    if (tier3TowerTexture.id == 0) tier3TowerTexture = CreateFallbackTexture(RED);
    placeholderTexture = LoadTexture("placeholder.png");
    if (placeholderTexture.id == 0) placeholderTexture = CreateFallbackTexture(WHITE);
    tier1ProjectileTexture = LoadTexture("tier1projectile.png");
    if (tier1ProjectileTexture.id == 0) tier1ProjectileTexture = CreateFallbackTexture(SKYBLUE);
    tier2ProjectileTexture = LoadTexture("tier2projectile.png");
    if (tier2ProjectileTexture.id == 0) tier2ProjectileTexture = CreateFallbackTexture(LIME);
    tier3ProjectileTexture = LoadTexture("tier3projectile.png");
    if (tier3ProjectileTexture.id == 0) tier3ProjectileTexture = CreateFallbackTexture(ORANGE);
    tier1EnemyTexture = LoadTexture("tier1enemy.png");
    if (tier1EnemyTexture.id == 0) tier1EnemyTexture = CreateFallbackTexture(RED);
    tier2EnemyTexture = LoadTexture("tier2enemy.png");
    if (tier2EnemyTexture.id == 0) tier2EnemyTexture = CreateFallbackTexture(YELLOW);
    backgroundTexture = LoadTexture("towerdefensegrass.png");
    if (backgroundTexture.id == 0) backgroundTexture = CreateFallbackTexture(DARKGREEN);
    tier3EnemyTexture = LoadTexture("tier3enemy.png");
    if (tier3EnemyTexture.id == 0) tier3EnemyTexture = CreateFallbackTexture(DARKGRAY);
    tier4EnemyTexture = LoadTexture("tier4enemy.png");
    if (tier4EnemyTexture.id == 0) tier4EnemyTexture = CreateFallbackTexture(GOLD);
    bottomGridTexture = LoadTexture("bottomsidegrid.png");
    if (bottomGridTexture.id == 0) bottomGridTexture = CreateFallbackTexture(BROWN);
    leftGridTexture = LoadTexture("leftsidegrid.png");
    if (leftGridTexture.id == 0) leftGridTexture = CreateFallbackTexture(DARKBLUE);
    topGridTexture = LoadTexture("topsidegrid.png");
    if (topGridTexture.id == 0) topGridTexture = CreateFallbackTexture(PURPLE);
    secondRightmostTexture = LoadTexture("secondrightmost.png");
    if (secondRightmostTexture.id == 0) secondRightmostTexture = CreateFallbackTexture(GRAY);
    rightGridTexture = LoadTexture("rightsidegrid.png");
    if (rightGridTexture.id == 0) rightGridTexture = CreateFallbackTexture(MAROON);
    mediummaptopTexture = LoadTexture("mediummaptop.png");
    if (mediummaptopTexture.id == 0) mediummaptopTexture = CreateFallbackTexture(PURPLE);
    mediummapgridTexture = LoadTexture("mediummapgrid.png");
    if (mediummapgridTexture.id == 0) mediummapgridTexture = CreateFallbackTexture(DARKGREEN);
    hardmapgridTexture = LoadTexture("hardmapgrid.png");
    if (hardmapgridTexture.id == 0) hardmapgridTexture = CreateFallbackTexture(DARKGRAY);
    hardmaprightmostTexture = LoadTexture("hardmaprightmost.png");
    if (hardmaprightmostTexture.id == 0) hardmaprightmostTexture = CreateFallbackTexture(MAROON);

    InitGrid();
    InitWaypoints();

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_P)) {
            currentState = PLAYING;
            ResetGame();
        }
        if (currentState == PLAYING || currentState == PAUSED) {
            HandlePauseButton();
            if (currentState == PLAYING) {
                HandleSkipWaveButton();
                HandleTowerMenuClick();
                HandleTowerSelection();
                HandleTowerPlacement();
                UpdateGameElements();
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
                        if (spawnedEnemies < waves[currentWaveIndex].basicCount) type = BASIC_ENEMY;
                        else if (spawnedEnemies < waves[currentWaveIndex].basicCount + waves[currentWaveIndex].fastCount) type = FAST_ENEMY;
                        else if (spawnedEnemies < waves[currentWaveIndex].basicCount + waves[currentWaveIndex].fastCount + waves[currentWaveIndex].armouredCount) type = ARMOURED_ENEMY;
                        else type = FAST_ARMOURED_ENEMY;
                        // Use center-left spawn point (i.e. column 0, row = gridRows/2)
                        Vector2 spawnPoint = { tileWidth / 2.0f, (float)(gridRows * tileHeight) / 2.0f };
                        enemies.push_back(CreateEnemy(type, spawnPoint));
                        waveTimer = waves[currentWaveIndex].spawnInterval;
                        spawnedEnemies++;
                        Enemy &e = enemies.back();
                        if (currentDifficulty == MEDIUM) {
                            e.maxHp = (int)(e.maxHp * 1.2f);
                            e.hp = e.maxHp;
                        } else if (currentDifficulty == HARD) {
                            e.maxHp = (int)(e.maxHp * 1.4f);
                            e.hp = e.maxHp;
                        }
                    }
                    if (spawnedEnemies >= totalEnemies && enemies.empty()) {
                        waveInProgress = false;
                        currentWaveIndex++;
                        if (currentWaveIndex >= waves.size()) currentState = WIN;
                        else waveDelay = 15.0f;
                    }
                }
                for (auto& tower : towers) {
                    tower.rotationAngle += tower.rotationSpeed * GetFrameTime();
                    if (tower.rotationAngle > 360.0f) tower.rotationAngle -= 360.0f;
                }
                UpdateWeatherParticles();
                if (currentDifficulty == HARD) UpdateTowerMalfunctions();
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);

        if (currentState == MENU) {
            DrawMenuScreen();
        } else if (currentState == PLAYING || currentState == PAUSED) {
            for (int row = 0; row < gridRows; row++) {
                for (int col = 0; col < gridColumns; col++) {
                    // Select sourceRec dimensions based on chosen difficulty texture
                    Rectangle sourceRec = { 
                        0.0f, 0.0f, 
                        (float)(currentDifficulty == EASY ? backgroundTexture.width : 
                        (currentDifficulty == MEDIUM ? mediummapgridTexture.width : hardmapgridTexture.width)), 
                        (float)(currentDifficulty == EASY ? backgroundTexture.height : 
                        (currentDifficulty == MEDIUM ? mediummapgridTexture.height : hardmapgridTexture.height))
                    };
                    Rectangle destRec = { (float)(col * tileWidth), (float)(row * tileHeight), (float)tileWidth, (float)tileHeight };
                    Vector2 origin = { 0.0f, 0.0f };
                    Texture2D* textureToUse = nullptr;
                    if (currentDifficulty == EASY) {
                        textureToUse = &backgroundTexture;
                        if (col == 0) textureToUse = &leftGridTexture;
                        else if (col == gridColumns - 1) textureToUse = &rightGridTexture;
                        else if (col == gridColumns - 2) textureToUse = &secondRightmostTexture;
                        else if (row == 0) textureToUse = &topGridTexture;
                        else if (row == gridRows - 1) textureToUse = &bottomGridTexture;
                    } else if (currentDifficulty == MEDIUM) {
                        textureToUse = &mediummapgridTexture;
                        if (col == 0) textureToUse = &mediummaptopTexture;
                    } else if (currentDifficulty == HARD) {
                        textureToUse = &hardmapgridTexture;
                        if (col == gridColumns - 1) textureToUse = &hardmaprightmostTexture;
                    }
                    DrawTexturePro(*textureToUse, sourceRec, destRec, origin, 0.0f, WHITE);
                }
            }
            for (size_t i = 0; i < waypoints.size() - 1; ++i) {
                DrawLineV(waypoints[i], waypoints[i + 1], ColorAlpha(LIGHTGRAY, 0.5f));
            }
            DrawGridHighlight();
            if (selectedTowerType != NONE) {
                Vector2 mousePos = GetMousePosition();
                int gridCol = mousePos.x / tileWidth;
                int gridRow = mousePos.y / tileHeight;
                if (gridCol >= 0 && gridCol < gridColumns && gridRow >= 0 && gridRow < gridRows && grid[gridRow][gridCol]) {
                    Tower ghostTower = CreateTower(NONE, { (float)(gridCol * tileWidth + tileWidth / 2), (float)(gridRow * tileHeight + tileHeight / 2) });
                    DrawCircleV(ghostTower.position, tileWidth / 2.5f, ghostTower.color);
                }
            }
            DrawGameElements();
            DrawRainyAtmosphereOverlay();
            if (currentDifficulty == MEDIUM || currentDifficulty == HARD) DrawWeatherParticles();
            DrawText("Tower Defense", titleX - MeasureText("Tower Defense", 20) / 2, titleY, 20, MAROON);
            DrawText(TextFormat("Money: %d", playerMoney), moneyX, moneyY, regularTextFontSize, textColor);
            DrawText(TextFormat("Escaped: %d/%d", enemiesReachedEnd, maxEnemiesReachedEnd), escapedX, escapedY, regularTextFontSize, RED);
            if (waveInProgress) {
                int totalEnemies = waves[currentWaveIndex].basicCount + waves[currentWaveIndex].fastCount + waves[currentWaveIndex].armouredCount + waves[currentWaveIndex].fastArmouredCount;
                int enemiesRemaining = totalEnemies - spawnedEnemies + (int)enemies.size();
                DrawText(TextFormat("Wave %d - Enemies Remaining: %d", currentWaveIndex + 1, enemiesRemaining), waveInfoX, waveInfoY, regularTextFontSize, textColor);
            } else if (currentWaveIndex < waves.size()) {
                string nextWaveText = "Next Wave in " + to_string((int)waveDelay + 1);
                DrawText(nextWaveText.c_str(), screenWidth / 2 - MeasureText(nextWaveText.c_str(), largeTextFontSize) / 2, nextWaveTimerY, largeTextFontSize, BLUE);
            } else {
                DrawText("All Waves Completed!", waveInfoX, waveInfoY, regularTextFontSize, GREEN);
            }
            int currentMenuX = towerMenuStartX;
            Rectangle tier1Rec = { (float)currentMenuX, (float)towerMenuStartY, (float)towerSelectionWidth, (float)towerSelectionHeight };
            DrawRectangleRec(tier1Rec, BLUE);
            DrawRectangleLinesEx(tier1Rec, 2.0f, selectedTowerType == TIER1_DEFAULT ? GOLD : DARKGRAY);
            DrawText(GetTowerName(TIER1_DEFAULT), currentMenuX + 10, towerMenuStartY + 10, towerTypeTextSize, WHITE);
            DrawText(TextFormat("$%d", GetTowerCost(TIER1_DEFAULT)), currentMenuX + 10, towerMenuStartY + towerSelectionHeight - 25, regularTextFontSize, WHITE);
            currentMenuX += towerMenuSpacingX;
            Rectangle tier2Rec = { (float)currentMenuX, (float)towerMenuStartY, (float)towerSelectionWidth, (float)towerSelectionHeight };
            DrawRectangleRec(tier2Rec, GREEN);
            DrawRectangleLinesEx(tier2Rec, 2.0f, selectedTowerType == TIER2_FAST ? GOLD : DARKGRAY);
            DrawText(GetTowerName(TIER2_FAST), currentMenuX + 10, towerMenuStartY + 10, towerTypeTextSize, WHITE);
            DrawText(TextFormat("$%d", GetTowerCost(TIER2_FAST)), currentMenuX + 10, towerMenuStartY + towerSelectionHeight - 25, regularTextFontSize, WHITE);
            currentMenuX += towerMenuSpacingX;
            Rectangle tier3Rec = { (float)currentMenuX, (float)towerMenuStartY, (float)towerSelectionWidth, (float)towerSelectionHeight };
            DrawRectangleRec(tier3Rec, RED);
            DrawRectangleLinesEx(tier3Rec, 2.0f, selectedTowerType == TIER3_STRONG ? GOLD : DARKGRAY);
            DrawText(GetTowerName(TIER3_STRONG), currentMenuX + 10, towerMenuStartY + 10, towerTypeTextSize, WHITE);
            DrawText(TextFormat("$%d", GetTowerCost(TIER3_STRONG)), currentMenuX + 10, towerMenuStartY + towerSelectionHeight - 25, regularTextFontSize, WHITE);
            if (selectedTowerType != NONE) {
                DrawText(TextFormat("Selected: %s", GetTowerName(selectedTowerType)), uiPadding, selectedTowerTextY, regularTextFontSize, GOLD);
            }
            DrawSelectedTowerInfo();
            DrawWaveProgressBar();
            DrawTowerTooltip(TIER1_DEFAULT, GetMousePosition()); // Simplified; actual logic in tower.cpp
            DrawPauseButton();
            DrawSkipWaveButton();
            if (currentState == PAUSED) DrawPauseScreen();
        } else if (currentState == GAME_OVER || currentState == WIN) {
            for (int y = 0; y < screenHeight; y += backgroundTexture.height) {
                for (int x = 0; x < screenWidth; x += backgroundTexture.width) {
                    Rectangle sourceRec = { 0.0f, 0.0f, (float)backgroundTexture.width, (float)backgroundTexture.height };
                    Rectangle destRec = { (float)x, (float)y, (float)backgroundTexture.width, (float)backgroundTexture.height };
                    DrawTexturePro(backgroundTexture, sourceRec, destRec, { 0.0f, 0.0f }, 0.0f, WHITE);
                }
            }
            DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(DARKGRAY, 0.5f));
            if (currentState == GAME_OVER) {
                DrawText("Game Over", screenWidth / 2 - 100, screenHeight / 2 - 50, 40, RED);
                DrawText(TextFormat("Enemies Escaped: %d/%d", enemiesReachedEnd, maxEnemiesReachedEnd), screenWidth / 2 - 150, screenHeight / 2 - 10, 20, RED);
            } else {
                DrawText("You Win!", screenWidth / 2 - 100, screenHeight / 2 - 50, 40, GREEN);
            }
            Rectangle restartButton = { 
                screenWidth / 2.0f - 50.0f, 
                screenHeight / 2.0f + (currentState == GAME_OVER ? 30.0f : 10.0f), 
                100.0f, 
                40.0f 
            };
            DrawRectangleRec(restartButton, LIGHTGRAY);
            DrawText("Restart", screenWidth / 2 - 30, restartButton.y + 10, 20, textColor);
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), restartButton)) {
                ResetGame();
                currentState = MENU;
            }
        }
        EndDrawing();
    }

    UnloadTexture(tier1TowerTexture);
    UnloadTexture(tier2TowerTexture);
    UnloadTexture(tier3TowerTexture);
    UnloadTexture(placeholderTexture);
    UnloadTexture(tier1ProjectileTexture);
    UnloadTexture(tier2ProjectileTexture);
    UnloadTexture(tier3ProjectileTexture);
    UnloadTexture(tier1EnemyTexture);
    UnloadTexture(tier2EnemyTexture);
    UnloadTexture(backgroundTexture);
    UnloadTexture(tier3EnemyTexture);
    UnloadTexture(tier4EnemyTexture);
    UnloadTexture(bottomGridTexture);
    UnloadTexture(leftGridTexture);
    UnloadTexture(topGridTexture);
    UnloadTexture(secondRightmostTexture);
    UnloadTexture(rightGridTexture);
    UnloadTexture(mediummaptopTexture);
    UnloadTexture(mediummapgridTexture);
    UnloadTexture(hardmapgridTexture);
    UnloadTexture(hardmaprightmostTexture);
    CloseWindow();
    return 0;
}