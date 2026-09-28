/**
 * @file pathfinding.h
 * @brief Grid A* pathfinding for maze/tactical navigation.
 *
 * A small, self-contained 4-directional A* over a rectangular grid. The caller
 * supplies a walkability predicate, so the same routine works for mazes, battle
 * maps, or any cell grid.
 *
 * @author DND Engine Team
 * @date 2026-06-03
 * @version 0.12.0
 */
#pragma once

#include "core/types.h"
#include <vector>
#include <functional>

namespace Dnd {

/// @brief A grid cell coordinate (column x, row y).
struct Cell {
    i32 x;
    i32 y;
};

/**
 * @brief 4-directional A* from start to goal on a [0,width) x [0,height) grid.
 * @param width Grid width in cells.
 * @param height Grid height in cells.
 * @param start Start cell.
 * @param goal Goal cell.
 * @param isWalkable Returns true if cell (x,y) can be entered.
 * @return The path from start to goal inclusive, or an empty vector if unreachable.
 */
std::vector<Cell> FindPath(i32 width, i32 height, Cell start, Cell goal,
                           const std::function<bool(i32, i32)>& isWalkable);

} // namespace Dnd
