#include "Camera.h"
#include <algorithm>

void Camera::toggleMode() {
    mode = (mode == CameraMode::ThirdPerson)
         ? CameraMode::FirstPerson
         : CameraMode::ThirdPerson;
}

void Camera::orbit(float dYaw, float dPitch) {
    yaw   += dYaw;
    pitch  = std::clamp(pitch + dPitch, minPitch, maxPitch);
}

void Camera::zoom(float delta) {
    distance = std::clamp(distance - delta, minDist, maxDist);
}

void Camera::setFirstPersonPose(const Vec3& pos, float y, float p) {
    fpPosition = pos;
    fpYaw   = y;
    fpPitch = std::clamp(p, -1.3f, 1.3f);
}

Vec3 Camera::position() const {
    if(mode == CameraMode::FirstPerson) return fpPosition;
    // Third-person: orbit around target
    float cp = std::cos(pitch), sp = std::sin(pitch);
    float cy = std::cos(yaw),   sy = std::sin(yaw);
    return target + Vec3{ cy*cp*distance, sp*distance, sy*cp*distance };
}

Vec3 Camera::forward() const {
    if(mode == CameraMode::FirstPerson) {
        float cp = std::cos(fpPitch), sp = std::sin(fpPitch);
        float cy = std::cos(fpYaw),   sy = std::sin(fpYaw);
        return Vec3{cy*cp, sp, sy*cp}.normalized();
    }
    return (target - position()).normalized();
}

Vec3 Camera::right() const {
    return forward().cross(Vec3::up()).normalized();
}

Mat4 Camera::view() const {
    if(mode == CameraMode::FirstPerson) {
        return Mat4::LookAt(fpPosition, fpPosition + forward(), Vec3::up());
    }
    Vec3 pos = position();
    return Mat4::LookAt(pos, target, Vec3::up());
}

Mat4 Camera::projection() const {
    return Mat4::Perspective(fovY, aspect, zNear, zFar);
}
