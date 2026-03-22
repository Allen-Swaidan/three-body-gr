#include "spacetime_grid.h"
#include <cmath>

void SpacetimeGrid::init(float cellSz, int divisions) {
    cellSize = cellSz;
    gridDivisions = divisions;

    int verts = (divisions + 1) * (divisions + 1);
    vertices.resize(verts);

    // Index buffer is constant topology — only vertex positions change
    std::vector<unsigned int> indices;
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
    glBufferData(GL_ARRAY_BUFFER, verts * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    // curvature
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, curvature));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void SpacetimeGrid::update(const std::array<Body, Simulation::NUM_BODIES>& bodies,
                           const glm::vec3& cameraPos) {
    // Snap grid center to nearest cell so lines don't swim as camera moves
    float half = (gridDivisions * cellSize) * 0.5f;
    float cx = std::floor(cameraPos.x / cellSize) * cellSize;
    float cz = std::floor(cameraPos.z / cellSize) * cellSize;

    for (int i = 0; i <= gridDivisions; ++i) {
        for (int j = 0; j <= gridDivisions; ++j) {
            int idx = i * (gridDivisions + 1) + j;
            float bx = cx - half + j * cellSize;
            float bz = cz - half + i * cellSize;

            glm::dvec3 point(bx, 0.0, bz);
            double c = spacetimeCurvature(point, bodies, Simulation::G, Simulation::C);

            vertices[idx].position = glm::vec3(bx, (float)c, bz);
            vertices[idx].curvature = (float)std::abs(c);
        }
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
