#pragma once
#include "types.h"
#include <cairo.h>
#include <memory>

class Shape {
public:
    ShapeType type = ShapeType::None;
    Color strokeColor{0, 0, 0, 1};
    Color fillColor{0, 0, 0, 0};
    double strokeWidth = 2.0;
    double opacity = 1.0;
    bool filled = false;

    Shape() = default;
    explicit Shape(ShapeType t) : type(t) {}
    virtual ~Shape() = default;

    virtual void render(cairo_t* cr) const = 0;
    virtual std::string serialize(int indent = 0) const;
    virtual std::string getMeasurements() const;
    virtual bool containsPoint(double x, double y) const = 0;
    virtual void moveBy(double dx, double dy) = 0;

    void applyStyle(cairo_t* cr) const;
    static std::unique_ptr<Shape> deserialize(const std::string& data);
};

class FreehandShape : public Shape {
public:
    std::vector<Point> points;
    BrushType brushType = BrushType::Pen;

    FreehandShape();
    void addPoint(double x, double y);
    void render(cairo_t* cr) const override;
    std::string serialize(int indent = 0) const override;
    std::string getMeasurements() const override;
    bool containsPoint(double x, double y) const override;
    void moveBy(double dx, double dy) override;
};

class LineShape : public Shape {
public:
    double x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    LineShape();
    void render(cairo_t* cr) const override;
    std::string serialize(int indent = 0) const override;
    std::string getMeasurements() const override;
    bool containsPoint(double x, double y) const override;
    void moveBy(double dx, double dy) override;
};

class RectShape : public Shape {
public:
    double x = 0, y = 0, w = 0, h = 0;
    RectShape();
    void render(cairo_t* cr) const override;
    std::string serialize(int indent = 0) const override;
    std::string getMeasurements() const override;
    bool containsPoint(double x, double y) const override;
    void moveBy(double dx, double dy) override;
};

class CircleShape : public Shape {
public:
    double cx = 0, cy = 0, rx = 0, ry = 0;
    CircleShape();
    void render(cairo_t* cr) const override;
    std::string serialize(int indent = 0) const override;
    std::string getMeasurements() const override;
    bool containsPoint(double x, double y) const override;
    void moveBy(double dx, double dy) override;
};

class ImageShape : public Shape {
public:
    double x = 0, y = 0;
    double scale = 1.0;
    cairo_surface_t* surface = nullptr;

    ImageShape();
    ~ImageShape() override;
    bool loadFromFile(const std::string& path);
    void render(cairo_t* cr) const override;
    std::string serialize(int indent = 0) const override;
    std::string getMeasurements() const override;
    bool containsPoint(double x, double y) const override;
    void moveBy(double dx, double dy) override;
    int getWidth() const;
    int getHeight() const;
};
