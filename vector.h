#ifndef VECTOR_H
#include <cmath>

#define PI 3.14159265
static float DEG_TO_RAD(float deg) {
    return deg * 180.0 / PI;
}

struct Vector2D {
    float x, y;
    Vector2D(float x_ = 0.0, float y_ = 0.0) {
        x = x_;
        y = y_;
    }
    float SqrMagnitude() {
        return x * x + y * y;
    }
    float Magnitude() {
        return sqrt(SqrMagnitude());
    }
    Vector2D Normalized() {
        float mag = Magnitude();
        Vector2D norm {x / mag, y / mag};
        return norm;
    }
    /**
     * Operators
     */
    Vector2D operator+(const Vector2D& b) {
        Vector2D sum {x + b.x, y + b.y};
        return sum;
    }
    void operator+=(const Vector2D& b) {
        x += b.x, 
        y += b.y;
    }
    Vector2D operator-(const Vector2D& b) {
        Vector2D sum {x - b.x, y - b.y};
        return sum;
    }
    void operator-=(const Vector2D& b) {
        x -= b.x, 
        y -= b.y;
    }
    Vector2D operator*(const Vector2D& b) {
        return Dot(*this, b);
    }
    Vector2D operator*(const float& m) {
        Vector2D mul {x * m, y * m};
        return mul;
    }
    Vector2D operator/(const float& d) {
        Vector2D div {x / d, y / d};
        return div;
    }
    Vector2D operator-() const {
        Vector2D neg { -x, -y };
        return neg;
    }
    /**
     * Static functions
     */
    static float Dot(Vector2D u, Vector2D v) {
        return u.x * v.x + u.y * v.y;
    }
};

const static Vector2D v_zero { 0.0, 0.0 };
const static Vector2D v_up { 0.0, 1.0 };
const static Vector2D v_right { 1.0, 0.0 };

#endif