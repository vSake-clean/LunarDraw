#include "canvas.h"
#include <cmath>
#include <cstring>
#include <cstdlib>

Canvas::Canvas(Project* proj) : project(proj) {}
Canvas::~Canvas() {}

void Canvas::setTool(ShapeType tool) { currentTool = tool; selectNone(); }
void Canvas::setBrush(BrushType bt) { currentBrush = bt; }

void Canvas::selectNone() {
    selectedShape = nullptr;
    selectedLayerIdx = -1;
    selectedShapeIdx = -1;
    if (measureLabel) gtk_label_set_text(GTK_LABEL(measureLabel), "");
}

Shape* Canvas::hitTest(double x, double y) {
    for (int li = (int)project->layers.size() - 1; li >= 0; li--) {
        auto& layer = project->layers[li];
        if (!layer->visible) continue;
        for (int si = (int)layer->shapes.size() - 1; si >= 0; si--) {
            if (layer->shapes[si]->containsPoint(x, y)) {
                selectedLayerIdx = li;
                selectedShapeIdx = si;
                return layer->shapes[si].get();
            }
        }
    }
    return nullptr;
}

void Canvas::deleteSelected() {
    if (selectedLayerIdx >= 0 && selectedLayerIdx < (int)project->layers.size() &&
        selectedShapeIdx >= 0 && selectedShapeIdx < (int)project->layers[selectedLayerIdx]->shapes.size()) {
        project->saveUndoState();
        project->layers[selectedLayerIdx]->shapes.erase(
            project->layers[selectedLayerIdx]->shapes.begin() + selectedShapeIdx);
        selectNone();
        redraw();
    }
}

GtkWidget* Canvas::create() {
    widget = gtk_drawing_area_new();
    gtk_widget_set_size_request(widget, project->width, project->height);
    gtk_widget_set_events(widget, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                                   GDK_POINTER_MOTION_MASK | GDK_KEY_PRESS_MASK |
                                   GDK_KEY_RELEASE_MASK | GDK_SCROLL_MASK);
    gtk_widget_set_can_focus(widget, TRUE);

    g_signal_connect(widget, "draw", G_CALLBACK(onDraw), this);
    g_signal_connect(widget, "button-press-event", G_CALLBACK(onPress), this);
    g_signal_connect(widget, "button-release-event", G_CALLBACK(onRelease), this);
    g_signal_connect(widget, "motion-notify-event", G_CALLBACK(onMove), this);
    g_signal_connect(widget, "key-press-event", G_CALLBACK(onKeyPress), this);
    g_signal_connect(widget, "key-release-event", G_CALLBACK(onKeyRelease), this);

    return widget;
}

void Canvas::redraw() { if (widget) gtk_widget_queue_draw(widget); }

gboolean Canvas::onDraw(GtkWidget* w, cairo_t* cr, gpointer data) {
    auto* canvas = static_cast<Canvas*>(data);

    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
    for (int x = 0; x < canvas->project->width; x += 20) {
        for (int y = 0; y < canvas->project->height; y += 20) {
            if ((x / 20 + y / 20) % 2 == 0) {
                cairo_rectangle(cr, x, y, 20, 20);
                cairo_fill(cr);
            }
        }
    }

    for (const auto& layer : canvas->project->layers)
        layer->render(cr);

    if (canvas->drawing) canvas->renderPreview(cr);
    canvas->renderSelection(cr);

    double size = canvas->strokeWidth;
    double px = 20 + size, py = canvas->project->height - 20 - size;
    cairo_set_source_rgba(cr, 0.3, 0.3, 0.3, 0.5);
    cairo_arc(cr, px, py, size + 3, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_set_source_rgba(cr, canvas->strokeColor.r, canvas->strokeColor.g, canvas->strokeColor.b, 0.8);
    cairo_arc(cr, px, py, size, 0, 2 * M_PI);
    cairo_fill(cr);

    return FALSE;
}

void Canvas::renderPreview(cairo_t* cr) {
    double x1 = startX, y1 = startY;
    double x2 = currentX, y2 = currentY;

    if (shiftHeld && currentTool != ShapeType::Freehand) {
        double dx = x2 - x1;
        double dy = y2 - y1;
        if (currentTool == ShapeType::Line) {
            double len = std::sqrt(dx*dx + dy*dy);
            if (len > 0.01) {
                double angle = std::atan2(dy, dx);
                double snapped = std::round(angle / (M_PI/4)) * (M_PI/4);
                x2 = x1 + std::cos(snapped) * len;
                y2 = y1 + std::sin(snapped) * len;
            }
        } else {
            double size = std::max(std::abs(dx), std::abs(dy));
            x2 = x1 + (dx >= 0 ? size : -size);
            y2 = y1 + (dy >= 0 ? size : -size);
        }
    }

    if (currentTool == ShapeType::Freehand) {
        if (currentPoints.size() < 2) return;
        if (currentBrush == BrushType::Eraser) {
            cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
            cairo_set_source_rgba(cr, 0, 0, 0, 1);
            cairo_set_line_width(cr, strokeWidth * 3);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
            cairo_move_to(cr, currentPoints[0].x, currentPoints[0].y);
            for (size_t i = 1; i < currentPoints.size(); i++)
                cairo_line_to(cr, currentPoints[i].x, currentPoints[i].y);
            cairo_stroke(cr);
            cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
            return;
        }
        if (currentBrush == BrushType::Spray) {
            cairo_set_source_rgba(cr, strokeColor.r, strokeColor.g, strokeColor.b,
                                  strokeColor.a * brushOpacity * 0.6);
            double radius = strokeWidth * 2;
            for (const auto& p : currentPoints) {
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
        cairo_set_source_rgba(cr, strokeColor.r, strokeColor.g, strokeColor.b,
                              strokeColor.a * brushOpacity * (currentBrush == BrushType::Marker ? 0.5 : 0.7));
        cairo_set_line_width(cr, strokeWidth);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        cairo_move_to(cr, currentPoints[0].x, currentPoints[0].y);
        for (size_t i = 1; i < currentPoints.size(); i++)
            cairo_line_to(cr, currentPoints[i].x, currentPoints[i].y);
        cairo_stroke(cr);
        return;
    }

    switch (currentTool) {
        case ShapeType::Line:
            cairo_set_source_rgba(cr, strokeColor.r, strokeColor.g, strokeColor.b, 0.7);
            cairo_set_line_width(cr, strokeWidth);
            cairo_move_to(cr, x1, y1);
            cairo_line_to(cr, x2, y2);
            cairo_stroke(cr);
            break;
        case ShapeType::Rectangle:
            cairo_set_source_rgba(cr, strokeColor.r, strokeColor.g, strokeColor.b, 0.7);
            cairo_set_line_width(cr, strokeWidth);
            cairo_rectangle(cr, std::min(x1, x2), std::min(y1, y2), std::abs(x2 - x1), std::abs(y2 - y1));
            cairo_stroke(cr);
            break;
        case ShapeType::Circle: {
            double cx, cy, rx, ry;
            if (centerDrawing) {
                cx = x1; cy = y1;
                rx = std::abs(x2 - x1); ry = std::abs(y2 - y1);
            } else {
                cx = (x1 + x2) / 2; cy = (y1 + y2) / 2;
                rx = std::abs(x2 - x1) / 2; ry = std::abs(y2 - y1) / 2;
            }
            cairo_set_source_rgba(cr, strokeColor.r, strokeColor.g, strokeColor.b, 0.7);
            cairo_set_line_width(cr, strokeWidth);
            cairo_save(cr);
            cairo_translate(cr, cx, cy);
            cairo_scale(cr, rx, ry);
            cairo_arc(cr, 0, 0, 1, 0, 2 * M_PI);
            cairo_restore(cr);
            cairo_stroke(cr);
            break;
        }
        default: break;
    }
}

void Canvas::renderSelection(cairo_t* cr) {
    if (!selectedShape) return;
    cairo_set_source_rgb(cr, 0, 0.4, 1);
    cairo_set_line_width(cr, 1.5);
    cairo_set_dash(cr, (double[]){4, 4}, 2, 0);

    switch (selectedShape->type) {
        case ShapeType::Rectangle: {
            auto* r = static_cast<RectShape*>(selectedShape);
            cairo_rectangle(cr, r->x - 3, r->y - 3, r->w + 6, r->h + 6);
            break;
        }
        case ShapeType::Circle: {
            auto* c = static_cast<CircleShape*>(selectedShape);
            cairo_save(cr);
            cairo_translate(cr, c->cx, c->cy);
            cairo_scale(cr, c->rx + 3, c->ry + 3);
            cairo_arc(cr, 0, 0, 1, 0, 2 * M_PI);
            cairo_restore(cr);
            break;
        }
        case ShapeType::Line: {
            auto* l = static_cast<LineShape*>(selectedShape);
            cairo_rectangle(cr, std::min(l->x1, l->x2) - 5, std::min(l->y1, l->y2) - 5,
                          std::abs(l->x2 - l->x1) + 10, std::abs(l->y2 - l->y1) + 10);
            break;
        }
        case ShapeType::Freehand: {
            auto* f = static_cast<FreehandShape*>(selectedShape);
            if (!f->points.empty()) {
                double minX = f->points[0].x, minY = f->points[0].y;
                double maxX = f->points[0].x, maxY = f->points[0].y;
                for (const auto& p : f->points) {
                    minX = std::min(minX, p.x); maxX = std::max(maxX, p.x);
                    minY = std::min(minY, p.y); maxY = std::max(maxY, p.y);
                }
                cairo_rectangle(cr, minX - 5, minY - 5, maxX - minX + 10, maxY - minY + 10);
            }
            break;
        }
        case ShapeType::Image: {
            auto* im = static_cast<ImageShape*>(selectedShape);
            cairo_rectangle(cr, im->x - 3, im->y - 3, im->getWidth() * im->scale + 6, im->getHeight() * im->scale + 6);
            break;
        }
        default: break;
    }
    cairo_stroke(cr);
    cairo_set_dash(cr, nullptr, 0, 0);
}

gboolean Canvas::onPress(GtkWidget* w, GdkEventButton* ev, gpointer data) {
    auto* canvas = static_cast<Canvas*>(data);
    gtk_widget_grab_focus(w);
    canvas->shiftHeld = (ev->state & GDK_SHIFT_MASK) != 0;

    if (ev->button == 1) {
        if (canvas->currentTool == ShapeType::None) {
            Shape* hit = canvas->hitTest(ev->x, ev->y);
            if (hit) {
                canvas->selectedShape = hit;
                canvas->dragging = true;
                canvas->dragOffsetX = ev->x;
                canvas->dragOffsetY = ev->y;
                if (canvas->measureLabel)
                    gtk_label_set_text(GTK_LABEL(canvas->measureLabel), hit->getMeasurements().c_str());
            } else {
                canvas->selectNone();
            }
            canvas->redraw();
            return TRUE;
        }
        canvas->handlePress(ev->x, ev->y);
    }
    return TRUE;
}

gboolean Canvas::onRelease(GtkWidget* w, GdkEventButton* ev, gpointer data) {
    auto* canvas = static_cast<Canvas*>(data);
    canvas->shiftHeld = (ev->state & GDK_SHIFT_MASK) != 0;
    if (ev->button == 1) {
        if (canvas->dragging) { canvas->dragging = false; return TRUE; }
        canvas->handleRelease(ev->x, ev->y);
    }
    return TRUE;
}

gboolean Canvas::onMove(GtkWidget* w, GdkEventMotion* ev, gpointer data) {
    auto* canvas = static_cast<Canvas*>(data);
    canvas->shiftHeld = (ev->state & GDK_SHIFT_MASK) != 0;

    if (canvas->coordLabel) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Coords: (%.0f, %.0f)", ev->x, ev->y);
        gtk_label_set_text(GTK_LABEL(canvas->coordLabel), buf);
    }

    if (canvas->dragging && canvas->selectedShape) {
        double dx = ev->x - canvas->dragOffsetX;
        double dy = ev->y - canvas->dragOffsetY;
        canvas->selectedShape->moveBy(dx, dy);
        canvas->dragOffsetX = ev->x;
        canvas->dragOffsetY = ev->y;
        if (canvas->measureLabel)
            gtk_label_set_text(GTK_LABEL(canvas->measureLabel), canvas->selectedShape->getMeasurements().c_str());
        canvas->redraw();
        return TRUE;
    }

    if (canvas->drawing) {
        canvas->currentX = ev->x;
        canvas->currentY = ev->y;
        if (canvas->currentTool == ShapeType::Freehand)
            canvas->currentPoints.emplace_back(ev->x, ev->y);
        canvas->redraw();
    }
    return TRUE;
}

gboolean Canvas::onKeyPress(GtkWidget* w, GdkEventKey* ev, gpointer data) {
    auto* canvas = static_cast<Canvas*>(data);
    if (ev->keyval == GDK_KEY_Delete || ev->keyval == GDK_KEY_BackSpace) {
        canvas->deleteSelected();
        return TRUE;
    }
    if (ev->keyval == GDK_KEY_Escape) {
        canvas->selectNone();
        canvas->redraw();
        return TRUE;
    }
    if ((ev->state & GDK_CONTROL_MASK) && ev->keyval == GDK_KEY_z) {
        canvas->project->undo();
        canvas->selectNone();
        canvas->redraw();
        if (canvas->onUndoCallback)
            canvas->onUndoCallback();
        return TRUE;
    }
    if (ev->keyval == GDK_KEY_bracketleft) {
        canvas->strokeWidth = std::max(0.5, canvas->strokeWidth - 0.5);
        if (canvas->onSizeChanged)
            canvas->onSizeChanged();
        canvas->redraw();
        return TRUE;
    }
    if (ev->keyval == GDK_KEY_bracketright) {
        canvas->strokeWidth = std::min(100.0, canvas->strokeWidth + 0.5);
        if (canvas->onSizeChanged)
            canvas->onSizeChanged();
        canvas->redraw();
        return TRUE;
    }
    if (ev->keyval == GDK_KEY_Shift_L || ev->keyval == GDK_KEY_Shift_R) {
        canvas->shiftHeld = (ev->state & GDK_SHIFT_MASK) != 0;
        canvas->redraw();
        return TRUE;
    }
    return FALSE;
}

gboolean Canvas::onKeyRelease(GtkWidget* w, GdkEventKey* ev, gpointer data) {
    auto* canvas = static_cast<Canvas*>(data);
    if (ev->keyval == GDK_KEY_Shift_L || ev->keyval == GDK_KEY_Shift_R) {
        canvas->shiftHeld = false;
        canvas->redraw();
        return TRUE;
    }
    return FALSE;
}

void Canvas::handlePress(double x, double y) {
    drawing = true;
    startX = x; startY = y;
    currentX = x; currentY = y;
    if (currentTool == ShapeType::Freehand) {
        currentPoints.clear();
        currentPoints.emplace_back(x, y);
    }
    selectNone();
    redraw();
}

void Canvas::handleRelease(double x, double y) {
    if (!drawing) return;
    drawing = false;

    auto* layer = project->getActiveLayer();
    if (!layer) return;

    double cx = 0, cy = 0, cw = 0, ch = 0;

    if (shiftHeld && currentTool != ShapeType::Freehand) {
        double dx = x - startX;
        double dy = y - startY;
        if (currentTool == ShapeType::Line) {
            double len = std::sqrt(dx*dx + dy*dy);
            if (len > 0.01) {
                double angle = std::atan2(dy, dx);
                double snapped = std::round(angle / (M_PI/4)) * (M_PI/4);
                cx = startX + std::cos(snapped) * len;
                cy = startY + std::sin(snapped) * len;
            } else { cx = x; cy = y; }
            x = cx; y = cy;
        } else {
            double size = std::max(std::abs(dx), std::abs(dy));
            cx = startX + (dx >= 0 ? size : -size);
            cy = startY + (dy >= 0 ? size : -size);
            x = cx; y = cy;
        }
    }

    project->saveUndoState();

    double x1 = std::min(startX, x), y1 = std::min(startY, y);
    double x2 = std::max(startX, x), y2 = std::max(startY, y);

    switch (currentTool) {
        case ShapeType::Freehand: {
            if (currentPoints.size() < 2) { currentPoints.clear(); break; }
            auto shape = std::make_unique<FreehandShape>();
            shape->strokeColor = strokeColor;
            shape->strokeWidth = strokeWidth;
            shape->opacity = brushOpacity;
            shape->brushType = currentBrush;
            shape->points = currentPoints;
            currentPoints.clear();
            layer->addShape(std::move(shape));
            break;
        }
        case ShapeType::Line: {
            if (std::abs(x2 - x1) < 1 && std::abs(y2 - y1) < 1) break;
            auto shape = std::make_unique<LineShape>();
            shape->strokeColor = strokeColor;
            shape->strokeWidth = strokeWidth;
            shape->x1 = startX; shape->y1 = startY;
            shape->x2 = x; shape->y2 = y;
            layer->addShape(std::move(shape));
            break;
        }
        case ShapeType::Rectangle: {
            if (std::abs(x2 - x1) < 1 && std::abs(y2 - y1) < 1) break;
            auto shape = std::make_unique<RectShape>();
            shape->strokeColor = strokeColor;
            shape->fillColor = fillColor;
            shape->strokeWidth = strokeWidth;
            shape->filled = (fillColor.a > 0);
            shape->opacity = brushOpacity;
            shape->x = x1; shape->y = y1;
            shape->w = x2 - x1; shape->h = y2 - y1;
            layer->addShape(std::move(shape));
            break;
        }
        case ShapeType::Circle: {
            if (std::abs(x2 - x1) < 1 && std::abs(y2 - y1) < 1) break;
            auto shape = std::make_unique<CircleShape>();
            shape->strokeColor = strokeColor;
            shape->fillColor = fillColor;
            shape->strokeWidth = strokeWidth;
            shape->filled = (fillColor.a > 0);
            shape->opacity = brushOpacity;
            if (centerDrawing) {
                shape->cx = startX;
                shape->cy = startY;
                shape->rx = std::abs(x - startX);
                shape->ry = std::abs(y - startY);
            } else {
                shape->cx = (startX + x) / 2;
                shape->cy = (startY + y) / 2;
                shape->rx = std::abs(x - startX) / 2;
                shape->ry = std::abs(y - startY) / 2;
            }
            layer->addShape(std::move(shape));
            break;
        }
        default: break;
    }
    redraw();
}

void Canvas::handleMove(double x, double y) {
    currentX = x; currentY = y;
    if (currentTool == ShapeType::Freehand && drawing)
        currentPoints.emplace_back(x, y);
    redraw();
}
