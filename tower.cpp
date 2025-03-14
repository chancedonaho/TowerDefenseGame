#include "game.h"

Tower CreateTower(TowerType type, Vector2 position) {
    Tower newTower;
    newTower.position = position;
    newTower.type = type;
    newTower.fireCooldown = 0.0f;
    newTower.rotationAngle = 0.0f;
    newTower.rotationSpeed = 0.0f;
    newTower.texture = { 0 };
    newTower.projectileTexture = { 0 };
    newTower.upgradeLevel = 0;
    newTower.abilityCooldownTimer = 0.0f;
    newTower.abilityActive = false;
    newTower.abilityTimer = 0.0f;
    newTower.isPowerShotActive = false;
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
            newTower.abilityCooldownDuration = 15.0f;
            newTower.abilityDuration = 3.0f;
            newTower.originalFireRate = newTower.fireRate;
            break;
        case TIER2_FAST:
            newTower.color = GREEN;
            newTower.range = 120.0f;
            newTower.fireRate = 1.5f;
            newTower.damage = 20;
            newTower.texture = tier2TowerTexture;
            newTower.projectileTexture = tier2ProjectileTexture;
            newTower.rotationSpeed = 25.0f;
            newTower.abilityCooldownDuration = 10.0f;
            newTower.abilityDuration = 5.0f;
            newTower.originalFireRate = newTower.fireRate;
            break;
        case TIER3_STRONG:
            newTower.color = RED;
            newTower.range = 200.0f;
            newTower.fireRate = 1.2f;
            newTower.damage = 40;
            newTower.texture = tier3TowerTexture;
            newTower.projectileTexture = tier3ProjectileTexture;
            newTower.rotationSpeed = 5.0f;
            newTower.abilityCooldownDuration = 8.0f;
            newTower.abilityDuration = 0.0f;
            newTower.originalFireRate = newTower.fireRate;
            break;
        case NONE:
            newTower.color = ColorAlpha(WHITE, 0.5f);
            newTower.range = 0;
            newTower.fireRate = 0;
            newTower.damage = 0;
            break;
        default:
            newTower.color = GRAY;
            newTower.range = 100.0f;
            newTower.fireRate = 1.0f;
            newTower.damage = 10;
            break;
    }
    return newTower;
}

int GetTowerCost(TowerType type) {
    switch (type) {
        case TIER1_DEFAULT: return 20;
        case TIER2_FAST: return 30;
        case TIER3_STRONG: return 50;
        default: return 0;
    }
}

const char* GetTowerName(TowerType type) {
    switch (type) {
        case TIER1_DEFAULT: return "Tier 1";
        case TIER2_FAST: return "Tier 2";
        case TIER3_STRONG: return "Tier 3";
        default: return "Unknown";
    }
}

int GetTowerUpgradeCost(TowerType type, int currentLevel) {
    if (currentLevel >= 2) return 0;
    switch (type) {
        case TIER1_DEFAULT: return currentLevel == 0 ? 30 : 60;
        case TIER2_FAST: return currentLevel == 0 ? 40 : 80;
        case TIER3_STRONG: return currentLevel == 0 ? 60 : 120;
        default: return 0;
    }
}

void ApplyTowerUpgrade(Tower& tower) {
    float damageMultiplier = 1.0f, rangeMultiplier = 1.0f, fireRateMultiplier = 1.0f;
    if (tower.upgradeLevel == 1) {
        damageMultiplier = 1.5f;
        rangeMultiplier = 1.2f;
        fireRateMultiplier = 1.2f;
    } else if (tower.upgradeLevel == 2) {
        damageMultiplier = 2.5f;
        rangeMultiplier = 1.5f;
        fireRateMultiplier = 1.5f;
    }
    switch (tower.type) {
        case TIER1_DEFAULT:
            tower.damage = (int)(25 * damageMultiplier);
            tower.range = 150.0f * rangeMultiplier;
            tower.fireRate = 1.0f * fireRateMultiplier;
            break;
        case TIER2_FAST:
            tower.damage = (int)(20 * damageMultiplier);
            tower.range = 120.0f * rangeMultiplier;
            tower.fireRate = 1.5f * (fireRateMultiplier + 0.1f * tower.upgradeLevel);
            break;
        case TIER3_STRONG:
            tower.damage = (int)(40 * (damageMultiplier + 0.2f * tower.upgradeLevel));
            tower.range = 200.0f * (rangeMultiplier + 0.1f * tower.upgradeLevel);
            tower.fireRate = 1.2f * fireRateMultiplier;
            break;
        default: break;
    }
}

void HandleTowerPlacement() {
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && selectedTowerType != NONE) {
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
                grid[gridRow][gridCol] = false; // Mark grid cell as occupied
                selectedTowerType = NONE;
                
                // Immediately force path recalculation for all enemies
                Vector2Int endGrid = GetGridCoords(waypoints[waypoints.size()-1]);
                for (auto& enemy : enemies) {
                    if (enemy.active) {
                        Vector2Int startGrid = GetGridCoords(enemy.position);
                        vector<Vector2Int> newPath = FindPathBFS(startGrid, endGrid);
                        if (!newPath.empty()) {
                            enemy.waypointsPath = newPath;
                            enemy.pathIndex = 0;
                            enemy.pathCheckTimer = 0.0f; // Reset the timer
                        }
                    }
                }
            } else {
                selectedTowerType = NONE;
            }
        } else {
            selectedTowerType = NONE;
        }
    }
}

bool IsMouseOverTowerUI() {
    if (selectedTowerIndex >= 0 && selectedTowerIndex < towers.size()) {
        Vector2 mousePos = GetMousePosition();
        Rectangle upgradeButton = { (float)selectedTowerInfoX, (float)(selectedTowerInfoY + infoSpacing * 6), (float)upgradeButtonWidth, (float)upgradeButtonHeight };
        Rectangle abilityButton = { (float)selectedTowerInfoX, (float)(selectedTowerInfoY + infoSpacing * 8), (float)abilityButtonWidth, (float)abilityButtonHeight };
        Rectangle infoPanel = { (float)selectedTowerInfoX - 10, (float)selectedTowerInfoY - 10, 200, 250 };
        return CheckCollisionPointRec(mousePos, upgradeButton) || CheckCollisionPointRec(mousePos, abilityButton) || CheckCollisionPointRec(mousePos, infoPanel);
    }
    return false;
}

void HandleTowerSelection() {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (IsMouseOverTowerUI()) return;
        Vector2 mousePos = GetMousePosition();
        selectedTowerIndex = -1;
        for (int i = 0; i < towers.size(); i++) {
            float distance = Vector2Distance(mousePos, towers[i].position);
            if (distance <= tileWidth / 2.0f) {
                selectedTowerIndex = i;
                break;
            }
        }
    }
}

void HandleTowerUpgrade() {
    if (selectedTowerIndex >= 0 && selectedTowerIndex < towers.size()) {
        Tower& selectedTower = towers[selectedTowerIndex];
        int upgradeCost = GetTowerUpgradeCost(selectedTower.type, selectedTower.upgradeLevel);
        Rectangle upgradeButton = { (float)selectedTowerInfoX, (float)(selectedTowerInfoY + infoSpacing * 6), (float)upgradeButtonWidth, (float)upgradeButtonHeight };
        bool canUpgrade = (selectedTower.upgradeLevel < 2) && (playerMoney >= upgradeCost);
        Color buttonColor = canUpgrade ? GREEN : GRAY;
        DrawRectangleRec(upgradeButton, buttonColor);
        DrawRectangleLinesEx(upgradeButton, 2.0f, BLACK);
        DrawText(TextFormat("Upgrade: $%d", upgradeCost), selectedTowerInfoX + 10, selectedTowerInfoY + infoSpacing * 6 + 10, 20, BLACK);
        if (canUpgrade && IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), upgradeButton)) {
            playerMoney -= upgradeCost;
            selectedTower.upgradeLevel++;
            ApplyTowerUpgrade(selectedTower);
        }
    }
}

void DrawTowers() {
    Vector2 mousePos = GetMousePosition();
    for (int i = 0; i < towers.size(); i++) {
        const auto& tower = towers[i];
        float distanceToMouse = Vector2Distance(mousePos, tower.position);
        bool isHovered = distanceToMouse <= tileWidth / 2.0f;
        bool isSelected = (i == selectedTowerIndex);
        if (isSelected || isHovered) {
            Color rangeColor = isSelected ? GOLD : tower.color;
            float rangeAlpha = isSelected ? 0.2f : 0.15f;
            DrawCircleV(tower.position, tower.range, ColorAlpha(rangeColor, rangeAlpha));
        }
        if (isSelected) {
            float pulseScale = 1.0f + 0.1f * sin(GetTime() * 5.0f);
            DrawCircleV(tower.position, tileWidth / 2.0f * pulseScale, ColorAlpha(GOLD, 0.5f));
            DrawCircleLinesV(tower.position, tileWidth / 2.0f * pulseScale, GOLD);
        } else if (isHovered) {
            DrawCircleLinesV(tower.position, tileWidth / 2.0f, ColorAlpha(WHITE, 0.8f));
        }
        if (tower.texture.id > 0) {
            Rectangle sourceRec = { 0.0f, 0.0f, (float)tower.texture.width, (float)tower.texture.height };
            Rectangle destRec = { tower.position.x, tower.position.y, (float)tileWidth, (float)tileHeight };
            Vector2 origin = { (float)tileWidth / 2.0f, (float)tileHeight / 2.0f };
            DrawTexturePro(tower.texture, sourceRec, destRec, origin, tower.rotationAngle, WHITE);
            if (tower.upgradeLevel > 0) {
                for (int lvl = 0; lvl < tower.upgradeLevel; lvl++) {
                    DrawCircle(tower.position.x - 10 + lvl * 10, tower.position.y - tileHeight / 2 - 5, 3, GOLD);
                }
            }
        } else {
            DrawCircleV(tower.position, tileWidth / 2.5f, tower.color);
            if (tower.upgradeLevel > 0) {
                for (int lvl = 0; lvl < tower.upgradeLevel; lvl++) {
                    DrawCircle(tower.position.x - 10 + lvl * 10, tower.position.y - tileHeight / 2 - 5, 3, GOLD);
                }
            }
        }
    }
}

void HandleTowerFiring() {
    for (auto& tower : towers) {
        if (tower.isMalfunctioning) continue;
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
            if (tower.upgradeLevel == 2) {
                if (tower.type == TIER1_DEFAULT) {
                    int actualDamage = tower.damage;
                    if (target->type == ARMOURED_ENEMY || target->type == FAST_ARMOURED_ENEMY) actualDamage = (int)(actualDamage * 0.7f);
                    target->hp -= actualDamage;
                    if (target->hp <= 0) {
                        target->active = false;
                        playerMoney += 10;
                        defeatedEnemies++;
                    }
                    LaserBeam laser = { tower.position, target->position, 0.1f, 0.1f, true, ColorAlpha(SKYBLUE, 0.8f), 2.0f };
                    laserBeams.push_back(laser);
                    VisualEffect impactEffect = { target->position, 0.2f, 0.2f, ColorAlpha(WHITE, 0.9f), 8.0f, true };
                    visualEffects.push_back(impactEffect);
                    tower.fireCooldown = 0.2f;
                } else if (tower.type == TIER2_FAST) {
                    Projectile newProjectile = { tower.position, target, 150.0f, tower.damage, true, tower.projectileTexture, Projectile::Type::FLAMETHROWER, tower.position, 50.0f };
                    projectiles.push_back(newProjectile);
                    tower.fireCooldown = 1.0f / tower.fireRate;
                } else {
                    int projectileDamage = tower.isPowerShotActive && tower.type == TIER3_STRONG ? tower.damage * 3 : tower.damage;
                    if (tower.isPowerShotActive) tower.isPowerShotActive = false;
                    Projectile newProjectile = { tower.position, target, 200.0f, projectileDamage, true, tower.projectileTexture, Projectile::Type::STANDARD, tower.position, 0.0f };
                    projectiles.push_back(newProjectile);
                    tower.fireCooldown = 1.0f / tower.fireRate;
                }
            } else {
                int projectileDamage = tower.isPowerShotActive && tower.type == TIER3_STRONG ? tower.damage * 3 : tower.damage;
                if (tower.isPowerShotActive) tower.isPowerShotActive = false;
                Projectile newProjectile = { tower.position, target, 200.0f, projectileDamage, true, tower.projectileTexture, Projectile::Type::STANDARD, tower.position, 0.0f };
                projectiles.push_back(newProjectile);
                tower.fireCooldown = 1.0f / tower.fireRate;
            }
            VisualEffect fireEffect = { tower.position, 0.2f, 0.2f, ColorAlpha(tower.type == TIER1_DEFAULT ? SKYBLUE : tower.type == TIER2_FAST ? LIME : RED, 0.8f),
                (tower.isPowerShotActive && tower.type == TIER3_STRONG) ? 25.0f : (tower.type == TIER2_FAST && tower.upgradeLevel == 2) ? 20.0f : (tower.type == TIER1_DEFAULT && tower.upgradeLevel == 2) ? 18.0f : 15.0f, true };
            if (tower.isPowerShotActive && tower.type == TIER3_STRONG) fireEffect.color = ColorAlpha(ORANGE, 0.9f);
            else if (tower.type == TIER2_FAST && tower.upgradeLevel == 2) fireEffect.color = ColorAlpha(ORANGE, 0.8f);
            else if (tower.type == TIER1_DEFAULT && tower.upgradeLevel == 2) fireEffect.color = ColorAlpha(SKYBLUE, 0.9f);
            visualEffects.push_back(fireEffect);
            tower.lastFiredTime = GetTime();
        }
    }
}

void ActivateTowerAbility(Tower& tower) {
    if (tower.abilityCooldownTimer > 0.0f || tower.abilityActive) return;
    tower.abilityCooldownTimer = tower.abilityCooldownDuration;
    switch (tower.type) {
        case TIER1_DEFAULT:
            tower.abilityActive = true;
            tower.abilityTimer = tower.abilityDuration;
            for (auto& enemy : enemies) {
                if (enemy.active && Vector2Distance(tower.position, enemy.position) <= tower.range) {
                    enemy.isSlowed = true;
                    enemy.slowTimer = tower.abilityDuration;
                    enemy.speed = enemy.originalSpeed * 0.5f;
                }
            }
            break;
        case TIER2_FAST:
            tower.abilityActive = true;
            tower.abilityTimer = tower.abilityDuration;
            tower.originalFireRate = tower.fireRate;
            tower.fireRate *= 1.5f;
            break;
        case TIER3_STRONG:
            tower.isPowerShotActive = true;
            break;
        default: break;
    }
}

void HandleTowerAbilityButton() {
    if (selectedTowerIndex >= 0 && selectedTowerIndex < towers.size()) {
        Tower& selectedTower = towers[selectedTowerIndex];
        Rectangle abilityButton = { (float)selectedTowerInfoX, (float)(selectedTowerInfoY + infoSpacing * 8), (float)abilityButtonWidth, (float)abilityButtonHeight };
        bool canActivate = (selectedTower.abilityCooldownTimer <= 0.0f && !selectedTower.abilityActive);
        Color buttonColor = canActivate ? BLUE : GRAY;
        const char* buttonText = selectedTower.abilityActive ? TextFormat("Active: %.1fs", selectedTower.abilityTimer) :
                               selectedTower.abilityCooldownTimer > 0.0f ? TextFormat("Cooldown: %.1fs", selectedTower.abilityCooldownTimer) : "Activate Ability";
        DrawRectangleRec(abilityButton, buttonColor);
        DrawRectangleLinesEx(abilityButton, 2.0f, BLACK);
        int textWidth = MeasureText(buttonText, 18);
        int textX = selectedTowerInfoX + (abilityButtonWidth - textWidth) / 2;
        DrawText(buttonText, textX, selectedTowerInfoY + infoSpacing * 8 + 10, 18, WHITE);
        DrawText(selectedTower.type == TIER1_DEFAULT ? "Area Slow (3s)" : selectedTower.type == TIER2_FAST ? "Speed Boost (5s)" : "Power Shot (3x DMG)",
                 selectedTowerInfoX, selectedTowerInfoY + infoSpacing * 7, 16, BLACK);
        if (canActivate && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), abilityButton)) {
            ActivateTowerAbility(selectedTower);
        }
    }
}

void RepairTower(Tower& tower) {
    if (tower.isMalfunctioning && playerMoney >= 50) {
        playerMoney -= 50;
        tower.isMalfunctioning = false;
        tower.lastFiredTime = GetTime();
        if (tower.type == TIER1_DEFAULT) tower.color = BLUE;
        else if (tower.type == TIER2_FAST) tower.color = GREEN;
        else if (tower.type == TIER3_STRONG) tower.color = RED;
    }
}

void DrawTowerTooltip(TowerType type, Vector2 position) {
    Vector2 mousePos = GetMousePosition();
    Rectangle tier1Rec = { (float)towerMenuStartX, (float)towerMenuStartY, (float)towerSelectionWidth, (float)towerSelectionHeight };
    Rectangle tier2Rec = { (float)towerMenuStartX + towerMenuSpacingX, (float)towerMenuStartY, (float)towerSelectionWidth, (float)towerSelectionHeight };
    Rectangle tier3Rec = { (float)towerMenuStartX + towerMenuSpacingX * 2, (float)towerMenuStartY, (float)towerSelectionWidth, (float)towerSelectionHeight };
    bool tooltipShown = false;
    if (CheckCollisionPointRec(mousePos, tier1Rec) && type == TIER1_DEFAULT) tooltipShown = true;
    else if (CheckCollisionPointRec(mousePos, tier2Rec) && type == TIER2_FAST) tooltipShown = true;
    else if (CheckCollisionPointRec(mousePos, tier3Rec) && type == TIER3_STRONG) tooltipShown = true;
    if (!tooltipShown) return;

    Tower dummyTower = CreateTower(type, {0, 0});
    int tooltipWidth = 200, tooltipHeight = 150, padding = 10, fontSize = 15, lineHeight = fontSize + 2;
    float tooltipX = position.x + 20;
    if (tooltipX + tooltipWidth > screenWidth) tooltipX = screenWidth - tooltipWidth - 5;
    float tooltipY = position.y;
    if (tooltipY + tooltipHeight > screenHeight) tooltipY = screenHeight - tooltipHeight - 5;
    DrawRectangle(tooltipX, tooltipY, tooltipWidth, tooltipHeight, ColorAlpha(LIGHTGRAY, 0.9f));
    DrawRectangleLinesEx((Rectangle){tooltipX, tooltipY, (float)tooltipWidth, (float)tooltipHeight}, 2, BLACK);
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
    DrawText(type == TIER1_DEFAULT ? "Area Slow (3s)" : type == TIER2_FAST ? "Speed Boost (5s)" : "Power Shot (3x DMG)", tooltipX + padding + 10, textY, fontSize - 2, DARKGRAY);
}