#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "body.h"
#include "simulation.h"

class SpacetimeGrid {
public:
    void init(float cellSize, int divisions);
    void update(const std::array<Body, Simulation::NUM_BODIES>& bodies,
                const glm::vec3& cameraPos);
    void draw() const;
    void cleanup();

private:
    GLuint vao = 0, vbo = 0, ebo = 0;
    int gridDivisions;
    float cellSize;
    int indexCount = 0;

    struct Vertex {
        glm::vec3 position;
        float curvature;
    };

    std::vector<Vertex> vertices;
};
