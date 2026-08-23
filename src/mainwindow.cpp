#include "mainwindow.h"
#include <cstring>
#include <cctype>

MainWindow::MainWindow() : canvas(&project) {}
MainWindow::~MainWindow() {}

void MainWindow::run(int argc, char* argv[]) {
    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "LunarDraw");
    gtk_window_set_default_size(GTK_WINDOW(window), 1300, 850);
    g_signal_connect(window, "delete-event", G_CALLBACK(onDeleteWindow), nullptr);
    g_object_set_data(G_OBJECT(window), "mw", this);

    auto* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    auto* menubar = gtk_menu_bar_new();
    auto* fileMenu = gtk_menu_new();
    auto* fileItem = gtk_menu_item_new_with_label("File");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileItem), fileMenu);

    auto* newItem = gtk_menu_item_new_with_label("New");
    g_signal_connect(newItem, "activate", G_CALLBACK(onNew), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), newItem);

    auto* openItem = gtk_menu_item_new_with_label("Open Project (.lunar)");
    g_signal_connect(openItem, "activate", G_CALLBACK(onOpen), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), openItem);

    auto* openImgItem = gtk_menu_item_new_with_label("Open Image (PNG/JPG)");
    g_signal_connect(openImgItem, "activate", G_CALLBACK(onOpenImage), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), openImgItem);

    auto* saveItem = gtk_menu_item_new_with_label("Save Project");
    g_signal_connect(saveItem, "activate", G_CALLBACK(onSave), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), saveItem);

    auto* sep1 = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), sep1);

    auto* exportItem = gtk_menu_item_new_with_label("Export PNG");
    g_signal_connect(exportItem, "activate", G_CALLBACK(onExport), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), exportItem);

    auto* quitItem = gtk_menu_item_new_with_label("Quit");
    g_signal_connect(quitItem, "activate", G_CALLBACK(gtk_main_quit), nullptr);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), quitItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileItem);

    auto* viewMenu = gtk_menu_new();
    auto* viewItem = gtk_menu_item_new_with_label("View");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(viewItem), viewMenu);

    auto* measureItem = gtk_check_menu_item_new_with_label("Show Measurements");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(measureItem), TRUE);
    g_signal_connect(measureItem, "toggled", G_CALLBACK(onToggleMeasure), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), measureItem);

    auto* centerItem = gtk_check_menu_item_new_with_label("Circle from Center");
    g_signal_connect(centerItem, "toggled", G_CALLBACK(onToggleCenter), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), centerItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), viewItem);
    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    auto* toolbar = createToolbar();
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    auto* hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), hpaned, TRUE, TRUE, 0);

    auto* scrolled = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    auto* coordLabel = gtk_label_new("Coords: (0, 0)");
    auto* measureLabel = gtk_label_new("");
    canvas.coordLabel = coordLabel;
    canvas.measureLabel = measureLabel;

    auto* canvasWidget = canvas.create();
    gtk_container_add(GTK_CONTAINER(scrolled), canvasWidget);
    gtk_paned_add1(GTK_PANED(hpaned), scrolled);

    canvas.onUndoCallback = [this]() { updateLayerList(); };
    canvas.onSizeChanged = [this]() {
        if (widthSpin)
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(widthSpin), canvas.strokeWidth);
    };

    auto* layerPanel = createLayerPanel();
    gtk_paned_add2(GTK_PANED(hpaned), layerPanel);
    gtk_paned_set_position(GTK_PANED(hpaned), 950);

    auto* statusBar = createStatusBar();
    gtk_box_pack_start(GTK_BOX(vbox), statusBar, FALSE, FALSE, 0);

    updateLayerList();
    gtk_widget_show_all(window);

    if (argc > 1) {
        std::string* pathArg = new std::string(argv[1]);
        g_timeout_add(100, [](gpointer data) -> gboolean {
            auto* mw = static_cast<MainWindow*>(data);
            auto* path = static_cast<std::string*>(g_object_get_data(G_OBJECT(mw->window), "open-path"));
            if (path) {
                mw->openFile(*path);
                g_object_set_data(G_OBJECT(mw->window), "open-path", nullptr);
                delete path;
            }
            return FALSE;
        }, this);
        g_object_set_data(G_OBJECT(window), "open-path", pathArg);
    }

    gtk_main();
}

void MainWindow::openFile(const std::string& path) {
    size_t dot = path.find_last_of('.');
    if (dot != std::string::npos) {
        std::string ext;
        for (auto& c : path.substr(dot)) ext += (char)std::tolower((unsigned char)c);
        if (ext == ".lunar") {
            if (!project.load(path)) {
                GtkWidget* d = gtk_message_dialog_new(GTK_WINDOW(window),
                    GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                    "Failed to load project: %s", path.c_str());
                gtk_dialog_run(GTK_DIALOG(d));
                gtk_widget_destroy(d);
                return;
            }
        } else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
            if (!project.loadImageAsLayer(path)) {
                GtkWidget* d = gtk_message_dialog_new(GTK_WINDOW(window),
                    GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                    "Failed to load image: %s", path.c_str());
                gtk_dialog_run(GTK_DIALOG(d));
                gtk_widget_destroy(d);
                return;
            }
            updateCanvasSize();
        }
    }
    canvas.redraw();
    updateLayerList();
}

void MainWindow::updateCanvasSize() {
    if (canvas.widget) {
        gtk_widget_set_size_request(canvas.widget, project.width, project.height);
    }
}

GtkWidget* MainWindow::createToolbar() {
    auto* toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_container_set_border_width(GTK_CONTAINER(toolbar), 2);

    // === TOOLS GROUP ===
    auto* toolsFrame = gtk_frame_new("Tools");
    auto* toolsBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_container_set_border_width(GTK_CONTAINER(toolsBox), 2);
    gtk_container_add(GTK_CONTAINER(toolsFrame), toolsBox);

    toolGroup = nullptr;
    struct ToolInfo { const char* label; ShapeType type; };
    ToolInfo tools[] = {
        {"Pen", ShapeType::Freehand},
        {"Line", ShapeType::Line},
        {"Rect", ShapeType::Rectangle},
        {"Circle", ShapeType::Circle},
        {"Select", ShapeType::None},
    };
    for (auto& t : tools) {
        auto* btn = gtk_toggle_button_new_with_label(t.label);
        g_signal_connect(btn, "clicked", G_CALLBACK(onToolClick), this);
        g_object_set_data(G_OBJECT(btn), "tool-type", GINT_TO_POINTER((int)t.type));
        if (!toolGroup) { toolGroup = btn; gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn), TRUE); }
        gtk_box_pack_start(GTK_BOX(toolsBox), btn, FALSE, FALSE, 2);
    }
    gtk_box_pack_start(GTK_BOX(toolbar), toolsFrame, FALSE, FALSE, 2);

    // === BRUSH GROUP ===
    auto* brushFrame = gtk_frame_new("Brush");
    auto* brushBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_container_set_border_width(GTK_CONTAINER(brushBox), 2);
    gtk_container_add(GTK_CONTAINER(brushFrame), brushBox);

    brushTypeCombo = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(brushTypeCombo), nullptr, "Pen");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(brushTypeCombo), nullptr, "Marker");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(brushTypeCombo), nullptr, "Spray");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(brushTypeCombo), nullptr, "Eraser");
    gtk_combo_box_set_active(GTK_COMBO_BOX(brushTypeCombo), 0);
    g_signal_connect(brushTypeCombo, "changed", G_CALLBACK(onBrushTypeChange), this);
    gtk_box_pack_start(GTK_BOX(brushBox), brushTypeCombo, FALSE, FALSE, 2);

    gtk_box_pack_start(GTK_BOX(brushBox), gtk_label_new("Opacity:"), FALSE, FALSE, 2);
    brushOpacityScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.05, 1.0, 0.05);
    gtk_range_set_value(GTK_RANGE(brushOpacityScale), 1.0);
    gtk_widget_set_size_request(brushOpacityScale, 90, -1);
    g_signal_connect(brushOpacityScale, "value-changed", G_CALLBACK(onBrushOpacityChange), this);
    gtk_box_pack_start(GTK_BOX(brushBox), brushOpacityScale, FALSE, FALSE, 2);

    gtk_box_pack_start(GTK_BOX(toolbar), brushFrame, FALSE, FALSE, 2);

    // === SHAPE GROUP ===
    auto* shapeFrame = gtk_frame_new("Shape");
    auto* shapeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_container_set_border_width(GTK_CONTAINER(shapeBox), 2);
    gtk_container_add(GTK_CONTAINER(shapeFrame), shapeBox);

    GdkRGBA strokeDef = {0, 0, 0, 1};
    auto* strokeBtn = gtk_color_button_new_with_rgba(&strokeDef);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(strokeBtn), "Stroke Color");
    g_signal_connect(strokeBtn, "color-set", G_CALLBACK(onStrokeColor), this);
    gtk_box_pack_start(GTK_BOX(shapeBox), gtk_label_new("Stroke:"), FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(shapeBox), strokeBtn, FALSE, FALSE, 2);

    GdkRGBA fillDef = {0.2, 0.5, 0.8, 0.5};
    auto* fillBtn = gtk_color_button_new_with_rgba(&fillDef);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(fillBtn), "Fill Color");
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(fillBtn), TRUE);
    g_signal_connect(fillBtn, "color-set", G_CALLBACK(onFillColor), this);
    gtk_box_pack_start(GTK_BOX(shapeBox), gtk_label_new("Fill:"), FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(shapeBox), fillBtn, FALSE, FALSE, 2);

    gtk_box_pack_start(GTK_BOX(shapeBox), gtk_label_new("Size:"), FALSE, FALSE, 2);

    auto* decBtn = gtk_button_new_with_label("\xe2\x88\x92");
    gtk_widget_set_size_request(decBtn, 24, -1);
    g_signal_connect(decBtn, "clicked", G_CALLBACK(onSizeDec), this);
    gtk_box_pack_start(GTK_BOX(shapeBox), decBtn, FALSE, FALSE, 1);

    widthSpin = gtk_spin_button_new_with_range(0.5, 100.0, 0.5);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(widthSpin), 2.0);
    gtk_widget_set_size_request(widthSpin, 60, -1);
    g_signal_connect(widthSpin, "value-changed", G_CALLBACK(onStrokeWidth), this);
    gtk_box_pack_start(GTK_BOX(shapeBox), widthSpin, FALSE, FALSE, 1);

    auto* incBtn = gtk_button_new_with_label("+");
    gtk_widget_set_size_request(incBtn, 24, -1);
    g_signal_connect(incBtn, "clicked", G_CALLBACK(onSizeInc), this);
    gtk_box_pack_start(GTK_BOX(shapeBox), incBtn, FALSE, FALSE, 1);

    gtk_box_pack_start(GTK_BOX(toolbar), shapeFrame, FALSE, FALSE, 2);

    return toolbar;
}

GtkWidget* MainWindow::createLayerPanel() {
    auto* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
    gtk_widget_set_size_request(vbox, 280, -1);

    auto* title = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(title), "<b>Layers</b>");
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 4);

    auto* scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    layerList = gtk_list_box_new();
    g_signal_connect(layerList, "row-selected", G_CALLBACK(onLayerSelect), this);
    gtk_container_add(GTK_CONTAINER(scroll), layerList);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

    auto* btnBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    auto* addBtn = gtk_button_new_with_label("+ Add");
    g_signal_connect(addBtn, "clicked", G_CALLBACK(onAddLayer), this);
    gtk_box_pack_start(GTK_BOX(btnBox), addBtn, TRUE, TRUE, 2);

    auto* remBtn = gtk_button_new_with_label("- Remove");
    g_signal_connect(remBtn, "clicked", G_CALLBACK(onRemoveLayer), this);
    gtk_box_pack_start(GTK_BOX(btnBox), remBtn, TRUE, TRUE, 2);

    auto* dupBtn = gtk_button_new_with_label("Duplicate");
    g_signal_connect(dupBtn, "clicked", G_CALLBACK(onDuplicateLayer), this);
    gtk_box_pack_start(GTK_BOX(btnBox), dupBtn, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(vbox), btnBox, FALSE, FALSE, 2);

    auto* btnBox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    auto* mergeBtn = gtk_button_new_with_label("Merge Down");
    g_signal_connect(mergeBtn, "clicked", G_CALLBACK(onMergeLayer), this);
    gtk_box_pack_start(GTK_BOX(btnBox2), mergeBtn, TRUE, TRUE, 2);

    auto* renBtn = gtk_button_new_with_label("Rename");
    g_signal_connect(renBtn, "clicked", G_CALLBACK(onRenameLayer), this);
    gtk_box_pack_start(GTK_BOX(btnBox2), renBtn, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(vbox), btnBox2, FALSE, FALSE, 2);

    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("Layer Opacity:"), FALSE, FALSE, 2);
    auto* opacityScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.05);
    gtk_range_set_value(GTK_RANGE(opacityScale), 1.0);
    g_signal_connect(opacityScale, "value-changed", G_CALLBACK(onOpacityChange), this);
    gtk_box_pack_start(GTK_BOX(vbox), opacityScale, FALSE, FALSE, 2);

    return vbox;
}

GtkWidget* MainWindow::createStatusBar() {
    auto* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 2);
    gtk_box_pack_start(GTK_BOX(hbox), canvas.coordLabel, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(hbox), canvas.measureLabel, FALSE, FALSE, 4);

    char sizeBuf[64];
    snprintf(sizeBuf, sizeof(sizeBuf), "Canvas: %dx%d", project.width, project.height);
    auto* sizeLabel = gtk_label_new(sizeBuf);
    gtk_box_pack_end(GTK_BOX(hbox), sizeLabel, FALSE, FALSE, 4);
    return hbox;
}

void MainWindow::updateLayerList() {
    if (!layerList) return;

    GtkListBoxRow* selectedRow = gtk_list_box_get_selected_row(GTK_LIST_BOX(layerList));
    int selectedIdx = -1;
    if (selectedRow)
        selectedIdx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(selectedRow), "layer-index"));

    gtk_container_foreach(GTK_CONTAINER(layerList), (GtkCallback)gtk_widget_destroy, nullptr);

    for (int i = 0; i < (int)project.layers.size(); i++) {
        auto* layer = project.layers[i].get();
        auto* row = gtk_list_box_row_new();
        g_object_set_data(G_OBJECT(row), "layer-index", GINT_TO_POINTER(i));
        g_object_set_data(G_OBJECT(row), "mw", this);

        auto* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
        gtk_container_set_border_width(GTK_CONTAINER(hbox), 2);

        auto* visBtn = gtk_check_button_new();
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(visBtn), layer->visible);
        g_object_set_data(G_OBJECT(visBtn), "layer-row", row);
        g_signal_connect(visBtn, "toggled", G_CALLBACK(onLayerVisibilityToggle), this);
        gtk_box_pack_start(GTK_BOX(hbox), visBtn, FALSE, FALSE, 2);

        auto* nameLabel = gtk_label_new(layer->name.c_str());
        gtk_box_pack_start(GTK_BOX(hbox), nameLabel, TRUE, TRUE, 2);

        auto* countLabel = gtk_label_new(std::to_string(layer->shapes.size()).c_str());
        gtk_box_pack_end(GTK_BOX(hbox), countLabel, FALSE, FALSE, 2);

        gtk_container_add(GTK_CONTAINER(row), hbox);
        gtk_list_box_insert(GTK_LIST_BOX(layerList), GTK_WIDGET(row), -1);
        gtk_widget_show_all(GTK_WIDGET(row));
    }

    if (selectedIdx >= 0 && selectedIdx < (int)project.layers.size()) {
        auto* row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(layerList), selectedIdx);
        if (row) gtk_list_box_select_row(GTK_LIST_BOX(layerList), row);
    } else if ((int)project.layers.size() > 0) {
        auto* row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(layerList), project.activeLayer);
        if (row) gtk_list_box_select_row(GTK_LIST_BOX(layerList), row);
    }
}

gboolean MainWindow::onDeleteWindow(GtkWidget* w, GdkEvent* ev, gpointer data) {
    gtk_main_quit();
    return TRUE;
}

void MainWindow::onNew(GtkWidget* w, gpointer data) {
    auto* mw = static_cast<MainWindow*>(data);
    mw->project.newProject(800, 600);
    mw->updateCanvasSize();
    mw->canvas.redraw();
    mw->updateLayerList();
}

void MainWindow::onOpen(GtkWidget* w, gpointer data) {
    auto* mw = static_cast<MainWindow*>(data);
    auto* dialog = gtk_file_chooser_dialog_new("Open Project",
        GTK_WINDOW(mw->window), GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, nullptr);
    auto* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "LunarDraw Project (*.lunar)");
    gtk_file_filter_add_pattern(filter, "*.lunar");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        mw->openFile(path);
        g_free(path);
    }
    gtk_widget_destroy(dialog);
}

void MainWindow::onOpenImage(GtkWidget* w, gpointer data) {
    auto* mw = static_cast<MainWindow*>(data);
    auto* dialog = gtk_file_chooser_dialog_new("Open Image",
        GTK_WINDOW(mw->window), GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, nullptr);
    auto* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Images (*.png *.jpg)");
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_filter_add_pattern(filter, "*.jpg");
    gtk_file_filter_add_pattern(filter, "*.jpeg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (!mw->project.loadImageAsLayer(path)) {
            GtkWidget* d = gtk_message_dialog_new(GTK_WINDOW(mw->window),
                GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                "Failed to load image");
            gtk_dialog_run(GTK_DIALOG(d));
            gtk_widget_destroy(d);
        }
        mw->updateCanvasSize();
        mw->canvas.redraw();
        mw->updateLayerList();
        g_free(path);
    }
    gtk_widget_destroy(dialog);
}

void MainWindow::onSave(GtkWidget* w, gpointer data) {
    auto* mw = static_cast<MainWindow*>(data);
    auto* dialog = gtk_file_chooser_dialog_new("Save Project",
        GTK_WINDOW(mw->window), GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancel", GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT, nullptr);
    auto* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "LunarDraw Project (*.lunar)");
    gtk_file_filter_add_pattern(filter, "*.lunar");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "untitled.lunar");

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        mw->project.save(path);
        g_free(path);
    }
    gtk_widget_destroy(dialog);
}

void MainWindow::onExport(GtkWidget* w, gpointer data) {
    auto* mw = static_cast<MainWindow*>(data);
    auto* dialog = gtk_file_chooser_dialog_new("Export Image",
        GTK_WINDOW(mw->window), GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancel", GTK_RESPONSE_CANCEL, "_Export", GTK_RESPONSE_ACCEPT, nullptr);
    auto* filterPNG = gtk_file_filter_new();
    gtk_file_filter_set_name(filterPNG, "PNG Image (*.png)");
    gtk_file_filter_add_pattern(filterPNG, "*.png");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filterPNG);
    auto* filterJPG = gtk_file_filter_new();
    gtk_file_filter_set_name(filterJPG, "JPEG Image (*.jpg)");
    gtk_file_filter_add_pattern(filterJPG, "*.jpg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filterJPG);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "export.png");

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        std::string spath(path);
        if (spath.size() >= 4 && spath.substr(spath.size()-4) == ".jpg") {
            if (!mw->project.exportJPG(spath)) {
                GtkWidget* d = gtk_message_dialog_new(GTK_WINDOW(mw->window),
                    GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                    "JPG export requires ImageMagick (convert)");
                gtk_dialog_run(GTK_DIALOG(d));
                gtk_widget_destroy(d);
            }
        } else {
            if (spath.find('.') == std::string::npos) spath += ".png";
            mw->project.exportPNG(spath);
        }
        g_free(path);
    }
    gtk_widget_destroy(dialog);
}

void MainWindow::onToolClick(GtkWidget* w, gpointer data) {
    auto* mw = static_cast<MainWindow*>(data);
    if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
        return;
    }
    int type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "tool-type"));
    if (type == -1) mw->canvas.currentTool = ShapeType::None;
    else mw->canvas.currentTool = static_cast<ShapeType>(type);
    mw->canvas.selectNone();
    mw->canvas.redraw();
}

void MainWindow::onBrushTypeChange(GtkWidget* w, gpointer data) {
    auto* mw = static_cast<MainWindow*>(data);
    int active = gtk_combo_box_get_active(GTK_COMBO_BOX(w));
    mw->canvas.setBrush(static_cast<BrushType>(active));
}

void MainWindow::onStrokeColor(GtkWidget* w, gpointer data) {
    auto* mw = static_cast<MainWindow*>(data);
    GdkRGBA color;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(w), &color);
    mw->canvas.strokeColor = Color(color.red, color.green, color.blue, color.alpha);
}

void MainWindow::onFillColor(GtkWidget* w, gpointer data) {
    auto* mw = static_cast<MainWindow*>(data);
    GdkRGBA color;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(w), &color);
    mw->canvas.fillColor = Color(color.red, color.green, color.blue, color.alpha);
}

void MainWindow::onStrokeWidth(GtkWidget* w, gpointer data) {
    auto* mw = static_cast<MainWindow*>(data);
    mw->canvas.strokeWidth = gtk_spin_button_get_value(GTK_SPIN_BUTTON(w));
}

void MainWindow::onBrushOpacityChange(GtkWidget* w, gpointer data) {
    auto* mw = static_cast<MainWindow*>(data);
    mw->canvas.brushOpacity = gtk_range_get_value(GTK_RANGE(w));
}

void MainWindow::onAddLayer(GtkWidget* w, gpointer data) {
    auto* mw = static_cast<MainWindow*>(data);
    std::string name = "Layer " + std::to_string(mw->project.layers.size());
    mw->project.addLayer(name);
    mw->project.activeLayer = mw->project.layers.size() - 1;
    mw->updateLayerList();
}

void MainWindow::onRemoveLayer(GtkWidget* w, gpointer data) {
    auto* mw = static_cast<MainWindow*>(data);
    GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(mw->layerList));
    if (row) {
        int idx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "layer-index"));
        mw->project.removeLayer(idx);
        mw->canvas.selectNone();
        mw->updateLayerList();
        mw->canvas.redraw();
    }
}

void MainWindow::onDuplicateLayer(GtkWidget* w, gpointer data) {
    auto* mw = static_cast<MainWindow*>(data);
    GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(mw->layerList));
    if (row) {
        int idx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "layer-index"));
        mw->project.duplicateLayer(idx);
        mw->project.activeLayer = idx + 1;
        mw->updateLayerList();
    }
}

void MainWindow::onMergeLayer(GtkWidget* w, gpointer data) {
    auto* mw = static_cast<MainWindow*>(data);
    GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(mw->layerList));
    if (row) {
        int idx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "layer-index"));
        mw->project.mergeDown(idx);
        mw->canvas.selectNone();
        mw->updateLayerList();
        mw->canvas.redraw();
    }
}

void MainWindow::onRenameLayer(GtkWidget* w, gpointer data) {
    auto* mw = static_cast<MainWindow*>(data);
    GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(mw->layerList));
    if (row) {
        int idx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "layer-index"));
        auto* dialog = gtk_message_dialog_new(GTK_WINDOW(mw->window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL,
            "Enter new name for layer:");
        auto* entry = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(entry), mw->project.layers[idx]->name.c_str());
        gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), entry, FALSE, FALSE, 4);
        gtk_widget_show(entry);

        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
            const char* newName = gtk_entry_get_text(GTK_ENTRY(entry));
            mw->project.renameLayer(idx, newName);
            mw->updateLayerList();
        }
        gtk_widget_destroy(dialog);
    }
}

void MainWindow::onLayerSelect(GtkListBox* box, GtkListBoxRow* row, gpointer data) {
    auto* mw = static_cast<MainWindow*>(data);
    if (row) {
        int idx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "layer-index"));
        mw->project.activeLayer = idx;
    }
}

void MainWindow::onOpacityChange(GtkWidget* w, gpointer data) {
    auto* mw = static_cast<MainWindow*>(data);
    double val = gtk_range_get_value(GTK_RANGE(w));
    int idx = mw->project.activeLayer;
    if (idx >= 0 && idx < (int)mw->project.layers.size()) {
        mw->project.layers[idx]->opacity = val;
        mw->canvas.redraw();
    }
}

void MainWindow::onToggleMeasure(GtkWidget* w, gpointer data) {
    auto* mw = static_cast<MainWindow*>(data);
    mw->canvas.showMeasurements = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
    mw->canvas.redraw();
}

void MainWindow::onToggleCenter(GtkWidget* w, gpointer data) {
    auto* mw = static_cast<MainWindow*>(data);
    mw->canvas.centerDrawing = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
}

void MainWindow::onSizeDec(GtkWidget* w, gpointer data) {
    auto* mw = static_cast<MainWindow*>(data);
    double val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(mw->widthSpin));
    val = std::max(0.5, val - 0.5);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(mw->widthSpin), val);
}

void MainWindow::onSizeInc(GtkWidget* w, gpointer data) {
    auto* mw = static_cast<MainWindow*>(data);
    double val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(mw->widthSpin));
    val = std::min(100.0, val + 0.5);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(mw->widthSpin), val);
}

void MainWindow::onLayerVisibilityToggle(GtkWidget* w, gpointer data) {
    auto* mw = static_cast<MainWindow*>(data);
    auto* row = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(w), "layer-row"));
    if (row) {
        int idx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "layer-index"));
        if (idx >= 0 && idx < (int)mw->project.layers.size()) {
            mw->project.layers[idx]->visible = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
            mw->canvas.redraw();
        }
    }
}
