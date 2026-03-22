#pragma once
#include "body.h"
#include <array>

// General Relativistic 3-body simulation using post-Newtonian approximation
// of the Einstein field equations (1PN order)
class Simulation {
public:
    static constexpr int NUM_BODIES = 3;
    static constexpr double C = 300.0;          // speed of light (simulation units)
    static constexpr double G = 1.0;            // gravitational constant

    std::array<Body, NUM_BODIES> bodies;
    bool running = false;
    double timeScale = 1.0;
    double elapsedTime = 0.0;
    int traceInterval = 3;      // add trace point every N steps

    void initDefaultBodies();
    void step(double dt);
    void reset();

private:
    int stepCounter = 0;

    // Compute acceleration on body i from all other bodies
    // using 1PN (first post-Newtonian) Einstein-Infeld-Hoffmann equations
    glm::dvec3 computeAcceleration(int i) const;

    // Newtonian acceleration component
    glm::dvec3 newtonianAccel(int i, int j) const;

    // 1PN general relativistic correction (Einstein-Infeld-Hoffmann)
    glm::dvec3 pnCorrection(int i, int j) const;

    // RK4 integration step
    void integrateRK4(double dt);
};

// Compute spacetime curvature (metric deviation) at a point
// Returns vertical displacement for grid visualization
double spacetimeCurvature(const glm::dvec3& point,
                          const std::array<Body, 3>& bodies,
                          double G, double C);
