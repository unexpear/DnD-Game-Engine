/**
 * @file rigidbody.h
 * @brief RigidBody component for physics simulation.
 * 
 * Stores mass, velocity, and acceleration for physics integration.
 * Static bodies (mass = 0) are not affected by forces.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.3.0
 */

#pragma once

#include "core/types.h"
#include "core/math.h"

namespace Dnd {

/**
 * @struct RigidBody
 * @brief Physics properties for an entity.
 */
struct RigidBody {
    Vec3 velocity;      ///< Current velocity (units/second)
    Vec3 acceleration;  ///< Current acceleration (units/second^2)
    Vec3 gravity;       ///< Gravity vector (default: 0, -9.81, 0)
    f32 mass;           ///< Mass in kg (0 = static/immovable)
    f32 damping;        ///< Velocity damping factor (0 = none, 1 = full stop)
    b8 isStatic;        ///< If true,不受 forces and doesn't move

    /// @brief Default constructor (dynamic body, default gravity).
    RigidBody()
        : velocity(0, 0, 0), acceleration(0, 0, 0), gravity(0, -9.81f, 0),
          mass(1.0f), damping(0.0f), isStatic(false) {}

    /**
     * @brief Constructs a rigidbody with specified mass.
     * @param bodyMass Mass in kg. If 0, body is static.
     */
    explicit RigidBody(f32 bodyMass)
        : velocity(0, 0, 0), acceleration(0, 0, 0), gravity(0, -9.81f, 0),
          mass(bodyMass), damping(0.0f), isStatic(bodyMass == 0.0f) {}

    /**
     * @brief Constructs a rigidbody with mass, damping, and static flag.
     * @param bodyMass Mass in kg.
     * @param bodyDamping Velocity damping factor (0 = none, 1 = full stop).
     * @param staticBody If true, body is immovable.
     */
    RigidBody(f32 bodyMass, f32 bodyDamping, b8 staticBody)
        : velocity(0, 0, 0), acceleration(0, 0, 0), gravity(0, -9.81f, 0),
          mass(bodyMass), damping(bodyDamping), isStatic(staticBody) {}

    /// @brief Creates a static (immovable) rigidbody.
    static RigidBody Static() {
        RigidBody rb;
        rb.isStatic = true;
        rb.mass = 0.0f;
        return rb;
    }
};

} // namespace Dnd
