#include "game.h"

Vector2Int GetGridCoords(Vector2 position) {
    return { (int)(position.x / tileWidth), (int)(position.y / tileHeight) };
}

Vector2 GetTileCenter(Vector2Int gridCoords) {
    return { (float)(gridCoords.x * tileWidth + tileWidth / 2), (float)(gridCoords.y * tileHeight + tileHeight / 2) };
}

struct GridCell {
    Vector2Int coords;
    Vector2Int parentCoords;
    int distance;
};

vector<Vector2Int> FindPathBFS(Vector2Int start, Vector2Int end) {
    if (!grid[end.y][end.x]) return {};
    queue<GridCell> cellQueue;
    map<pair<int, int>, bool> visited;
    cellQueue.push({start, {-1, -1}, 0});
    visited[{start.x, start.y}] = true;
    map<pair<int, int>, Vector2Int> parentMap;
    while (!cellQueue.empty()) {
        GridCell currentCell = cellQueue.front();
        cellQueue.pop();
        if (currentCell.coords.x == end.x && currentCell.coords.y == end.y) {
            vector<Vector2Int> path;
            Vector2Int currentCoords = currentCell.coords;
            while (currentCoords.x != -1 && currentCoords.y != -1) {
                path.push_back(currentCoords);
                auto it = parentMap.find({currentCoords.x, currentCoords.y});
                if (it == parentMap.end()) break;
                currentCoords = it->second;
            }
            reverse(path.begin(), path.end());
            return path;
        }
        int dx[] = {0, 0, 1, -1};
        int dy[] = {-1, 1, 0, 0};
        for (int i = 0; i < 4; ++i) {
            Vector2Int neighborCoords = {currentCell.coords.x + dx[i], currentCell.coords.y + dy[i]};
            if (neighborCoords.x >= 0 && neighborCoords.x < gridColumns && neighborCoords.y >= 0 && neighborCoords.y < gridRows &&
                grid[neighborCoords.y][neighborCoords.x] && !visited[{neighborCoords.x, neighborCoords.y}]) {
                cellQueue.push({neighborCoords, currentCell.coords, currentCell.distance + 1});
                visited[{neighborCoords.x, neighborCoords.y}] = true;
                parentMap[{neighborCoords.x, neighborCoords.y}] = currentCell.coords;
            }
        }
    }
    return {};
}

void DrawPath(const vector<Vector2Int>& path, Color color) {
    if (path.empty()) return;
    for (size_t i = 0; i < path.size(); ++i) {
        DrawRectangle(path[i].x * tileWidth, path[i].y * tileHeight, tileWidth, tileHeight, ColorAlpha(color, 0.1f));
        if (i < path.size() - 1) {
            DrawLineV(GetTileCenter(path[i]), GetTileCenter(path[i + 1]), ColorAlpha(color, 0.5f));
        }
    }
}