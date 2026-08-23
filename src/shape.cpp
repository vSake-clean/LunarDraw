#include "shape.h"
#include <cmath>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <cstring>

void Shape::applyStyle(cairo_t* cr) const {
    cairo_set_source_rgba(cr, strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a * opacity);
    cairo_set_line_width(cr, strokeWidth);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
}

std::string Shape::serialize(int indent) const {
    std::string pad(indent, ' ');
    std::ostringstream ss;
    ss << pad << "SHAPE_START\n";
    ss << pad << "TYPE=" << shapeTypeToString(type) << "\n";
    ss << pad << "STROKE=" << strokeColor.serialize() << "\n";
    ss << pad << "FILL=" << fillColor.serialize() << "\n";
    ss << pad << "STROKE_WIDTH=" << strokeWidth << "\n";
    ss << pad << "FILLED=" << (filled ? "1" : "0") << "\n";
    ss << pad << "OPACITY=" << opacity << "\n";
    return ss.str();
}

std::string Shape::getMeasurements() const { return ""; }

static std::string trimLine(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static void drawEllipse(cairo_t* cr, double cx, double cy, double rx, double ry) {
    cairo_save(cr);
    cairo_translate(cr, cx, cy);
    cairo_scale(cr, rx, ry);
    cairo_arc(cr, 0, 0, 1, 0, 2 * M_PI);
    cairo_restore(cr);
}

std::unique_ptr<Shape> Shape::deserialize(const std::string& data) {
    std::istringstream ss(data);
    std::string line;
    ShapeType type = ShapeType::None;

    while (std::getline(ss, line)) {
        std::string tl = trimLine(line);
        if (tl.find("TYPE=") == 0) {
            type = stringToShapeType(tl.substr(5));
            break;
        }
    }

    std::unique_ptr<Shape> shape;
    switch (type) {
        case ShapeType::Freehand: shape = std::make_unique<FreehandShape>(); break;
        case ShapeType::Line: shape = std::make_unique<LineShape>(); break;
        case ShapeType::Rectangle: shape = std::make_unique<RectShape>(); break;
        case ShapeType::Circle: shape = std::make_unique<CircleShape>(); break;
        case ShapeType::Image: shape = std::make_unique<ImageShape>(); break;
        default: return nullptr;
    }

    ss.clear();
    ss.seekg(0);
    while (std::getline(ss, line)) {
        std::string tl = trimLine(line);
        if (tl.empty() || tl == "SHAPE_START" || tl == "SHAPE_END") continue;
        if (tl.find("TYPE=") == 0) continue;
        if (tl.find("STROKE=") == 0) shape->strokeColor = Color::fromHex(tl.substr(7));
        else if (tl.find("FILL=") == 0) shape->fillColor = Color::fromHex(tl.substr(5));
        else if (tl.find("STROKE_WIDTH=") == 0) shape->strokeWidth = std::stod(tl.substr(13));
        else if (tl.find("FILLED=") == 0) shape->filled = (tl.substr(7) == "1");
        else if (tl.find("OPACITY=") == 0) shape->opacity = std::stod(tl.substr(8));
        else if (tl.find("BRUSH=") == 0) {
            if (auto* fh = dynamic_cast<FreehandShape*>(shape.get())) {
                int bv = std::stoi(tl.substr(6));
                if (bv >= 0 && bv <= 3) fh->brushType = static_cast<BrushType>(bv);
            }
        }
        else if (tl.find("X1=") == 0 && dynamic_cast<LineShape*>(shape.get()))
            dynamic_cast<LineShape*>(shape.get())->x1 = std::stod(tl.substr(3));
        else if (tl.find("Y1=") == 0 && dynamic_cast<LineShape*>(shape.get()))
            dynamic_cast<LineShape*>(shape.get())->y1 = std::stod(tl.substr(3));
        else if (tl.find("X2=") == 0 && dynamic_cast<LineShape*>(shape.get()))
            dynamic_cast<LineShape*>(shape.get())->x2 = std::stod(tl.substr(3));
        else if (tl.find("Y2=") == 0 && dynamic_cast<LineShape*>(shape.get()))
            dynamic_cast<LineShape*>(shape.get())->y2 = std::stod(tl.substr(3));
        else if (tl.find("X=") == 0) {
            if (auto* r = dynamic_cast<RectShape*>(shape.get())) r->x = std::stod(tl.substr(2));
            else if (auto* im = dynamic_cast<ImageShape*>(shape.get())) im->x = std::stod(tl.substr(2));
        }
        else if (tl.find("Y=") == 0) {
            if (auto* r = dynamic_cast<RectShape*>(shape.get())) r->y = std::stod(tl.substr(2));
            else if (auto* im = dynamic_cast<ImageShape*>(shape.get())) im->y = std::stod(tl.substr(2));
        }
        else if (tl.find("W=") == 0 && dynamic_cast<RectShape*>(shape.get()))
            dynamic_cast<RectShape*>(shape.get())->w = std::stod(tl.substr(2));
        else if (tl.find("H=") == 0 && dynamic_cast<RectShape*>(shape.get()))
            dynamic_cast<RectShape*>(shape.get())->h = std::stod(tl.substr(2));
        else if (tl.find("CX=") == 0 && dynamic_cast<CircleShape*>(shape.get()))
            dynamic_cast<CircleShape*>(shape.get())->cx = std::stod(tl.substr(3));
        else if (tl.find("CY=") == 0 && dynamic_cast<CircleShape*>(shape.get()))
            dynamic_cast<CircleShape*>(shape.get())->cy = std::stod(tl.substr(3));
        else if (tl.find("RX=") == 0 && dynamic_cast<CircleShape*>(shape.get()))
            dynamic_cast<CircleShape*>(shape.get())->rx = std::stod(tl.substr(3));
        else if (tl.find("RY=") == 0 && dynamic_cast<CircleShape*>(shape.get()))
            dynamic_cast<CircleShape*>(shape.get())->ry = std::stod(tl.substr(3));
        else if (tl.find("POINTS=") == 0 && dynamic_cast<FreehandShape*>(shape.get())) {
            auto* fh = dynamic_cast<FreehandShape*>(shape.get());
            int n = std::stoi(tl.substr(7));
            for (int i = 0; i < n; i++) {
                std::getline(ss, line);
                std::istringstream pts(trimLine(line));
                double px, py;
                if (pts >> px >> py) fh->points.emplace_back(px, py);
            }
        }
        else if (tl.find("SCALE=") == 0 && dynamic_cast<ImageShape*>(shape.get()))
            dynamic_cast<ImageShape*>(shape.get())->scale = std::stod(tl.substr(6));
        else if (tl.find("PATH=") == 0 && dynamic_cast<ImageShape*>(shape.get()))
            dynamic_cast<ImageShape*>(shape.get())->loadFromFile(tl.substr(5));
    }
    return shape;
}

FreehandShape::FreehandShape() : Shape(ShapeType::Freehand) {}

void FreehandShape::addPoint(double x, double y) {
    points.emplace_back(x, y);
}

void FreehandShape::render(cairo_t* cr) const {
    if (points.size() < 2) return;

    if (brushType == BrushType::Eraser) {
        cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
        cairo_set_source_rgba(cr, 0, 0, 0, 1);
        cairo_set_line_width(cr, strokeWidth * 3);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        cairo_move_to(cr, points[0].x, points[0].y);
        for (size_t i = 1; i < points.size(); i++)
            cairo_line_to(cr, points[i].x, points[i].y);
        cairo_stroke(cr);
        cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
        return;
    }

    if (brushType == BrushType::Spray) {
        cairo_set_source_rgba(cr, strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a * opacity * 0.6);
        double radius = strokeWidth * 2;
        for (const auto& p : points) {
            for (int i = 0; i < 8; i++) {
                double angle = (double)rand() / RAND_MAX * 2 * M_PI;
                double dist = (double)rand() / RAND_MAX * radius;
                double sx = p.x + cos(angle) * dist;
                double sy = p.y + sin(angle) * dist;
                cairo_arc(cr, sx, sy, strokeWidth * 0.3, 0, 2 * M_PI);
                cairo_fill(cr);
            }
        }
        return;
    }

    applyStyle(cr);
    double alpha = strokeColor.a * opacity;
    if (brushType == BrushType::Marker) alpha *= 0.5;
    cairo_set_source_rgba(cr, strokeColor.r, strokeColor.g, strokeColor.b, alpha);

    cairo_move_to(cr, points[0].x, points[0].y);
    for (size_t i = 1; i < points.size(); i++)
        cairo_line_to(cr, points[i].x, points[i].y);
    cairo_stroke(cr);
}

std::string FreehandShape::serialize(int indent) const {
    std::string pad(indent, ' ');
    std::ostringstream ss;
    ss << Shape::serialize(indent);
    ss << pad << "BRUSH=" << (int)brushType << "\n";
    ss << pad << "POINTS=" << points.size() << "\n";
    for (const auto& p : points)
        ss << pad << p.x << " " << p.y << "\n";
    ss << pad << "SHAPE_END\n";
    return ss.str();
}

std::string FreehandShape::getMeasurements() const {
    if (points.empty()) return "Freehand: no points";
    double minX = points[0].x, minY = points[0].y;
    double maxX = points[0].x, maxY = points[0].y;
    for (const auto& p : points) {
        minX = std::min(minX, p.x); maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y); maxY = std::max(maxY, p.y);
    }
    std::ostringstream ss;
    ss << brushTypeToString(brushType) << ": " << points.size() << " pts, "
       << (maxX - minX) << "x" << (maxY - minY);
    return ss.str();
}

bool FreehandShape::containsPoint(double x, double y) const {
    if (points.size() < 2) return false;
    for (size_t i = 1; i < points.size(); i++) {
        double dx = points[i].x - points[i-1].x;
        double dy = points[i].y - points[i-1].y;
        double len = std::sqrt(dx*dx + dy*dy);
        if (len < 0.01) continue;
        double t = std::max(0.0, std::min(1.0, ((x - points[i-1].x)*dx + (y - points[i-1].y)*dy) / (len*len)));
        double px = points[i-1].x + t * dx;
        double py = points[i-1].y + t * dy;
        double dist = std::sqrt((x - px)*(x - px) + (y - py)*(y - py));
        if (dist < std::max(5.0, strokeWidth)) return true;
    }
    return false;
}

void FreehandShape::moveBy(double dx, double dy) {
    for (auto& p : points) { p.x += dx; p.y += dy; }
}

LineShape::LineShape() : Shape(ShapeType::Line) {}

void LineShape::render(cairo_t* cr) const {
    applyStyle(cr);
    cairo_move_to(cr, x1, y1);
    cairo_line_to(cr, x2, y2);
    cairo_stroke(cr);
}

std::string LineShape::serialize(int indent) const {
    std::string pad(indent, ' ');
    std::ostringstream ss;
    ss << Shape::serialize(indent);
    ss << pad << "X1=" << x1 << "\n" << pad << "Y1=" << y1 << "\n";
    ss << pad << "X2=" << x2 << "\n" << pad << "Y2=" << y2 << "\n";
    ss << pad << "SHAPE_END\n";
    return ss.str();
}

std::string LineShape::getMeasurements() const {
    double dx = x2 - x1, dy = y2 - y1;
    double len = std::sqrt(dx*dx + dy*dy);
    double angle = std::atan2(dy, dx) * 180.0 / M_PI;
    std::ostringstream ss;
    ss << "Line: len=" << std::fixed << std::setprecision(1) << len
       << " angle=" << std::fixed << std::setprecision(1) << angle << "deg";
    return ss.str();
}

bool LineShape::containsPoint(double x, double y) const {
    double dx = x2 - x1, dy = y2 - y1;
    double len = std::sqrt(dx*dx + dy*dy);
    if (len < 0.01) return std::sqrt((x-x1)*(x-x1)+(y-y1)*(y-y1)) < strokeWidth;
    double t = std::max(0.0, std::min(1.0, ((x-x1)*dx + (y-y1)*dy) / (len*len)));
    double px = x1 + t*dx, py = y1 + t*dy;
    return std::sqrt((x-px)*(x-px)+(y-py)*(y-py)) < std::max(5.0, strokeWidth);
}

void LineShape::moveBy(double dx, double dy) { x1+=dx; y1+=dy; x2+=dx; y2+=dy; }

RectShape::RectShape() : Shape(ShapeType::Rectangle) {}

void RectShape::render(cairo_t* cr) const {
    applyStyle(cr);
    if (filled && fillColor.a > 0) {
        cairo_set_source_rgba(cr, fillColor.r, fillColor.g, fillColor.b, fillColor.a * opacity);
        cairo_rectangle(cr, x, y, w, h);
        cairo_fill_preserve(cr);
        applyStyle(cr);
    }
    cairo_rectangle(cr, x, y, w, h);
    cairo_stroke(cr);
}

std::string RectShape::serialize(int indent) const {
    std::string pad(indent, ' ');
    std::ostringstream ss;
    ss << Shape::serialize(indent);
    ss << pad << "X=" << x << "\n" << pad << "Y=" << y << "\n";
    ss << pad << "W=" << w << "\n" << pad << "H=" << h << "\n";
    ss << pad << "SHAPE_END\n";
    return ss.str();
}

std::string RectShape::getMeasurements() const {
    double area = std::abs(w * h);
    std::ostringstream ss;
    ss << "Rect: " << std::fixed << std::setprecision(1) << std::abs(w) << "x" << std::abs(h)
       << " area=" << area;
    return ss.str();
}

bool RectShape::containsPoint(double xp, double yp) const {
    double minX = std::min(x, x+w), maxX = std::max(x, x+w);
    double minY = std::min(y, y+h), maxY = std::max(y, y+h);
    return xp >= minX - strokeWidth && xp <= maxX + strokeWidth &&
           yp >= minY - strokeWidth && yp <= maxY + strokeWidth;
}

void RectShape::moveBy(double dx, double dy) { x+=dx; y+=dy; }

CircleShape::CircleShape() : Shape(ShapeType::Circle) {}

void CircleShape::render(cairo_t* cr) const {
    applyStyle(cr);
    if (filled && fillColor.a > 0) {
        cairo_set_source_rgba(cr, fillColor.r, fillColor.g, fillColor.b, fillColor.a * opacity);
        drawEllipse(cr, cx, cy, rx, ry);
        cairo_fill_preserve(cr);
        applyStyle(cr);
    }
    drawEllipse(cr, cx, cy, rx, ry);
    cairo_stroke(cr);
}

std::string CircleShape::serialize(int indent) const {
    std::string pad(indent, ' ');
    std::ostringstream ss;
    ss << Shape::serialize(indent);
    ss << pad << "CX=" << cx << "\n" << pad << "CY=" << cy << "\n";
    ss << pad << "RX=" << rx << "\n" << pad << "RY=" << ry << "\n";
    ss << pad << "SHAPE_END\n";
    return ss.str();
}

std::string CircleShape::getMeasurements() const {
    std::ostringstream ss;
    if (std::abs(rx - ry) < 0.01)
        ss << "Circle: R=" << std::fixed << std::setprecision(1) << rx
           << " D=" << (rx*2) << " area=" << (M_PI * rx * rx);
    else
        ss << "Ellipse: rx=" << std::fixed << std::setprecision(1) << rx
           << " ry=" << ry << " area=" << (M_PI * rx * ry);
    return ss.str();
}

bool CircleShape::containsPoint(double xp, double yp) const {
    double dx = (xp - cx) / rx, dy = (yp - cy) / ry;
    return (dx*dx + dy*dy) <= 1.0 + (strokeWidth / std::min(rx, ry));
}

void CircleShape::moveBy(double dx, double dy) { cx+=dx; cy+=dy; }

ImageShape::ImageShape() : Shape(ShapeType::Image) { filled = false; }
ImageShape::~ImageShape() { if (surface) cairo_surface_destroy(surface); }

bool ImageShape::loadFromFile(const std::string& path) {
    if (surface) { cairo_surface_destroy(surface); surface = nullptr; }
    surface = cairo_image_surface_create_from_png(path.c_str());
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(surface);
        surface = nullptr;
        return false;
    }
    return true;
}

int ImageShape::getWidth() const {
    return surface ? cairo_image_surface_get_width(surface) : 0;
}

int ImageShape::getHeight() const {
    return surface ? cairo_image_surface_get_height(surface) : 0;
}

void ImageShape::render(cairo_t* cr) const {
    if (!surface) return;
    cairo_save(cr);
    cairo_set_source_surface(cr, surface, x, y);
    if (opacity < 1.0) cairo_paint_with_alpha(cr, opacity);
    else cairo_paint(cr);
    cairo_restore(cr);
}

std::string ImageShape::serialize(int indent) const {
    std::string pad(indent, ' ');
    std::ostringstream ss;
    ss << Shape::serialize(indent);
    ss << pad << "X=" << x << "\n" << pad << "Y=" << y << "\n";
    ss << pad << "SCALE=" << scale << "\n";
    ss << pad << "SHAPE_END\n";
    return ss.str();
}

std::string ImageShape::getMeasurements() const {
    if (!surface) return "Image: none";
    std::ostringstream ss;
    ss << "Image: " << getWidth() << "x" << getHeight() << " at (" << x << "," << y << ")";
    return ss.str();
}

bool ImageShape::containsPoint(double xp, double yp) const {
    if (!surface) return false;
    return xp >= x && xp <= x + getWidth() * scale &&
           yp >= y && yp <= y + getHeight() * scale;
}

void ImageShape::moveBy(double dx, double dy) { x+=dx; y+=dy; }
