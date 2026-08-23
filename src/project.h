#pragma once
#include "layer.h"
#include <fstream>
#include <cairo.h>

class Project {
public:
    int width = 800;
    int height = 600;
    std::string filename;
    std::vector<std::unique_ptr<Layer>> layers;
    int activeLayer = 0;
    std::vector<std::string> undoStack;

    Project() {
        addLayer("Background");
        addLayer("Layer 1");
        activeLayer = layers.size() - 1;
    }

    void saveUndoState() {
        std::ostringstream ss;
        for (const auto& layer : layers)
            ss << layer->serialize(0);
        undoStack.push_back(ss.str());
        if (undoStack.size() > 50)
            undoStack.erase(undoStack.begin());
    }

    void undo() {
        if (undoStack.empty()) return;
        std::string state = undoStack.back();
        undoStack.pop_back();
        layers.clear();
        std::istringstream ss(state);
        std::string line;
        while (std::getline(ss, line)) {
            if (line.find("LAYER_START") != std::string::npos) {
                std::string data = line + "\n";
                int depth = 1;
                while (depth > 0 && std::getline(ss, line)) {
                    data += line + "\n";
                    if (line.find("LAYER_START") != std::string::npos) depth++;
                    if (line.find("LAYER_END") != std::string::npos) depth--;
                }
                auto layer = Layer::deserialize(data);
                if (layer) layers.push_back(std::move(layer));
            }
        }
        if (layers.empty()) addLayer("Layer 1");
        activeLayer = (int)layers.size() - 1;
    }

    void newProject(int w, int h) {
        width = w; height = h;
        layers.clear();
        undoStack.clear();
        addLayer("Background");
        addLayer("Layer 1");
        activeLayer = layers.size() - 1;
    }

    Layer* getActiveLayer() {
        if (activeLayer < 0 || activeLayer >= (int)layers.size()) return nullptr;
        return layers[activeLayer].get();
    }

    void addLayer(const std::string& name) {
        layers.push_back(std::make_unique<Layer>(name));
    }

    void removeLayer(int index) {
        if (index >= 0 && index < (int)layers.size() && layers.size() > 1) {
            layers.erase(layers.begin() + index);
            if (activeLayer >= (int)layers.size()) activeLayer = (int)layers.size() - 1;
        }
    }

    void duplicateLayer(int index) {
        if (index < 0 || index >= (int)layers.size()) return;
        std::string data = layers[index]->serialize(0);
        auto dup = Layer::deserialize(data);
        if (dup) {
            dup->name = layers[index]->name + " (copy)";
            layers.insert(layers.begin() + index + 1, std::move(dup));
        }
    }

    void renameLayer(int index, const std::string& newName) {
        if (index >= 0 && index < (int)layers.size())
            layers[index]->name = newName;
    }

    void mergeDown(int index) {
        if (index <= 0 || index >= (int)layers.size()) return;
        auto& src = layers[index];
        auto& dst = layers[index - 1];
        for (auto& shape : src->shapes)
            dst->addShape(std::move(shape));
        src->shapes.clear();
        removeLayer(index);
    }

    bool loadImageAsLayer(const std::string& path) {
        auto* surface = cairo_image_surface_create_from_png(path.c_str());
        if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
            cairo_surface_destroy(surface);
            return false;
        }
        int iw = cairo_image_surface_get_width(surface);
        int ih = cairo_image_surface_get_height(surface);
        cairo_surface_destroy(surface);

        auto imgShape = std::make_unique<ImageShape>();
        if (!imgShape->loadFromFile(path)) return false;
        imgShape->x = (width - imgShape->getWidth()) / 2;
        imgShape->y = (height - imgShape->getHeight()) / 2;

        size_t slash = path.find_last_of("/\\");
        std::string name = (slash == std::string::npos) ? path : path.substr(slash + 1);
        addLayer(name);
        activeLayer = layers.size() - 1;
        getActiveLayer()->addShape(std::move(imgShape));

        if (iw > width) width = iw;
        if (ih > height) height = ih;
        return true;
    }

    bool save(const std::string& path) {
        std::ofstream file(path);
        if (!file) return false;
        file << "LUNAR_PROJECT 1.0\n";
        file << "WIDTH=" << width << "\n";
        file << "HEIGHT=" << height << "\n";
        file << "ACTIVE_LAYER=" << activeLayer << "\n";
        file << "LAYER_COUNT=" << layers.size() << "\n";
        for (const auto& layer : layers)
            file << layer->serialize(0);
        file << "END_PROJECT\n";
        filename = path;
        return true;
    }

    bool load(const std::string& path) {
        std::ifstream file(path);
        if (!file) return false;
        layers.clear();
        std::string line;
        bool inLayer = false;
        std::string layerData;
        int layerDepth = 0;

        while (std::getline(file, line)) {
            if (line == "END_PROJECT") break;
            if (!inLayer) {
                if (line.find("WIDTH=") == 0) width = std::stoi(line.substr(6));
                else if (line.find("HEIGHT=") == 0) height = std::stoi(line.substr(7));
                else if (line.find("ACTIVE_LAYER=") == 0) activeLayer = std::stoi(line.substr(13));
                else if (line.find("LAYER_START") != std::string::npos) {
                    inLayer = true;
                    layerData = line + "\n";
                    layerDepth = 1;
                }
            } else {
                layerData += line + "\n";
                if (line.find("LAYER_START") != std::string::npos) layerDepth++;
                if (line.find("LAYER_END") != std::string::npos) {
                    layerDepth--;
                    if (layerDepth == 0) {
                        auto layer = Layer::deserialize(layerData);
                        if (layer) layers.push_back(std::move(layer));
                        inLayer = false;
                        layerData.clear();
                    }
                }
            }
        }
        if (layers.empty()) addLayer("Layer 1");
        if (activeLayer >= (int)layers.size()) activeLayer = layers.size() - 1;
        filename = path;
        return true;
    }

    bool exportPNG(const std::string& path) {
        cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
        cairo_t* cr = cairo_create(surface);
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_paint(cr);
        for (const auto& layer : layers) layer->render(cr);
        cairo_surface_write_to_png(surface, path.c_str());
        cairo_destroy(cr);
        cairo_surface_destroy(surface);
        return true;
    }

    bool exportJPG(const std::string& path) {
        std::string tmp = path + ".lunar_tmp.png";
        if (!exportPNG(tmp)) return false;
        std::string cmd = "convert \"" + tmp + "\" \"" + path + "\" 2>/dev/null";
        int ret = system(cmd.c_str());
        std::remove(tmp.c_str());
        return ret == 0;
    }
};
