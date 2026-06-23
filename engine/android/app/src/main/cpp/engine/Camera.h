#pragma once
#include "../math/Mat4.h"
#include "../math/Vec3.h"
#include <cmath>

enum class CameraMode { ThirdPerson, FirstPerson };

class Camera {
public:
    CameraMode mode    = CameraMode::ThirdPerson;

    // Third-person orbit
    Vec3  target       = {0,1,0};
    float yaw          = 0.f;      // horizontal rotation (radians)
    float pitch        = 0.35f;   // vertical tilt
    float distance     = 5.f;     // orbit radius
    float minPitch     = -1.0f;
    float maxPitch     = 1.4f;
    float minDist      = 1.5f;
    float maxDist      = 15.f;

    // First-person
    Vec3  fpPosition   = {};
    float fpYaw        = 0.f;
    float fpPitch      = 0.f;

    float fovY         = 1.047f;   // 60 degrees
    float aspect       = 1.7778f;
    float zNear        = 0.1f;
    float zFar         = 500.f;

    void toggleMode();
    void orbit(float dYaw, float dPitch);
    void zoom(float delta);
    void setFirstPersonPose(const Vec3& pos, float yaw, float pitch);

    Mat4 view()       const;
    Mat4 projection() const;
    Vec3 position()   const;
    Vec3 forward()    const;
    Vec3 right()      const;
};
