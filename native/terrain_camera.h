#pragma once
#include "stereo.h"
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <mutex>

struct TerrainCameraInfo {
    Vector3 position, forward;
    float horizontalForwardX, horizontalForwardZ;
    Vector3 right, up;
    float nearClip, aspect;
    uint8_t orthographic;
    uint8_t reserved[3];
    float fovX, fovY, orthoWidth, orthoHeight;
};
static_assert(sizeof(TerrainCameraInfo) == 84);
static_assert(offsetof(TerrainCameraInfo, orthographic) == 64);
static_assert(offsetof(TerrainCameraInfo, fovY) == 72);

inline bool track_terrain_camera(TerrainCameraInfo& camera, Pose head, float tanX, float tanY) {
    auto finite = [](Vector3 v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); };
    if (camera.orthographic || !valid_pose(head) || !finite(camera.position) ||
        !finite(camera.forward) || !finite(camera.right) || !finite(camera.up) ||
        !std::isfinite(camera.aspect) || camera.aspect <= 0 ||
        !std::isfinite(tanX) || !std::isfinite(tanY) || tanX <= 0 || tanY <= 0 || tanX > 20 || tanY > 20) return false;
    // GIANTS' terrain scanner uses its own aspect; cover both eyes conservatively.
    const float tangent = (std::max)(tanY, tanX / camera.aspect) * 1.02f;
    if (!std::isfinite(tangent) || tangent > 20) return false;
    const auto right = camera.right, up = camera.up, forward = camera.forward;
    auto basis = [right, up, forward](Vector3 v) -> Vector3 {
        return {right.x*v.x + up.x*v.y - forward.x*v.z,
                right.y*v.x + up.y*v.y - forward.y*v.z,
                right.z*v.x + up.z*v.y - forward.z*v.z};
    };
    const auto offset = basis(head.position);
    auto result = camera;
    result.position = {camera.position.x + offset.x, camera.position.y + offset.y, camera.position.z + offset.z};
    result.forward = basis(rotate(head.orientation, {0, 0, -1}));
    result.right = basis(rotate(head.orientation, {1, 0, 0}));
    result.up = basis(rotate(head.orientation, {0, 1, 0}));
    const float horizontal = std::hypot(result.forward.x, result.forward.z);
    if (horizontal > 0.00001f) {
        result.horizontalForwardX = result.forward.x / horizontal;
        result.horizontalForwardZ = result.forward.z / horizontal;
    }
    result.fovY = 2 * std::atan(tangent);
    result.fovX = 2 * std::atan(tangent * camera.aspect);
    if (!finite(result.position) || !finite(result.forward) || !finite(result.right) || !finite(result.up)) return false;
    camera = result;
    return true;
}

class TerrainTrackingState {
    std::mutex mutex_;
    Pose head_{};
    float tanX_ = 0, tanY_ = 0;
    uint64_t sampledAt_ = 0;
    bool valid_ = false;
public:
    void publish(Pose head, float tanX, float tanY, uint64_t now) {
        const std::lock_guard<std::mutex> lock(mutex_);
        head_ = head;
        tanX_ = tanX;
        tanY_ = tanY;
        sampledAt_ = now;
        valid_ = true;
    }
    void clear() {
        const std::lock_guard<std::mutex> lock(mutex_);
        valid_ = false;
    }
    bool apply(TerrainCameraInfo& camera, uint64_t now) {
        const std::lock_guard<std::mutex> lock(mutex_);
        // Terrain runs on an update worker; a stalled/lost VR session must expire.
        if (!valid_ || now < sampledAt_ || now - sampledAt_ > 250) return false;
        return track_terrain_camera(camera, head_, tanX_, tanY_);
    }
};
