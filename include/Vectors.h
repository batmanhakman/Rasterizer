#pragma once
#include <cmath>
#include <math.h>

struct Vector2D
{
    float x;
    float y;
};

struct Vector3D
{
    float x;
    float y;
    float z;
};


class Vectors
{
    public:
    static Vector3D Scalar(const Vector3D& vector, float scalar);
    static Vector3D Negative(const Vector3D& vector);
    static Vector3D Addition(const Vector3D& vector, const Vector3D& vector2);
    static Vector3D Subtraction(const Vector3D& vector, const Vector3D& vector2);
    static float length(const Vector2D& vector);
    static float length(const Vector3D& vector);
    static float DotProduct(const Vector3D& vector, const Vector3D& vector2);
    static Vector3D CrossProduct(const Vector3D& vector, const Vector3D& vector2);
};