#include "camera.h"
#include "mapprojections.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "util.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/matrix.hpp>
#include <iostream>

Camera::Camera(glm::vec3 position, glm::vec3 up, float zNear, float zFar, float aspectRatio, float yaw, float pitch)
    : _front(glm::vec3(0.0f, 0.0f, -1.0f))
    , _movementSpeed(SPEED)
{
    _zNear = zNear;
    _zFar = zFar;
    _aspectRatio = aspectRatio;
    _position = position;
    _worldUp = up;
    _yaw = yaw;
    _pitch = pitch;
    _zoom = ZOOM;

    updateCameraVectors();
    updateFrustum();
}

glm::vec3 Camera::front()
{
    return _front;
}

glm::vec3 Camera::right()
{
    return _right;
}

glm::vec3 Camera::up()
{
    return _up;
}

void Camera::aspectRatio(float aspectRatio)
{
    _aspectRatio = aspectRatio;
}

bool Camera::insideViewFrustum(glm::vec3 p1, glm::vec3 p2)
{
    Frustum frustum = _viewFrustum;

    unsigned checked = 0;

    checked += checkPlane(frustum.leftFace, p1, p2);
    checked += checkPlane(frustum.rightFace, p1, p2);
    checked += checkPlane(frustum.topFace, p1, p2);
    checked += checkPlane(frustum.bottomFace, p1, p2);
    checked += checkPlane(frustum.nearFace, p1, p2);
    checked += checkPlane(frustum.farFace, p1, p2);

    /* This is a temporary hack because some AABBs are wrongly detected
     * as outside of the frustum, which should be investigated in the
     * nearer future */
    return checked >= 5;
}

bool Camera::checkPlane(Plane& plane, glm::vec3 p1, glm::vec3 p2)
{
    float minY = p1.y;
    float maxY = p2.y;
    float width = p2.x - p1.x;
    glm::vec3 aabbCenter = p1 + ((p2 - p1) / 2.0f);

    float halfHeight = (maxY - minY) / 2.0f;
    float halfBlockSize = width / 2.0f;
    float r = halfBlockSize * std::abs(plane.normal.x)
        + halfHeight * std::abs(plane.normal.y)
        + halfBlockSize * std::abs(plane.normal.z);

    return -r <= plane.getSignedDistanceToPlane(aabbCenter);
}

glm::mat4 Camera::getViewMatrix()
{
    return glm::lookAt(_position, _position + _front, _up);
}

float Camera::zoom()
{
    return _zoom;
}

void Camera::zoom(float zoom)
{
    _zoom = zoom;
}

glm::vec3 Camera::position()
{
    return _position;
}

void Camera::yaw(float yaw)
{
    _yaw = yaw;
}

void Camera::pitch(float pitch)
{
    _pitch = pitch;
}

float Camera::pitch()
{
    return _pitch;
}

float Camera::yaw()
{
    return _yaw;
}

void Camera::processKeyboard(CameraAction direction, float deltaTime)
{
    float dist = glm::length(_position);
    float damp = (glm::clamp(dist, 317.0f, 450.0f) - 316.95f) / 133.0f;

    float velocity = _movementSpeed * deltaTime * damp;

    glm::vec3 oldPosition = _position;

    glm::vec3 r = glm::vec3(100000, 100000, 100000);

    /* Spherical traversal */
    glm::vec3 earthNormal = glm::normalize(_position);
    glm::vec3 projectedFront = _front - glm::dot(_front, earthNormal) / glm::dot(earthNormal, earthNormal) * earthNormal;
    projectedFront = glm::normalize(projectedFront);

    glm::vec3 collisionCorrection = earthNormal * verticalCollisionOffset;

    if (direction == CameraAction::SPEED_UP)
        _movementSpeed = SPEED * SPEED_UP_MULT;
    else
        _movementSpeed = SPEED;

    _position += collisionCorrection;

    if (direction == CameraAction::MOVE_FORWARD) {
        _position = _position + projectedFront * velocity;
    }
    if (direction == CameraAction::MOVE_BACKWARD) {
        _position = _position - projectedFront * velocity;
    }
    if (direction == CameraAction::MOVE_LEFT) {
        _position -= _right * velocity;
    }
    if (direction == CameraAction::MOVE_RIGHT) {
        _position += _right * velocity;
    }
    if (direction == CameraAction::MOVE_UP)
        _position += earthNormal * velocity;
    if (direction == CameraAction::MOVE_DOWN)
        _position -= earthNormal * velocity;
    if (direction == CameraAction::LOOK_UP)
        _pitch += 1 * 0.5;
    if (direction == CameraAction::LOOK_DOWN)
        _pitch -= 1 * 0.5;
    if (direction == CameraAction::LOOK_LEFT)
        _yaw += 1 * 0.5;
    if (direction == CameraAction::LOOK_RIGHT)
        _yaw -= 1 * 0.5;

    /* Limit the positions */
    if (glm::length(_position) >= 1000)
        _position = oldPosition;

    /* Cannot go over the poles */
    if (glm::abs(_position.x) <= 30 && glm::abs(_position.z) <= 30)
        _position = oldPosition;

    _pitch = glm::clamp(_pitch, -89.0f, 0.0f);

    if (_yaw >= 360.0f || _yaw <= -360.0f)
        _yaw = 0;

    updateCameraVectors();
    updateFrustum();
}

void Camera::processMouseMovement(float xOffset, float yOffset)
{
    _yaw += xOffset * 0.5;
    _pitch += yOffset * 0.5;

    _pitch = glm::clamp(_pitch, -89.0f, 0.0f);

    if (_yaw >= 360.0f || _yaw <= -360.0f)
        _yaw = 0;

    updateCameraVectors();
    updateFrustum();
}

Frustum Camera::viewFrustum()
{
    return _viewFrustum;
}

void Camera::updateFrustum()
{
    const float halfVSide = _zFar * tanf(glm::radians(_zoom) * 0.5f);
    const float halfHSide = halfVSide * _aspectRatio;
    const glm::vec3 frontMultFar = _zFar * _front;

    _viewFrustum.nearFace = { _position + _zNear * _front, _front };
    _viewFrustum.farFace = { _position + frontMultFar, -_front };

    _viewFrustum.rightFace = { _position,
        glm::cross(frontMultFar - _right * halfHSide, _up) };

    _viewFrustum.leftFace = { _position,
        glm::cross(_up, frontMultFar + _right * halfHSide) };

    _viewFrustum.topFace = { _position,
        glm::cross(_right, frontMultFar - _up * halfVSide) };

    _viewFrustum.bottomFace = { _position,
        glm::cross(frontMultFar + _up * halfVSide, _right) };
}

void Camera::updateCameraVectors()
{

    /* Earth-based camera vectors */
    glm::vec3 earthNormal = glm::normalize(_position);

    glm::vec3 earthRight = glm::normalize(glm::cross(earthNormal, _worldUp));
    glm::vec3 earthFront = -1.0f * glm::normalize(glm::cross(earthNormal, earthRight));
    glm::vec3 front1;

    front1 = glm::vec3(0, 0, -1.0);

    glm::mat4 earthSystem = glm::mat4(glm::vec4(earthRight, 0),
        glm::vec4(earthNormal, 0),
        glm::vec4(earthFront, 0),
        glm::vec4(0, 0, 0, 1));

    glm::quat pitchQuat = glm::angleAxis(glm::radians(_pitch), earthRight);
    glm::quat yawQuat = glm::angleAxis(glm::radians(_yaw), earthNormal);
    glm::quat earthQuat = glm::quat_cast(earthSystem);

    front1 = glm::vec3(yawQuat * pitchQuat * earthQuat * front1);

    _front = glm::normalize(front1);
    _right = glm::normalize(glm::cross(_front, earthNormal));
    _up = glm::normalize(glm::cross(_right, _front));
}
