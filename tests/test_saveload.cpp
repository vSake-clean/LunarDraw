#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../src/project.h"

int main() {
    Project proj;
    proj.newProject(800, 600);

    auto* layer = proj.getActiveLayer();
    if (!layer) {
        printf("FAIL: No active layer\n");
        return 1;
    }

    auto rect = std::make_unique<RectShape>();
    rect->x = 100; rect->y = 100;
    rect->w = 200; rect->h = 150;
    rect->strokeColor = Color(1, 0, 0, 1);
    rect->fillColor = Color(0, 0, 1, 0.5);
    rect->strokeWidth = 3.0;
    rect->filled = true;
    layer->addShape(std::move(rect));

    auto circ = std::make_unique<CircleShape>();
    circ->cx = 400; circ->cy = 300;
    circ->rx = 80; circ->ry = 80;
    circ->strokeColor = Color(0, 1, 0, 1);
    circ->strokeWidth = 2.0;
    layer->addShape(std::move(circ));

    auto line = std::make_unique<LineShape>();
    line->x1 = 50; line->y1 = 50;
    line->x2 = 750; line->y2 = 550;
    line->strokeColor = Color(0, 0, 0, 1);
    line->strokeWidth = 1.0;
    layer->addShape(std::move(line));

    if (!proj.save("/tmp/test_lunar.lunar")) {
        printf("FAIL: Could not save project\n");
        return 1;
    }
    printf("PASS: Saved project\n");

    Project loaded;
    if (!loaded.load("/tmp/test_lunar.lunar")) {
        printf("FAIL: Could not load project\n");
        return 1;
    }
    printf("PASS: Loaded project\n");

    if (loaded.width != 800 || loaded.height != 600) {
        printf("FAIL: Dimensions mismatch: %dx%d vs 800x600\n", loaded.width, loaded.height);
        return 1;
    }
    printf("PASS: Dimensions correct\n");

    if (loaded.layers.size() != 2) {
        printf("FAIL: Layer count mismatch: %zu vs 2\n", loaded.layers.size());
        return 1;
    }
    printf("PASS: Layer count correct\n");

    auto* bgLayer = loaded.layers[0].get();
    auto* drawLayer = loaded.layers[1].get();

    if (drawLayer->shapes.size() != 3) {
        printf("FAIL: Shape count mismatch: %zu vs 3\n", drawLayer->shapes.size());
        return 1;
    }
    printf("PASS: Shape count correct\n");

    auto* r = dynamic_cast<RectShape*>(drawLayer->shapes[0].get());
    if (!r || r->x != 100 || r->y != 100 || r->w != 200 || r->h != 150) {
        printf("FAIL: Rect shape mismatch\n");
        return 1;
    }
    printf("PASS: Rect shape correct\n");

    auto* c = dynamic_cast<CircleShape*>(drawLayer->shapes[1].get());
    if (!c || c->cx != 400 || c->cy != 300 || c->rx != 80 || c->ry != 80) {
        printf("FAIL: Circle shape mismatch\n");
        return 1;
    }
    printf("PASS: Circle shape correct\n");

    auto* l = dynamic_cast<LineShape*>(drawLayer->shapes[2].get());
    if (!l || l->x1 != 50 || l->y1 != 50 || l->x2 != 750 || l->y2 != 550) {
        printf("FAIL: Line shape mismatch\n");
        return 1;
    }
    printf("PASS: Line shape correct\n");

    printf("\nAll tests passed!\n");
    std::remove("/tmp/test_lunar.lunar");
    return 0;
}
