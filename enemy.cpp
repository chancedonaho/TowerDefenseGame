#include "game.h"

Enemy CreateEnemy(EnemyType type, Vector2 startPosition) {
    Enemy newEnemy;
    newEnemy.position = startPosition;
    newEnemy.currentWaypoint = 0;
    newEnemy.active = true;
    newEnemy.type = type;
    newEnemy.texture = { 0 };
    newEnemy.slowTimer = 0.0f;
    newEnemy.isSlowed = false;
    newEnemy.hasDotEffect = false;
    newEnemy.dotTimer = 0.0f;
    newEnemy.dotTickTimer = 0.0f;
    newEnemy.dotDamage = 0;
    newEnemy.pathCheckTimer = 0.0f; // Initialize individual path check timer
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
            newEnemy.speed = 60.0f;
            newEnemy.hp = 150;
            newEnemy.maxHp = 150;
            newEnemy.color = DARKGRAY;
            newEnemy.texture = tier3EnemyTexture;
            break;
        case FAST_ARMOURED_ENEMY:
            newEnemy.speed = 90.0f;
            newEnemy.hp = 100;
            newEnemy.maxHp = 100;
            newEnemy.color = GOLD;
            newEnemy.texture = tier4EnemyTexture;
            break;
        default:
            newEnemy.speed = 75.0f;
            newEnemy.hp = 50;
            newEnemy.maxHp = 50;
            newEnemy.color = PINK;
            break;
    }
    newEnemy.originalSpeed = newEnemy.speed;
    
    // Calculate initial path when enemy is created
    Vector2Int startGrid = GetGridCoords(newEnemy.position);
    Vector2Int endGrid = GetGridCoords(waypoints[waypoints.size()-1]);
    newEnemy.waypointsPath = FindPathBFS(startGrid, endGrid);
    newEnemy.pathIndex = 0;
    
    return newEnemy;
}

void UpdateEnemies() {
    for (auto &enemy : enemies) {
        if (!enemy.active) continue;
        
        // Update individual path check timer
        enemy.pathCheckTimer -= GetFrameTime();
        
        // Check if we need to recalculate path
        bool needsRecalculation = false;
        
        // Only check for path recalculation when the timer expires
        if (enemy.pathCheckTimer <= 0.0f) {
            enemy.pathCheckTimer = 1.5f; // Recalculate much less frequently to avoid erratic movement
            
            // Check if current path is blocked by a tower or if we don't have a path yet
            if (enemy.waypointsPath.empty()) {
                needsRecalculation = true;
            } 
            else if (enemy.pathIndex < enemy.waypointsPath.size()) {
                // Check if the next waypoint is blocked
                if (!grid[enemy.waypointsPath[enemy.pathIndex].y][enemy.waypointsPath[enemy.pathIndex].x]) {
                    needsRecalculation = true;
                }
            }
            
            // Recalculate path if needed
            if (needsRecalculation) {
                Vector2Int currentGridPos = GetGridCoords(enemy.position);
                Vector2Int endGridPos = GetGridCoords(waypoints[waypoints.size() - 1]);
                
                // Calculate a new path that avoids towers using BFS
                vector<Vector2Int> newPath = FindPathBFS(currentGridPos, endGridPos);
                
                // Only update the path if we found a valid one
                if (!newPath.empty()) {
                    enemy.waypointsPath = newPath;
                    enemy.pathIndex = 0;
                }
                // If no valid path found, keep the current path and try again later
            }
        }

        // Follow the path
        if (!enemy.waypointsPath.empty()) {
            if (enemy.pathIndex < enemy.waypointsPath.size()) {
                Vector2 target = GetTileCenter(enemy.waypointsPath[enemy.pathIndex]);
                Vector2 direction = Vector2Subtract(target, enemy.position);
                float distance = Vector2Length(direction);
                if (distance < 5.0f) {
                    enemy.pathIndex++;
                } else {
                    Vector2 normalizedDir = Vector2Normalize(direction);
                    enemy.position = Vector2Add(enemy.position, Vector2Scale(normalizedDir, enemy.speed * GetFrameTime()));
                }
            } else {
                enemy.active = false;
                enemiesReachedEnd++;
                if (enemiesReachedEnd >= maxEnemiesReachedEnd) currentState = GAME_OVER;
            }
        } else { // fall back to default static waypoints
            if (enemy.currentWaypoint < waypoints.size()) {
                Vector2 target = waypoints[enemy.currentWaypoint];
                Vector2 direction = Vector2Subtract(target, enemy.position);
                float distance = Vector2Length(direction);
                if (distance < 5.0f) {
                    enemy.currentWaypoint++;
                } else {
                    Vector2 normalizedDir = Vector2Normalize(direction);
                    enemy.position = Vector2Add(enemy.position, Vector2Scale(normalizedDir, enemy.speed * GetFrameTime()));
                }
            } else {
                enemy.active = false;
                enemiesReachedEnd++;
                if (enemiesReachedEnd >= maxEnemiesReachedEnd) currentState = GAME_OVER;
            }
        }
        
        // Handle enemy status effect updates (slow, DoT, etc.)
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
}

void DrawEnemies() {
    // First draw all enemy paths for better layering
    for (const auto& enemy : enemies) {
        if (!enemy.active || enemy.waypointsPath.empty()) continue;
        
        // Draw the dynamic path the enemy is following
        for (size_t i = enemy.pathIndex; i < enemy.waypointsPath.size() - 1; ++i) {
            Vector2 start = GetTileCenter(enemy.waypointsPath[i]);
            Vector2 end = GetTileCenter(enemy.waypointsPath[i + 1]);
            
            // Use enemy color with reduced alpha for the path
            Color pathColor = ColorAlpha(enemy.color, 0.3f);
            DrawLineEx(start, end, 2.0f, pathColor);
            
            // Draw small circles at path nodes
            DrawCircleV(start, 3.0f, pathColor);
            if (i == enemy.waypointsPath.size() - 2) {
                DrawCircleV(end, 3.0f, pathColor);
            }
        }
    }

    // Then draw all enemies over the paths
    for (const auto& enemy : enemies) {
        if (!enemy.active) continue;
        if (enemy.texture.id > 0) {
            Rectangle sourceRec = { 0.0f, 0.0f, (float)enemy.texture.width, (float)enemy.texture.height };
            Rectangle destRec = { enemy.position.x - tileWidth / 2, enemy.position.y - tileHeight / 2, (float)tileWidth, (float)tileHeight };
            DrawTexturePro(enemy.texture, sourceRec, destRec, { 0, 0 }, 0.0f, WHITE);
        } else {
            DrawCircleV(enemy.position, tileWidth / 2.5f, enemy.color);
        }
        float hpRatio = (float)enemy.hp / enemy.maxHp;
        DrawRectangle(enemy.position.x - 15, enemy.position.y - tileHeight / 2 - 10, 30, 5, RED);
        DrawRectangle(enemy.position.x - 15, enemy.position.y - tileHeight / 2 - 10, 30 * hpRatio, 5, GREEN);
        DrawRectangleLines(enemy.position.x - 15, enemy.position.y - tileHeight / 2 - 10, 30, 5, BLACK);
    }
}

void UpdateProjectiles() {
    for (auto& projectile : projectiles) {
        if (!projectile.active || !projectile.targetEnemy->active) {
            projectile.active = false;
            continue;
        }
        Vector2 direction = Vector2Subtract(projectile.targetEnemy->position, projectile.position);
        float distance = Vector2Length(direction);
        if (distance < 5.0f) {
            if (projectile.type == Projectile::Type::STANDARD) {
                int actualDamage = (projectile.targetEnemy->type == ARMOURED_ENEMY || projectile.targetEnemy->type == FAST_ARMOURED_ENEMY) ? (int)(projectile.damage * 0.7f) : projectile.damage;
                projectile.targetEnemy->hp -= actualDamage;
                if (projectile.targetEnemy->hp <= 0) {
                    projectile.targetEnemy->active = false;
                    playerMoney += 10;
                    defeatedEnemies++;
                }
                projectile.active = false;
            } else if (projectile.type == Projectile::Type::FLAMETHROWER) {
                VisualEffect explosion = { projectile.position, 0.5f, 0.5f, ColorAlpha(ORANGE, 0.8f), projectile.effectRadius, true };
                visualEffects.push_back(explosion);
                for (auto& enemy : enemies) {
                    if (!enemy.active) continue;
                    float enemyDistance = Vector2Distance(projectile.position, enemy.position);
                    if (enemyDistance <= projectile.effectRadius) {
                        int initialDamage = (enemy.type == ARMOURED_ENEMY || enemy.type == FAST_ARMOURED_ENEMY) ? (int)(projectile.damage / 3 * 0.7f) : projectile.damage / 3;
                        enemy.hp -= initialDamage;
                        enemy.hasDotEffect = true;
                        enemy.dotTimer = 4.0f;
                        enemy.dotTickTimer = 0.5f;
                        enemy.dotDamage = projectile.damage / 8;
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
            Vector2 normalizedDir = Vector2Normalize(direction);
            projectile.position = Vector2Add(projectile.position, Vector2Scale(normalizedDir, projectile.speed * GetFrameTime()));
        }
    }
}

void DrawProjectiles() {
    for (const auto& projectile : projectiles) {
        if (!projectile.active) continue;
        if (projectile.type == Projectile::Type::STANDARD) {
            if (projectile.texture.id > 0) {
                Rectangle sourceRec = { 0.0f, 0.0f, (float)projectile.texture.width, (float)projectile.texture.height };
                Rectangle destRec = { projectile.position.x - 5, projectile.position.y - 5, 10, 10 };
                DrawTexturePro(projectile.texture, sourceRec, destRec, { 5, 5 }, 0.0f, WHITE);
            } else {
                DrawCircleV(projectile.position, 5.0f, ORANGE);
            }
        } else if (projectile.type == Projectile::Type::FLAMETHROWER) {
            DrawLineEx(projectile.sourcePosition, projectile.position, 5.0f, ColorAlpha(ORANGE, 0.8f));
            VisualEffect flame = { projectile.position, 0.1f, 0.1f, ColorAlpha(ORANGE, 0.6f), 10.0f, true };
            visualEffects.push_back(flame);
        }
    }
}