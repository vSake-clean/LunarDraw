#pragma once
#include "shape.h"
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <cctype>
#include <locale>

static inline std::string ltrim(const std::string& s) {
    auto it = std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isspace(c); });
    return std::string(it, s.end());
}

class Layer {
public:
    std::string name;
    bool visible = true;
    double opacity = 1.0;
    std::vector<std::unique_ptr<Shape>> shapes;

    Layer() : name("Layer") {}
    explicit Layer(const std::string& name) : name(name) {}

    void addShape(std::unique_ptr<Shape> shape) {
        shapes.push_back(std::move(shape));
    }

    void render(cairo_t* cr) const {
        if (!visible) return;
        cairo_save(cr);
        if (opacity < 1.0) {
            cairo_push_group(cr);
        }
        for (const auto& shape : shapes) {
            shape->render(cr);
        }
        if (opacity < 1.0) {
            cairo_pop_group_to_source(cr);
            cairo_paint_with_alpha(cr, opacity);
        }
        cairo_restore(cr);
    }

    std::string serialize(int indent = 0) const {
        std::string pad(indent, ' ');
        std::ostringstream ss;
        ss << pad << "LAYER_START\n";
        ss << pad << "NAME=" << name << "\n";
        ss << pad << "VISIBLE=" << (visible ? "1" : "0") << "\n";
        ss << pad << "OPACITY=" << opacity << "\n";
        ss << pad << "SHAPE_COUNT=" << shapes.size() << "\n";
        for (const auto& shape : shapes) {
            ss << shape->serialize(indent + 1);
        }
        ss << pad << "LAYER_END\n";
        return ss.str();
    }

    static std::unique_ptr<Layer> deserialize(const std::string& data) {
        auto layer = std::make_unique<Layer>();
        std::istringstream ss(data);
        std::string line;
        int shapeCount = 0;
        std::string shapeData;
        bool inShape = false;
        int shapeDepth = 0;

        while (std::getline(ss, line)) {
            std::string tl = ltrim(line);
            if (tl.empty()) continue;
            if (!inShape) {
                if (tl.find("NAME=") == 0) layer->name = tl.substr(5);
                else if (tl.find("VISIBLE=") == 0) layer->visible = (tl.substr(8) == "1");
                else if (tl.find("OPACITY=") == 0) layer->opacity = std::stod(tl.substr(8));
                else if (tl.find("SHAPE_COUNT=") == 0) shapeCount = std::stoi(tl.substr(12));
                else if (tl == "SHAPE_START" || tl.find("SHAPE_START") == 0) {
                    inShape = true;
                    shapeData = line + "\n";
                    shapeDepth = 1;
                }
            } else {
                shapeData += line + "\n";
                std::string tl2 = ltrim(line);
                if (tl2.find("SHAPE_START") != std::string::npos) shapeDepth++;
                if (tl2.find("SHAPE_END") != std::string::npos) {
                    shapeDepth--;
                    if (shapeDepth == 0) {
                        auto shape = Shape::deserialize(shapeData);
                        if (shape) layer->addShape(std::move(shape));
                        inShape = false;
                        shapeData.clear();
                    }
                }
            }
        }
        return layer;
    }
};
