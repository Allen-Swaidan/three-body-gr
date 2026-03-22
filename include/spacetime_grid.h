#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "body.h"
#include "simulation.h"

class SpacetimeGrid {
public:
    void init(float size, int divisions);
    void update(const std::array<Body, Simulation::NUM_BODIES>& bodies);
    void draw() const;
    void cleanup();

private:
    GLuint vao = 0, vbo = 0, ebo = 0;
    int gridDivisions;
    float gridSize;
    int indexCount = 0;

    struct Vertex {
        glm::vec3 position;
        glm::vec3 basePosition;  // unwarped position
        float curvature;         // for coloring
    };

    std::vector<Vertex> vertices;
};
