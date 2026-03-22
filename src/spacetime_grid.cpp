#include "spacetime_grid.h"
#include <cmath>

void SpacetimeGrid::init(float size, int divisions) {
    gridSize = size;
    gridDivisions = divisions;
    float half = size * 0.5f;
    float step = size / (float)divisions;

    vertices.clear();
    std::vector<unsigned int> indices;

    for (int i = 0; i <= divisions; ++i) {
        for (int j = 0; j <= divisions; ++j) {
            Vertex v;
            v.basePosition = glm::vec3(
                -half + j * step,
                0.0f,
                -half + i * step
            );
            v.position = v.basePosition;
            v.curvature = 0.0f;
            vertices.push_back(v);
        }
    }

    // Create line indices for grid wireframe
    for (int i = 0; i <= divisions; ++i) {
        for (int j = 0; j < divisions; ++j) {
            // Horizontal lines
            indices.push_back(i * (divisions + 1) + j);
            indices.push_back(i * (divisions + 1) + j + 1);
            // Vertical lines
            indices.push_back(j * (divisions + 1) + i);
            indices.push_back((j + 1) * (divisions + 1) + i);
        }
    }

    indexCount = (int)indices.size();

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    // base position
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, basePosition));
    glEnableVertexAttribArray(1);
    // curvature
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, curvature));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void SpacetimeGrid::update(const std::array<Body, Simulation::NUM_BODIES>& bodies) {
    for (auto& v : vertices) {
        glm::dvec3 point(v.basePosition.x, 0.0, v.basePosition.z);
        double c = spacetimeCurvature(point, bodies, Simulation::G, Simulation::C);
        v.position = v.basePosition;
        v.position.y = (float)c;
        v.curvature = (float)std::abs(c);
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), vertices.data());
}

void SpacetimeGrid::draw() const {
    glBindVertexArray(vao);
    glDrawElements(GL_LINES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void SpacetimeGrid::cleanup() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
}
