#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <cmath>
#include <sstream>
#include <iomanip>

struct Color {
    double r = 0, g = 0, b = 0, a = 1.0;

    Color() = default;
    Color(double r, double g, double b, double a = 1.0) : r(r), g(g), b(b), a(a) {}

    std::string toHex() const {
        std::ostringstream ss;
        ss << "#" << std::hex << std::setfill('0')
           << std::setw(2) << (int)(r * 255)
           << std::setw(2) << (int)(g * 255)
           << std::setw(2) << (int)(b * 255)
           << std::setw(2) << (int)(a * 255);
        return ss.str();
    }

    static Color fromHex(const std::string& hex) {
        Color c;
        if (hex.size() >= 9 && hex[0] == '#') {
            c.r = std::stoi(hex.substr(1, 2), nullptr, 16) / 255.0;
            c.g = std::stoi(hex.substr(3, 2), nullptr, 16) / 255.0;
            c.b = std::stoi(hex.substr(5, 2), nullptr, 16) / 255.0;
            c.a = std::stoi(hex.substr(7, 2), nullptr, 16) / 255.0;
        } else if (hex.size() >= 7 && hex[0] == '#') {
            c.r = std::stoi(hex.substr(1, 2), nullptr, 16) / 255.0;
            c.g = std::stoi(hex.substr(3, 2), nullptr, 16) / 255.0;
            c.b = std::stoi(hex.substr(5, 2), nullptr, 16) / 255.0;
        }
        return c;
    }

    std::string serialize() const { return toHex(); }
};

enum class ShapeType {
    Freehand,
    Line,
    Rectangle,
    Circle,
    Image,
    None
};

enum class BrushType {
    Pen,
    Marker,
    Spray,
    Eraser
};

struct Point {
    double x = 0, y = 0;
    Point() = default;
    Point(double x, double y) : x(x), y(y) {}
};

static std::string shapeTypeToString(ShapeType t) {
    switch (t) {
        case ShapeType::Freehand: return "freehand";
        case ShapeType::Line: return "line";
        case ShapeType::Rectangle: return "rectangle";
        case ShapeType::Circle: return "circle";
        case ShapeType::Image: return "image";
        default: return "none";
    }
}

static ShapeType stringToShapeType(const std::string& s) {
    if (s == "freehand") return ShapeType::Freehand;
    if (s == "line") return ShapeType::Line;
    if (s == "rectangle") return ShapeType::Rectangle;
    if (s == "circle") return ShapeType::Circle;
    if (s == "image") return ShapeType::Image;
    return ShapeType::None;
}

static const char* brushTypeToString(BrushType bt) {
    switch (bt) {
        case BrushType::Pen: return "Pen";
        case BrushType::Marker: return "Marker";
        case BrushType::Spray: return "Spray";
        case BrushType::Eraser: return "Eraser";
        default: return "Pen";
    }
}
