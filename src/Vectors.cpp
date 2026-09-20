#include "Vectors.h"


Vector3D Vectors::Scalar(const Vector3D& vector, float scalar)
{
    Vector3D newVector;

    newVector.x = vector.x * scalar;
    newVector.y = vector.y * scalar;
    newVector.z = vector.z * scalar;

    return newVector;
}

Vector3D Vectors::Negative(const Vector3D& vector)
{
    Vector3D newVector;

    newVector.x = vector.x * -1;
    newVector.y = vector.y * -1;
    newVector.z = vector.z * -1;
    

    return newVector;
}

Vector3D Vectors::Addition(const Vector3D& vector, const Vector3D& vector2)
{
    Vector3D newVector;
    
    newVector.x = vector.x + vector2.x;
    newVector.y = vector.y + vector2.y;
    newVector.z = vector.z + vector2.z;

    return newVector;
}


Vector3D Vectors::Subtraction(const Vector3D& vector, const Vector3D& vector2)
{
    Vector3D newVector;
    
    newVector.x = vector.x - vector2.x;
    newVector.y = vector.y - vector2.y;
    newVector.z = vector.z - vector2.z;

    return newVector;
}


float Vectors::length(const Vector3D& vector)
{
    float length = std::sqrt((vector.x*vector.x) + (vector.y*vector.y) + (vector.z * vector.z));

    return length;
}

float Vectors::DotProduct(const Vector3D& vector, const Vector3D& vector2)
{
    float dotProductResult((vector.x * vector2.x) + (vector.y * vector2.y) + (vector.z * vector2.z));

    return dotProductResult;
}

Vector3D Vectors::CrossProduct(const Vector3D& vector, const Vector3D& vector2)
{
    Vector3D newVector;

    newVector.x = ((vector.y * vector2.z) - (vector.z * vector2.y));
    newVector.y = ((vector.z*vector2.x) - (vector.x * vector2.z));
    newVector.z = ((vector.x * vector2.y) - (vector.y * vector2.x));


    return newVector;
}