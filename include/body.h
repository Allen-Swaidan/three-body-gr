#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

struct Body {
    glm::dvec3 position;
    glm::dvec3 velocity;
    double mass;           // in solar masses
    glm::vec4 color;
    float radius;

    // Trace path
    std::vector<glm::vec3> tracePath;
    static constexpr size_t MAX_TRACE_POINTS = 10000;

    void addTracePoint() {
        tracePath.push_back(glm::vec3(position));
        if (tracePath.size() > MAX_TRACE_POINTS)
            tracePath.erase(tracePath.begin());
    }
};

// GPU mesh for rendering a sphere
struct SphereMesh {
    GLuint vao = 0, vbo = 0, ebo = 0;
    int indexCount = 0;

    void init(int stacks = 24, int slices = 36);
    void draw() const;
    void cleanup();
};

// GPU buffers for trace lines
struct TraceRenderer {
    GLuint vao = 0, vbo = 0;
    int maxPoints;

    void init(int maxPts);
    void update(const std::vector<glm::vec3>& points);
    void draw(int count) const;
    void cleanup();
};
