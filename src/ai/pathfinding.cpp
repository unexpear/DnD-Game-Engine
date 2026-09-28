/**
 * @file pathfinding.cpp
 * @brief Implementation of grid A* (see pathfinding.h).
 *
 * @author DND Engine Team
 * @date 2026-06-03
 * @version 0.12.0
 */

#include "ai/pathfinding.h"
#include <queue>
#include <algorithm>
#include <cstdlib>
#include <climits>

namespace Dnd {

std::vector<Cell> FindPath(i32 width, i32 height, Cell start, Cell goal,
                           const std::function<bool(i32, i32)>& isWalkable) {
    std::vector<Cell> path;
    if (width <= 0 || height <= 0) return path;

    auto inBounds = [&](i32 x, i32 y) { return x >= 0 && x < width && y >= 0 && y < height; };
    if (!inBounds(start.x, start.y) || !inBounds(goal.x, goal.y)) return path;
    if (!isWalkable(goal.x, goal.y) || !isWalkable(start.x, start.y)) return path;

    const i32 n = width * height;
    auto idx = [&](i32 x, i32 y) { return y * width + x; };
    auto heuristic = [&](i32 x, i32 y) { return std::abs(x - goal.x) + std::abs(y - goal.y); };

    std::vector<i32> cameFrom(n, -1);
    std::vector<i32> gScore(n, INT_MAX);
    std::vector<char> closed(n, 0);

    // Min-heap of (fScore, cellIndex).
    using PQItem = std::pair<i32, i32>;
    std::priority_queue<PQItem, std::vector<PQItem>, std::greater<PQItem>> open;

    gScore[idx(start.x, start.y)] = 0;
    open.push({heuristic(start.x, start.y), idx(start.x, start.y)});

    const i32 dx[4] = { 1, -1, 0, 0 };
    const i32 dy[4] = { 0, 0, 1, -1 };
    bool found = false;

    while (!open.empty()) {
        const i32 cur = open.top().second;
        open.pop();
        if (closed[cur]) continue;
        closed[cur] = 1;

        const i32 cx = cur % width;
        const i32 cy = cur / width;
        if (cx == goal.x && cy == goal.y) { found = true; break; }

        for (i32 d = 0; d < 4; ++d) {
            const i32 nx = cx + dx[d];
            const i32 ny = cy + dy[d];
            if (!inBounds(nx, ny) || !isWalkable(nx, ny)) continue;
            const i32 ni = idx(nx, ny);
            if (closed[ni]) continue;
            const i32 tentative = gScore[cur] + 1;
            if (tentative < gScore[ni]) {
                gScore[ni] = tentative;
                cameFrom[ni] = cur;
                open.push({ tentative + heuristic(nx, ny), ni });
            }
        }
    }

    if (!found) return path;

    // Reconstruct start -> goal.
    for (i32 c = idx(goal.x, goal.y); c != -1; c = cameFrom[c]) {
        path.push_back({ c % width, c / width });
    }
    std::reverse(path.begin(), path.end());
    return path;
}

} // namespace Dnd
