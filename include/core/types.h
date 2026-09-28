/**
 * @file types.h
 * @brief Platform-independent type aliases and utility macros.
 * 
 * This module defines fixed-width type aliases used throughout the entire engine.
 * All engine code uses these aliases instead of raw C/C++ types to ensure:
 *   - Explicit size semantics (i32 is always 32 bits, regardless of platform)
 *   - Clear intent (u8 signals byte-sized data, f32 signals single-precision float)
 *   - Portability across Windows, Linux, and macOS
 * 
 * @note This header has no implementation. It is purely type definitions.
 * @warning Do NOT include standard integer types directly in engine code.
 *          Always use these aliases for consistency.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#pragma once

#include <cstdint>
#include <cstddef>

// ==========================================
// SIGNED INTEGER TYPES
// ==========================================

/** @brief 8-bit signed integer. Range: -128 to 127. Used for small counters, flags. */
using i8 = int8_t;

/** @brief 16-bit signed integer. Range: -32,768 to 32,767. Used for texture coordinates, indices. */
using i16 = int16_t;

/** @brief 32-bit signed integer. Range: -2,147,483,648 to 2,147,483,647. Default integer type. */
using i32 = int32_t;

/** @brief 64-bit signed integer. Used for file offsets, large counts, timestamps. */
using i64 = int64_t;

// ==========================================
// UNSIGNED INTEGER TYPES
// ==========================================

/** @brief 8-bit unsigned integer. Range: 0 to 255. Used for byte data, color channels. */
using u8 = uint8_t;

/** @brief 16-bit unsigned integer. Range: 0 to 65,535. Used for mesh indices. */
using u16 = uint16_t;

/** @brief 32-bit unsigned integer. Range: 0 to 4,294,967,295. Used for entity IDs, hashes. */
using u32 = uint32_t;

/** @brief 64-bit unsigned integer. Used for file sizes, memory addresses. */
using u64 = uint64_t;

// ==========================================
// FLOATING-POINT TYPES
// ==========================================

/** @brief 32-bit IEEE 754 single-precision float. Default float type for engine math. */
using f32 = float;

/** @brief 64-bit IEEE 754 double-precision float. Used for high-precision calculations. */
using f64 = double;

// ==========================================
// BOOLEAN TYPES
// ==========================================

/** @brief 8-bit boolean. Stored as a single byte. Used in packed structures. */
using b8 = bool;

/** @brief 32-bit boolean. Stored as a 4-byte word. Used for alignment in performance-critical code. */
using b32 = bool;

// ==========================================
// UTILITY MACROS
// ==========================================

/**
 * @brief Computes the number of elements in a fixed-size array at compile time.
 * 
 * Uses sizeof(arr) / sizeof(arr[0]) to calculate element count.
 * Only works on actual arrays, not pointers. Passing a pointer will
 * produce incorrect results (sizeof pointer / sizeof element).
 * 
 * @param arr The array to compute the element count of. Must be a real array, not a pointer.
 * @return The number of elements in the array as a compile-time constant.
 * 
 * @par Example:
 * @code
 * int numbers[10];
 * size_t count = DND_ARRAY_SIZE(numbers); // count == 10
 * @endcode
 * 
 * @warning Do NOT use on pointers. `int* p = arr; DND_ARRAY_SIZE(p)` returns wrong value.
 */
#define DND_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/**
 * @brief Suppresses compiler "unused variable" warnings.
 * 
 * Casts the given variable to void, signaling to the compiler that
 * the variable is intentionally unused. Useful for parameters that
 * are required by an interface but not used in a specific implementation.
 * 
 * @param x The variable to mark as unused.
 * 
 * @par Example:
 * @code
 * void OnEvent(int eventId, const char* data) {
 *     DND_UNUSED(data); // data not needed for this handler
 *     Log("Event %d", eventId);
 * }
 * @endcode
 */
#define DND_UNUSED(x) (void)(x)
