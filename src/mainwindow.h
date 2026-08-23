#pragma once
#include <gtk/gtk.h>
#include "canvas.h"
#include "project.h"

class MainWindow {
public:
    GtkWidget* window = nullptr;
    Project project;
    Canvas canvas;

    GtkWidget* layerList = nullptr;
    GtkWidget* toolGroup = nullptr;
    GtkWidget* brushOpacityScale = nullptr;
    GtkWidget* brushTypeCombo = nullptr;
    GtkWidget* widthSpin = nullptr;

    bool firstRender = true;

    MainWindow();
    ~MainWindow();

    void run(int argc, char* argv[]);
    void openFile(const std::string& path);

private:
    GtkWidget* createToolbar();
    GtkWidget* createLayerPanel();
    GtkWidget* createStatusBar();
    void updateLayerList();
    void updateCanvasSize();

    static void onNew(GtkWidget* w, gpointer data);
    static void onOpen(GtkWidget* w, gpointer data);
    static void onSave(GtkWidget* w, gpointer data);
    static void onExport(GtkWidget* w, gpointer data);
    static void onOpenImage(GtkWidget* w, gpointer data);
    static void onToolClick(GtkWidget* w, gpointer data);
    static void onBrushTypeChange(GtkWidget* w, gpointer data);
    static void onStrokeColor(GtkWidget* w, gpointer data);
    static void onFillColor(GtkWidget* w, gpointer data);
    static void onStrokeWidth(GtkWidget* w, gpointer data);
    static void onBrushOpacityChange(GtkWidget* w, gpointer data);
    static void onAddLayer(GtkWidget* w, gpointer data);
    static void onRemoveLayer(GtkWidget* w, gpointer data);
    static void onDuplicateLayer(GtkWidget* w, gpointer data);
    static void onMergeLayer(GtkWidget* w, gpointer data);
    static void onRenameLayer(GtkWidget* w, gpointer data);
    static void onLayerSelect(GtkListBox* box, GtkListBoxRow* row, gpointer data);
    static void onOpacityChange(GtkWidget* w, gpointer data);
    static void onToggleMeasure(GtkWidget* w, gpointer data);
    static void onToggleCenter(GtkWidget* w, gpointer data);
    static void onSizeDec(GtkWidget* w, gpointer data);
    static void onSizeInc(GtkWidget* w, gpointer data);
    static void onLayerVisibilityToggle(GtkWidget* w, gpointer data);
    static gboolean onDeleteWindow(GtkWidget* w, GdkEvent* ev, gpointer data);
    static gboolean onLayerDoubleClick(GtkWidget* w, GdkEventButton* ev, gpointer data);
};
