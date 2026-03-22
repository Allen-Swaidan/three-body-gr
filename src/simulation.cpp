#include "simulation.h"
#include <cmath>
#include <glm/glm.hpp>

void Simulation::initDefaultBodies() {
    // Body 0: Red - heavy central body
    bodies[0].position = glm::dvec3(0.0, 0.0, 0.0);
    bodies[0].velocity = glm::dvec3(0.0, 0.0, -0.5);
    bodies[0].mass = 100.0;
    bodies[0].color = glm::vec4(1.0f, 0.2f, 0.2f, 1.0f);
    bodies[0].radius = 2.0f;

    // Body 1: Blue
    bodies[1].position = glm::dvec3(20.0, 0.0, 0.0);
    bodies[1].velocity = glm::dvec3(0.0, 0.0, 3.0);
    bodies[1].mass = 80.0;
    bodies[1].color = glm::vec4(0.2f, 0.4f, 1.0f, 1.0f);
    bodies[1].radius = 1.8f;

    // Body 2: Green
    bodies[2].position = glm::dvec3(-15.0, 0.0, 15.0);
    bodies[2].velocity = glm::dvec3(2.0, 0.0, -1.0);
    bodies[2].mass = 60.0;
    bodies[2].color = glm::vec4(0.2f, 1.0f, 0.3f, 1.0f);
    bodies[2].radius = 1.5f;

    for (auto& b : bodies) b.tracePath.clear();
    elapsedTime = 0.0;
    stepCounter = 0;
}

void Simulation::reset() {
    running = false;
    for (auto& b : bodies) b.tracePath.clear();
    elapsedTime = 0.0;
    stepCounter = 0;
}

// Einstein-Infeld-Hoffmann equations of motion (1PN approximation)
// These are derived from the Einstein field equations in the weak-field,
// slow-motion limit and capture the leading-order GR corrections:
//   - Perihelion precession
//   - Velocity-dependent gravitational interaction
//   - Non-linear gravitational self-interaction

glm::dvec3 Simulation::newtonianAccel(int i, int j) const {
    glm::dvec3 rij = bodies[j].position - bodies[i].position;
    double r = glm::length(rij);
    if (r < 0.1) r = 0.1; // softening
    return G * bodies[j].mass * rij / (r * r * r);
}

glm::dvec3 Simulation::pnCorrection(int i, int j) const {
    const Body& bi = bodies[i];
    const Body& bj = bodies[j];

    glm::dvec3 rij = bj.position - bi.position;
    double r = glm::length(rij);
    if (r < 0.1) r = 0.1;
    glm::dvec3 nij = rij / r;

    double vi2 = glm::dot(bi.velocity, bi.velocity);
    double vj2 = glm::dot(bj.velocity, bj.velocity);
    double vi_dot_vj = glm::dot(bi.velocity, bj.velocity);
    double nij_dot_vi = glm::dot(nij, bi.velocity);
    double nij_dot_vj = glm::dot(nij, bj.velocity);

    double c2 = C * C;
    double Gm_r = G * bj.mass / r;

    // Sum of Newtonian potentials from other bodies at position of body i
    double phi_i = 0.0;
    double phi_j = 0.0;
    for (int k = 0; k < NUM_BODIES; ++k) {
        if (k != i) {
            double rik = glm::length(bodies[k].position - bi.position);
            if (rik < 0.1) rik = 0.1;
            phi_i += G * bodies[k].mass / rik;
        }
        if (k != j) {
            double rjk = glm::length(bodies[k].position - bj.position);
            if (rjk < 0.1) rjk = 0.1;
            phi_j += G * bodies[k].mass / rjk;
        }
    }

    // 1PN EIH acceleration terms
    // Reference: Blanchet, "Gravitational Radiation from Post-Newtonian Sources"
    double A = 0.0;
    A += -vi2 / c2;
    A += -2.0 * vj2 / c2;
    A += 4.0 * vi_dot_vj / c2;
    A += 1.5 * nij_dot_vj * nij_dot_vj / c2;
    A += 5.0 * phi_i / c2;
    A += 4.0 * phi_j / c2;

    double B = 0.0;
    B += glm::dot(nij, 4.0 * bi.velocity - 3.0 * bj.velocity) / c2;

    glm::dvec3 correction = Gm_r / (r * r) * (A * rij + B * r * (bi.velocity - bj.velocity));

    // Gravitomagnetic / frame-dragging term (velocity-dependent force)
    correction += Gm_r / (r * r * c2) * (
        glm::dot(bi.velocity - bj.velocity, rij) * (bi.velocity - bj.velocity)
    ) * 0.5;

    return correction;
}

glm::dvec3 Simulation::computeAcceleration(int i) const {
    glm::dvec3 accel(0.0);
    for (int j = 0; j < NUM_BODIES; ++j) {
        if (j == i) continue;
        accel += newtonianAccel(i, j);
        accel += pnCorrection(i, j);
    }
    return accel;
}

void Simulation::integrateRK4(double dt) {
    struct State {
        glm::dvec3 pos, vel;
    };

    std::array<State, NUM_BODIES> s0;
    for (int i = 0; i < NUM_BODIES; ++i) {
        s0[i] = { bodies[i].position, bodies[i].velocity };
    }

    auto computeDerivs = [&](const std::array<State, NUM_BODIES>& state)
        -> std::array<std::pair<glm::dvec3, glm::dvec3>, NUM_BODIES>
    {
        // Temporarily set body states
        std::array<Body, NUM_BODIES> saved = bodies;
        for (int i = 0; i < NUM_BODIES; ++i) {
            bodies[i].position = state[i].pos;
            bodies[i].velocity = state[i].vel;
        }

        std::array<std::pair<glm::dvec3, glm::dvec3>, NUM_BODIES> derivs;
        for (int i = 0; i < NUM_BODIES; ++i) {
            derivs[i] = { state[i].vel, computeAcceleration(i) };
        }

        // Restore (const_cast needed since computeAcceleration is const)
        const_cast<Simulation*>(this)->bodies = saved;
        return derivs;
    };

    // k1
    std::array<State, NUM_BODIES> stmp;
    auto k1 = computeDerivs(s0);

    // k2
    for (int i = 0; i < NUM_BODIES; ++i) {
        stmp[i].pos = s0[i].pos + 0.5 * dt * k1[i].first;
        stmp[i].vel = s0[i].vel + 0.5 * dt * k1[i].second;
    }
    auto k2 = computeDerivs(stmp);

    // k3
    for (int i = 0; i < NUM_BODIES; ++i) {
        stmp[i].pos = s0[i].pos + 0.5 * dt * k2[i].first;
        stmp[i].vel = s0[i].vel + 0.5 * dt * k2[i].second;
    }
    auto k3 = computeDerivs(stmp);

    // k4
    for (int i = 0; i < NUM_BODIES; ++i) {
        stmp[i].pos = s0[i].pos + dt * k3[i].first;
        stmp[i].vel = s0[i].vel + dt * k3[i].second;
    }
    auto k4 = computeDerivs(stmp);

    // Combine
    for (int i = 0; i < NUM_BODIES; ++i) {
        bodies[i].position = s0[i].pos + (dt / 6.0) *
            (k1[i].first + 2.0 * k2[i].first + 2.0 * k3[i].first + k4[i].first);
        bodies[i].velocity = s0[i].vel + (dt / 6.0) *
            (k1[i].second + 2.0 * k2[i].second + 2.0 * k3[i].second + k4[i].second);
    }
}

void Simulation::step(double dt) {
    if (!running) return;

    dt *= timeScale;
    int substeps = 4;
    double subdt = dt / substeps;

    for (int s = 0; s < substeps; ++s) {
        integrateRK4(subdt);
    }

    stepCounter++;
    if (stepCounter % traceInterval == 0) {
        for (auto& b : bodies) b.addTracePoint();
    }
    elapsedTime += dt;
}

// Compute spacetime curvature for grid visualization
// Based on Schwarzschild metric: ds^2 = -(1 - rs/r)c^2 dt^2 + (1 - rs/r)^-1 dr^2 + ...
// The curvature (deflection) is proportional to the gravitational potential
double spacetimeCurvature(const glm::dvec3& point,
                          const std::array<Body, 3>& bodies,
                          double G, double C) {
    double totalCurvature = 0.0;
    for (const auto& body : bodies) {
        glm::dvec3 diff = point - body.position;
        // Project to XZ plane for grid
        double r = std::sqrt(diff.x * diff.x + diff.z * diff.z);
        if (r < 1.0) r = 1.0;

        // Schwarzschild radius: rs = 2GM/c^2
        double rs = 2.0 * G * body.mass / (C * C);

        // Metric deviation ~ -GM/(rc^2) gives the "well" depth
        // Amplified for visualization
        double curvature = -G * body.mass / (r * 0.5);

        // Make it fall off smoothly
        double falloff = std::exp(-r * 0.02);
        totalCurvature += curvature * (1.0 + 3.0 * rs / r) * falloff;
    }
    return totalCurvature * 0.15; // scale factor for visual
}
