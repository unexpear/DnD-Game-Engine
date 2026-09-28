/**
 * @file gizmo.h
 * @brief 3D transform gizmo for editor viewport (translate/rotate/scale).
 *
 * Renders interactive axis arrows at the selected entity's position.
 * Supports mouse click-and-drag for axis-constrained transformation.
 */
#pragma once

#include "core/types.h"
#include "core/math.h"
#include "ecs/entity.h"

namespace Dnd {

// Forward declarations
class Scene;

/**
 * @enum GizmoAxis
 * @brief Identifies which gizmo axis the user is interacting with.
 */
enum class GizmoAxis : i32 {
    None = -1,
    X,   ///< Red axis (right)
    Y,   ///< Green axis (up)
    Z    ///< Blue axis (forward)
};

/**
 * @class GizmoManager
 * @brief Manages the interactive 3D gizmo for the editor viewport.
 *
 * Renders axis arrows at the selected entity's position and handles
 * mouse-driven axis-constrained dragging.
 *
 * Singleton. Accessed via GizmoManager::Get().
 */
class GizmoManager {
public:
    static GizmoManager& Get();

    void Initialize();
    void Shutdown();

    /// @brief Returns true if the user is currently dragging a gizmo axis.
    b8 IsDragging() const { return m_draggedAxis != GizmoAxis::None; }

    /**
     * @brief Per-frame update: input handling and dragging logic.
     * Called before rendering.
     * @param dt Delta time in seconds.
     * @param scene The active scene (for entity access).
     * @param viewportWidth Width of the viewport in pixels.
     * @param viewportHeight Height of the viewport in pixels.
     */
    void Update(f32 dt, Scene* scene, f32 viewportWidth, f32 viewportHeight);

    /**
     * @brief Renders the gizmo arrows using DebugDraw.
     * Called after scene rendering, during editor overlay pass.
     * @param scene The active scene (for entity access).
     */
    void Render(Scene* scene);

private:
    GizmoManager() = default;
    ~GizmoManager() = default;
    GizmoManager(const GizmoManager&) = delete;
    GizmoManager& operator=(const GizmoManager&) = delete;

    static GizmoManager* s_instance;

    /** @brief Length of each gizmo axis arrow (from origin). */
    static constexpr f32 AXIS_LENGTH = 2.0f;
    /** @brief Radius threshold for ray-axis hit testing. */
    static constexpr f32 HIT_RADIUS = 0.3f;

    /** @brief Current axis being hovered (for highlighting). */
    GizmoAxis m_hoveredAxis = GizmoAxis::None;
    /** @brief Current axis being dragged (if any). */
    GizmoAxis m_draggedAxis = GizmoAxis::None;
    /** @brief Entity position when drag started (for computing delta). */
    Vec3 m_dragStartPos;
    /** @brief Entity scale when drag started (for scale mode). */
    Vec3 m_dragStartScale;
    /** @brief Entity rotation when drag started (for rotate mode). */
    Quat m_dragStartRot;
    /** @brief Reference angle at drag start (for rotate mode). */
    f32 m_dragStartAngle = 0.0f;
    /** @brief Gizmo origin position when drag started. */
    Vec3 m_dragGizmoOrigin;
    /** @brief Scalar parameter on the axis at drag start (for offset). */
    f32 m_dragStartS = 0.0f;
    /** @brief Entity being dragged (for undo). */
    Entity m_dragEntity;
    /** @brief Whether the gizmo is initialized. */
    b8 m_initialized = false;

    /**
     * @brief Returns the axis direction vector for the given axis enum.
     */
    static Vec3 AxisDir(GizmoAxis axis);

    /**
     * @brief Computes closest distance from a ray to a line segment.
     * @param rayOrigin Ray origin.
     * @param rayDir Ray direction (normalized).
     * @param segStart Line segment start.
     * @param segEnd Line segment end.
     * @return Closest distance from ray to segment, or large value if no valid solution.
     */
    static f32 RaySegmentDistance(const Vec3& rayOrigin, const Vec3& rayDir,
                                  const Vec3& segStart, const Vec3& segEnd);

    /**
     * @brief Computes the scalar parameter s on the axis line for the closest point
     *        between the ray and the infinite axis line.
     * @param rayOrigin Ray origin.
     * @param rayDir Ray direction (normalized).
     * @param axisOrigin Point on the axis line (gizmo position).
     * @param axisDir Axis direction (normalized).
     * @param outS Output: scalar along axis from axisOrigin.
     * @return true if a valid closest point was found.
     */
    static b8 ClosestPointOnAxis(const Vec3& rayOrigin, const Vec3& rayDir,
                                  const Vec3& axisOrigin, const Vec3& axisDir,
                                  f32& outS);

    /**
     * @brief Computes the angle (radians) of the ray intersection with the ring plane
     *        around the given axis.
     * @return Angle in radians, or 0 if no valid intersection.
     */
    static f32 RingAngle(const Vec3& rayOrigin, const Vec3& rayDir,
                          const Vec3& ringCenter, const Vec3& axisDir);

    /** @brief Renders the translate (arrow) gizmo. */
    void DrawTranslateGizmo(Scene* scene, Entity selected);

    /** @brief Renders the scale (cube) gizmo. */
    void DrawScaleGizmo(Scene* scene, Entity selected);

    /** @brief Renders the rotate (ring) gizmo. */
    void DrawRotateGizmo(Scene* scene, Entity selected);
};

} // namespace Dnd
