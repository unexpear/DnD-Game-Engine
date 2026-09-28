/**
 * @file gizmo.cpp
 * @brief Implementation of the GizmoManager editor transform gizmo.
 */
#include "tools/gizmo.h"
#include "tools/editor_settings.h"
#include "tools/undo.h"
#include "core/log.h"
#include "core/input.h"
#include "rendering/debug_draw.h"
#include "physics/collision.h"
#include "ecs/component.h"
#include "ecs/components/transform.h"
#include "scene/scene.h"
#include "rendering/renderer.h"

namespace Dnd {

GizmoManager* GizmoManager::s_instance = nullptr;

GizmoManager& GizmoManager::Get() {
    if (!s_instance) {
        s_instance = new GizmoManager();
    }
    return *s_instance;
}

void GizmoManager::Initialize() {
    m_hoveredAxis = GizmoAxis::None;
    m_draggedAxis = GizmoAxis::None;
    m_initialized = true;
    DND_LOG_INFO("GizmoManager initialized");
}

void GizmoManager::Shutdown() {
    m_initialized = false;
}

Vec3 GizmoManager::AxisDir(GizmoAxis axis) {
    switch (axis) {
        case GizmoAxis::X: return Vec3(1, 0, 0);
        case GizmoAxis::Y: return Vec3(0, 1, 0);
        case GizmoAxis::Z: return Vec3(0, 0, 1);
        default:           return Vec3(0, 0, 0);
    }
}

f32 GizmoManager::RaySegmentDistance(const Vec3& rayOrigin, const Vec3& rayDir,
                                      const Vec3& segStart, const Vec3& segEnd) {
    Vec3 segDir = segEnd - segStart;
    f32 segLen = segDir.Length();
    if (segLen < 1e-8f) return 1e10f;
    segDir = segDir / segLen;

    Vec3 w0 = rayOrigin - segStart;
    f32 a = rayDir.Dot(rayDir);
    f32 b = rayDir.Dot(segDir);
    f32 c = segDir.Dot(segDir);
    f32 d = rayDir.Dot(w0);
    f32 e = segDir.Dot(w0);

    f32 denom = a * c - b * b;
    if (std::abs(denom) < 1e-8f) return 1e10f;

    f32 t = (b * e - c * d) / denom;
    f32 s = (a * e - b * d) / denom;

    if (s < 0.0f) s = 0.0f;
    if (s > segLen) s = segLen;

    Vec3 closestOnRay = rayOrigin + rayDir * t;
    Vec3 closestOnSeg = segStart + segDir * s;

    return (closestOnRay - closestOnSeg).Length();
}

b8 GizmoManager::ClosestPointOnAxis(const Vec3& rayOrigin, const Vec3& rayDir,
                                     const Vec3& axisOrigin, const Vec3& axisDir,
                                     f32& outS) {
    Vec3 delta = axisOrigin - rayOrigin;
    f32 b = axisDir.Dot(rayDir);
    f32 d1 = delta.Dot(axisDir);
    f32 d2 = delta.Dot(rayDir);

    f32 denom = b * b - 1.0f;
    if (std::abs(denom) < 1e-8f) {
        outS = 0.0f;
        return false;
    }

    f32 t = (b * d1 - d2) / denom;
    outS = b * t - d1;
    return true;
}

namespace {
    f32 Snap(f32 value, f32 increment) {
        if (increment <= 0.0f) return value;
        return std::floor(value / increment + 0.5f) * increment;
    }
}

void GizmoManager::Update(f32 dt, Scene* scene, f32 viewportWidth, f32 viewportHeight) {
    (void)dt;
    if (!m_initialized) return;
    EditorSettings& settings = EditorSettings::Get();
    if (!settings.editorActive) {
        m_draggedAxis = GizmoAxis::None;
        m_hoveredAxis = GizmoAxis::None;
        return;
    }

    Entity selected = settings.selectedEntity;
    if (!scene || !selected.IsValid()) {
        m_draggedAxis = GizmoAxis::None;
        m_hoveredAxis = GizmoAxis::None;
        return;
    }

    ComponentRegistry& cr = scene->GetComponentRegistry();
    Transform* xf = cr.GetComponent<Transform>(selected);
    if (!xf) return;

    Vec3 gizmoOrigin = xf->position;

    // Build pick ray from mouse position
    f32 mx = Input::GetMouseX();
    f32 my = Input::GetMouseY();

    f32 ndcX = (2.0f * mx / viewportWidth) - 1.0f;
    f32 ndcY = 1.0f - (2.0f * my / viewportHeight);

    const Mat4& viewMat = Renderer::GetViewMatrix();
    const Mat4& projMat = Renderer::GetProjectionMatrix();
    Mat4 invVP = (projMat * viewMat).Inverse();
    Vec4 nearH = invVP * Vec4(ndcX, ndcY, -1.0f, 1.0f);
    Vec4 farH  = invVP * Vec4(ndcX, ndcY,  1.0f, 1.0f);
    Vec3 nearW = nearH.ToVec3() / nearH.w;
    Vec3 farW  = farH.ToVec3() / farH.w;
    Vec3 rayDir = (farW - nearW).Normalized();

    // Handle dragging per mode
    if (m_draggedAxis != GizmoAxis::None) {
        if (!Input::IsMouseButtonDown(MouseButton::LEFT)) {
            // Drag ended: push undo command if anything changed
            if (m_dragEntity.IsValid()) {
                Transform* dxf = cr.GetComponent<Transform>(m_dragEntity);
                if (dxf) {
                    b8 changed = false;
                    changed |= (dxf->position != m_dragStartPos);
                    changed |= (dxf->rotation.x != m_dragStartRot.x || dxf->rotation.y != m_dragStartRot.y || dxf->rotation.z != m_dragStartRot.z || dxf->rotation.w != m_dragStartRot.w);
                    changed |= (dxf->scale != m_dragStartScale);
                    if (changed) {
                        UndoStack::Get().Push(new TransformCommand(
                            m_dragEntity,
                            m_dragStartPos, m_dragStartRot, m_dragStartScale,
                            dxf->position, dxf->rotation, dxf->scale));
                    }
                }
            }
            m_dragEntity = Entity::None();
            m_draggedAxis = GizmoAxis::None;
            m_hoveredAxis = GizmoAxis::None;
            return;
        }

        f32 s;
        if (!ClosestPointOnAxis(nearW, rayDir, m_dragGizmoOrigin, AxisDir(m_draggedAxis), s)) return;
        f32 deltaS = s - m_dragStartS;

        b8 snap = Input::IsKeyDown(KeyCode::LEFT_SHIFT) || Input::IsKeyDown(KeyCode::RIGHT_SHIFT);

        if (settings.gizmoMode == GizmoMode::Translate) {
            Vec3 newPos = m_dragStartPos + AxisDir(m_draggedAxis) * deltaS;
            if (snap) {
                newPos.x = Snap(newPos.x, 0.25f);
                newPos.y = Snap(newPos.y, 0.25f);
                newPos.z = Snap(newPos.z, 0.25f);
            }
            xf->position = newPos;
        } else if (settings.gizmoMode == GizmoMode::Scale) {
            Vec3 axis = AxisDir(m_draggedAxis);
            f32 mult = 1.0f + deltaS / AXIS_LENGTH;
            if (snap) mult = Snap(mult, 0.25f);
            if (mult < 0.01f) mult = 0.01f;
            Vec3 newScale = m_dragStartScale;
            newScale.x *= (axis.x != 0.0f) ? mult : 1.0f;
            newScale.y *= (axis.y != 0.0f) ? mult : 1.0f;
            newScale.z *= (axis.z != 0.0f) ? mult : 1.0f;
            xf->scale = newScale;
        } else if (settings.gizmoMode == GizmoMode::Rotate) {
            f32 angle = RingAngle(nearW, rayDir, gizmoOrigin, AxisDir(m_draggedAxis));
            f32 deltaAngle = angle - m_dragStartAngle;
            if (snap) deltaAngle = Snap(deltaAngle, Math::DegreesToRadians(15.0f));
            Vec3 axisDir = AxisDir(m_draggedAxis);
            f32 ha = deltaAngle * 0.5f;
            f32 s = std::sin(ha);
            Quat rotDelta(axisDir.x * s, axisDir.y * s, axisDir.z * s, std::cos(ha));
            xf->rotation = (rotDelta * m_dragStartRot).Normalized();
        }
        return;
    }

    // If mode is not recognized, clear and return
    if (settings.gizmoMode != GizmoMode::Translate && settings.gizmoMode != GizmoMode::Scale && settings.gizmoMode != GizmoMode::Rotate) {
        m_hoveredAxis = GizmoAxis::None;
        return;
    }

    // Hit-test axes
    m_hoveredAxis = GizmoAxis::None;

    GizmoAxis axes[] = { GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z };
    for (i32 i = 0; i < 3; ++i) {
        GizmoAxis axis = axes[i];
        Vec3 dir = AxisDir(axis);

        if (settings.gizmoMode == GizmoMode::Translate) {
            Vec3 shaftEnd = gizmoOrigin + dir * AXIS_LENGTH * 0.8f;
            Vec3 headTip  = gizmoOrigin + dir * AXIS_LENGTH;

            f32 dist = RaySegmentDistance(nearW, rayDir, gizmoOrigin, shaftEnd);
            if (dist < HIT_RADIUS) { m_hoveredAxis = axis; break; }

            dist = RaySegmentDistance(nearW, rayDir, shaftEnd, headTip);
            if (dist < HIT_RADIUS * 1.5f) { m_hoveredAxis = axis; break; }
        } else if (settings.gizmoMode == GizmoMode::Scale) {
            Vec3 cubeCenter = gizmoOrigin + dir * AXIS_LENGTH;
            f32 cubeHalf = 0.15f;
            Vec3 shaftEnd = gizmoOrigin + dir * (AXIS_LENGTH - cubeHalf);
            f32 dist = RaySegmentDistance(nearW, rayDir, gizmoOrigin, shaftEnd);
            if (dist < HIT_RADIUS) { m_hoveredAxis = axis; break; }

            f32 t;
            if (Collision::IntersectRayAABB(Ray(nearW, rayDir), cubeCenter - Vec3(cubeHalf, cubeHalf, cubeHalf), cubeCenter + Vec3(cubeHalf, cubeHalf, cubeHalf), t)) {
                m_hoveredAxis = axis; break;
            }
        } else if (settings.gizmoMode == GizmoMode::Rotate) {
            // Test ray against ring circle (semicircle arc)
            const f32 ringRadius = 1.6f;
            const f32 tubeRadius = 0.2f;

            // Find intersection of ray with the ring plane
            f32 denom = rayDir.Dot(dir);
            if (std::abs(denom) < 1e-8f) continue; // ray parallel to plane

            f32 planeT = -(nearW - gizmoOrigin).Dot(dir) / denom;
            Vec3 planePt = nearW + rayDir * planeT;

            // Check if point lies within ring band
            Vec3 fromCenter = planePt - gizmoOrigin;
            f32 dist = fromCenter.Length();
            if (dist >= ringRadius - tubeRadius && dist <= ringRadius + tubeRadius) {
                m_hoveredAxis = axis; break;
            }
        }
    }

    // Start drag
    if (m_hoveredAxis != GizmoAxis::None &&
        Input::IsMouseButtonPressed(MouseButton::LEFT)) {

        m_draggedAxis = m_hoveredAxis;
        m_dragGizmoOrigin = gizmoOrigin;
        m_dragEntity = selected;
        ClosestPointOnAxis(nearW, rayDir, m_dragGizmoOrigin, AxisDir(m_draggedAxis), m_dragStartS);

        if (settings.gizmoMode == GizmoMode::Translate) {
            m_dragStartPos = xf->position;
        } else if (settings.gizmoMode == GizmoMode::Scale) {
            m_dragStartScale = xf->scale;
        } else if (settings.gizmoMode == GizmoMode::Rotate) {
            m_dragStartRot = xf->rotation;
            m_dragStartAngle = RingAngle(nearW, rayDir, gizmoOrigin, AxisDir(m_draggedAxis));
        }

        DND_LOG_DEBUG("Gizmo: started drag on axis %d", static_cast<i32>(m_draggedAxis));
    }
}

void GizmoManager::Render(Scene* scene) {
    if (!m_initialized) return;
    EditorSettings& settings = EditorSettings::Get();
    if (!settings.editorActive) return;

    Entity selected = settings.selectedEntity;
    if (!scene || !selected.IsValid()) return;

    // Render based on gizmo mode
    if (settings.gizmoMode == GizmoMode::Translate) {
        DrawTranslateGizmo(scene, selected);
    } else if (settings.gizmoMode == GizmoMode::Scale) {
        DrawScaleGizmo(scene, selected);
    } else if (settings.gizmoMode == GizmoMode::Rotate) {
        DrawRotateGizmo(scene, selected);
    }
}

void GizmoManager::DrawTranslateGizmo(Scene* scene, Entity selected) {
    ComponentRegistry& cr = scene->GetComponentRegistry();
    Transform* xf = cr.GetComponent<Transform>(selected);
    if (!xf) return;

    Vec3 origin = xf->position;
    f32 len = AXIS_LENGTH;

    GizmoAxis axes[] = { GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z };
    Color axisColors[] = {
        Color(1.0f, 0.2f, 0.2f, 1.0f),
        Color(0.2f, 1.0f, 0.2f, 1.0f),
        Color(0.2f, 0.4f, 1.0f, 1.0f)
    };
    Color highlightColor(1.0f, 1.0f, 0.2f, 1.0f);

    for (i32 i = 0; i < 3; ++i) {
        GizmoAxis axis = axes[i];
        Vec3 dir = AxisDir(axis);

        Color color = axisColors[i];
        if (axis == m_hoveredAxis || axis == m_draggedAxis) {
            color = highlightColor;
        }

        f32 shaftLen = len * 0.8f;
        Vec3 shaftEnd = origin + dir * shaftLen;
        Vec3 headTip  = origin + dir * len;

        DebugDraw::DrawLine(origin, shaftEnd, color);

        // Arrowhead: cone made of line segments
        Vec3 perp = Vec3(0, 0, 0);
        if (std::abs(dir.x) < 0.9f) {
            perp = dir.Cross(Vec3(1, 0, 0)).Normalized();
        } else {
            perp = dir.Cross(Vec3(0, 1, 0)).Normalized();
        }
        Vec3 binorm = dir.Cross(perp).Normalized();
        f32 headRadius = 0.15f;
        i32 coneSegs = 8;
        for (i32 j = 0; j < coneSegs; ++j) {
            f32 a1 = static_cast<f32>(j) / static_cast<f32>(coneSegs) * Math::TWO_PI;
            f32 a2 = static_cast<f32>(j + 1) / static_cast<f32>(coneSegs) * Math::TWO_PI;
            Vec3 r1 = (perp * std::cos(a1) + binorm * std::sin(a1)) * headRadius;
            Vec3 r2 = (perp * std::cos(a2) + binorm * std::sin(a2)) * headRadius;
            Vec3 p1 = shaftEnd + r1;
            Vec3 p2 = shaftEnd + r2;
            DebugDraw::DrawLine(headTip, p1, color);
            DebugDraw::DrawLine(p1, p2, color);
        }
        DebugDraw::DrawLine(headTip, shaftEnd, color);
    }
}

f32 GizmoManager::RingAngle(const Vec3& rayOrigin, const Vec3& rayDir,
                             const Vec3& ringCenter, const Vec3& axisDir) {
    // Intersect ray with plane through ringCenter, normal = axisDir
    f32 denom = rayDir.Dot(axisDir);
    if (std::abs(denom) < 1e-8f) return 0.0f;

    f32 t = -(rayOrigin - ringCenter).Dot(axisDir) / denom;
    Vec3 pt = rayOrigin + rayDir * t;
    Vec3 fromCenter = pt - ringCenter;

    // Build two basis vectors in the ring plane
    Vec3 bx, by;
    if (std::abs(axisDir.x) < 0.9f) {
        bx = axisDir.Cross(Vec3(1, 0, 0)).Normalized();
    } else {
        bx = axisDir.Cross(Vec3(0, 1, 0)).Normalized();
    }
    by = axisDir.Cross(bx).Normalized();

    return std::atan2(fromCenter.Dot(by), fromCenter.Dot(bx));
}

void GizmoManager::DrawScaleGizmo(Scene* scene, Entity selected) {
    ComponentRegistry& cr = scene->GetComponentRegistry();
    Transform* xf = cr.GetComponent<Transform>(selected);
    if (!xf) return;

    Vec3 origin = xf->position;
    f32 len = AXIS_LENGTH;

    GizmoAxis axes[] = { GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z };
    Color axisColors[] = {
        Color(1.0f, 0.2f, 0.2f, 1.0f),
        Color(0.2f, 1.0f, 0.2f, 1.0f),
        Color(0.2f, 0.4f, 1.0f, 1.0f)
    };
    Color highlightColor(1.0f, 1.0f, 0.2f, 1.0f);

    for (i32 i = 0; i < 3; ++i) {
        GizmoAxis axis = axes[i];
        Vec3 dir = AxisDir(axis);

        Color color = axisColors[i];
        if (axis == m_hoveredAxis || axis == m_draggedAxis) {
            color = highlightColor;
        }

        // Shaft
        f32 cubeHalf = 0.15f;
        Vec3 shaftEnd = origin + dir * (len - cubeHalf);
        Vec3 cubeCenter = origin + dir * len;
        DebugDraw::DrawLine(origin, shaftEnd, color);

        // Cube at tip (12 edges)
        Vec3 cMin = cubeCenter - Vec3(cubeHalf, cubeHalf, cubeHalf);
        Vec3 cMax = cubeCenter + Vec3(cubeHalf, cubeHalf, cubeHalf);
        DebugDraw::DrawBox(cubeCenter, Vec3(cubeHalf, cubeHalf, cubeHalf), color);
    }
}

void GizmoManager::DrawRotateGizmo(Scene* scene, Entity selected) {
    ComponentRegistry& cr = scene->GetComponentRegistry();
    Transform* xf = cr.GetComponent<Transform>(selected);
    if (!xf) return;

    Vec3 origin = xf->position;
    const f32 ringRadius = 1.6f;
    const i32 segments = 32;

    GizmoAxis axes[] = { GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z };
    Color axisColors[] = {
        Color(1.0f, 0.2f, 0.2f, 1.0f),
        Color(0.2f, 1.0f, 0.2f, 1.0f),
        Color(0.2f, 0.4f, 1.0f, 1.0f)
    };
    Color highlightColor(1.0f, 1.0f, 0.2f, 1.0f);

    for (i32 i = 0; i < 3; ++i) {
        GizmoAxis axis = axes[i];
        Vec3 dir = AxisDir(axis);

        Color color = axisColors[i];
        if (axis == m_hoveredAxis || axis == m_draggedAxis) {
            color = highlightColor;
        }

        // Build two basis vectors for the ring plane
        Vec3 bx, by;
        if (std::abs(dir.x) < 0.9f) {
            bx = dir.Cross(Vec3(1, 0, 0)).Normalized();
        } else {
            bx = dir.Cross(Vec3(0, 1, 0)).Normalized();
        }
        by = dir.Cross(bx).Normalized();

        // Draw ring as line segments
        for (i32 j = 0; j < segments; ++j) {
            f32 a0 = (static_cast<f32>(j) / static_cast<f32>(segments)) * Math::TWO_PI;
            f32 a1 = (static_cast<f32>(j + 1) / static_cast<f32>(segments)) * Math::TWO_PI;
            Vec3 p0 = origin + (bx * std::cos(a0) + by * std::sin(a0)) * ringRadius;
            Vec3 p1 = origin + (bx * std::cos(a1) + by * std::sin(a1)) * ringRadius;
            DebugDraw::DrawLine(p0, p1, color);
        }
    }
}

} // namespace Dnd
