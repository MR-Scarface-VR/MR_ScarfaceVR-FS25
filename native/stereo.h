#pragma once
#include <cmath>

struct Vector3 {
    float x, y, z;
};

struct Quaternion { float x, y, z, w; };
struct Pose { Quaternion orientation; Vector3 position; };

inline Quaternion conjugate(Quaternion q) { return {-q.x, -q.y, -q.z, q.w}; }
inline Quaternion multiply(Quaternion a, Quaternion b) {
    return {a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
            a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
            a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w,
            a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z};
}
inline Vector3 rotate(Quaternion q, Vector3 v) {
    const auto r = multiply(multiply(q, {v.x, v.y, v.z, 0}), conjugate(q));
    return {r.x, r.y, r.z};
}
inline bool valid_pose(Pose p) {
    const auto q = p.orientation;
    const float norm = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
    return std::isfinite(p.position.x) && std::isfinite(p.position.y) &&
           std::isfinite(p.position.z) && std::isfinite(norm) && std::abs(norm - 1) < 0.001f;
}
inline Pose relative_pose(Pose origin, Pose pose) {
    const auto inverse = conjugate(origin.orientation);
    return {multiply(inverse, pose.orientation), rotate(inverse,
        {pose.position.x-origin.position.x, pose.position.y-origin.position.y,
         pose.position.z-origin.position.z})};
}
inline Pose recenter_origin(Pose head) {
    const auto forward = rotate(head.orientation, {0, 0, -1});
    const float halfYaw = std::atan2(-forward.x, -forward.z) * 0.5f;
    return {{0, std::sin(halfYaw), 0, std::cos(halfYaw)}, head.position};
}
inline void rotation_matrix(Quaternion q, float* m) {
    // GIANTS consumes column-major transforms, confirmed at its native multiply boundary.
    const auto x = rotate(q, {1, 0, 0});
    const auto y = rotate(q, {0, 1, 0});
    const auto z = rotate(q, {0, 0, 1});
    const float values[16] = {x.x,x.y,x.z,0, y.x,y.y,y.z,0, z.x,z.y,z.z,0, 0,0,0,1};
    for (unsigned i = 0; i < 16; ++i) m[i] = values[i];
}

inline Vector3 diagnostic_eye_offset(unsigned int eye) {
    // shortcut: fixed 64 mm for the desktop probe; use runtime eye poses before VR release.
    return {eye == 0 ? -0.032f : eye == 1 ? 0.032f : 0.0f, 0.0f, 0.0f};
}
