#include "./la.h"

Vec2f vec2f(float x, float y) {
    return (Vec2f) {
        .x = x,
        .y = y
    };
}

Vec2f vec2fs(float a) {
    return (Vec2f) {
        .x = a,
        .y = a
    };
}

Vec2f vec2f_add(Vec2f a, Vec2f b) {
    return (Vec2f) {
        .x = a.x + b.x,
        .y = a.y + b.y
    };
}

Vec2f vec2f_sub(Vec2f a, Vec2f b) {
    return (Vec2f) {
        .x = a.x - b.x,
        .y = a.y - b.y
    };
}

Vec2f vec2f_mul(Vec2f a, Vec2f b) {
    return (Vec2f) {
        .x = a.x * b.x,
        .y = a.y * b.y
    };
}

Vec2f vec2f_div(Vec2f a, Vec2f b) {
    return (Vec2f) {
        .x = a.x / b.x,
        .y = a.y / b.y
    };
}

