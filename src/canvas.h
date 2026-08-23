#pragma once
#include <gtk/gtk.h>
#include <functional>
#include "project.h"
#include "types.h"

class Canvas {
public:
    GtkWidget* widget = nullptr;
    Project* project = nullptr;
    ShapeType currentTool = ShapeType::Freehand;
    BrushType currentBrush = BrushType::Pen;
    Color strokeColor{0, 0, 0, 1};
    Color fillColor{0.2, 0.5, 0.8, 0.5};
    double strokeWidth = 2.0;
    double brushOpacity = 1.0;
    bool showMeasurements = true;

    bool drawing = false;
    double startX = 0, startY = 0;
    double currentX = 0, currentY = 0;
    std::vector<Point> currentPoints;

    Shape* selectedShape = nullptr;
    int selectedLayerIdx = -1;
    int selectedShapeIdx = -1;
    bool dragging = false;
    bool shiftHeld = false;
    bool centerDrawing = false;
    double dragOffsetX = 0, dragOffsetY = 0;

    GtkWidget* coordLabel = nullptr;
    GtkWidget* measureLabel = nullptr;

    Canvas(Project* proj);
    ~Canvas();

    GtkWidget* create();
    void redraw();
    void setTool(ShapeType tool);
    void setBrush(BrushType bt);

    static gboolean onDraw(GtkWidget* w, cairo_t* cr, gpointer data);
    static gboolean onPress(GtkWidget* w, GdkEventButton* ev, gpointer data);
    static gboolean onRelease(GtkWidget* w, GdkEventButton* ev, gpointer data);
    static gboolean onMove(GtkWidget* w, GdkEventMotion* ev, gpointer data);
    static gboolean onKeyPress(GtkWidget* w, GdkEventKey* ev, gpointer data);
    static gboolean onKeyRelease(GtkWidget* w, GdkEventKey* ev, gpointer data);

    void handlePress(double x, double y);
    void handleRelease(double x, double y);
    void handleMove(double x, double y);
    void renderPreview(cairo_t* cr);
    void renderSelection(cairo_t* cr);

    Shape* hitTest(double x, double y);
    void deleteSelected();
    void selectNone();
    std::function<void()> onUndoCallback;
    std::function<void()> onSizeChanged;
};
