#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "shader.h"
#include "camera.h"
#include "body.h"
#include "simulation.h"
#include "spacetime_grid.h"

#include <iostream>
#include <array>

// ── Globals ───────────────────────────────────────────────────────────────────
static Camera camera;
static float lastX = 640.0f, lastY = 360.0f;
static bool firstMouse = true;
static bool cursorCaptured = false;
static float deltaTime = 0.0f;
static float lastFrame = 0.0f;

// ── Callbacks ─────────────────────────────────────────────────────────────────
static void framebufferSizeCallback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
}

static void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (!cursorCaptured) return;

    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }
    float xoff = (float)xpos - lastX;
    float yoff = lastY - (float)ypos;
    lastX = (float)xpos;
    lastY = (float)ypos;
    camera.processMouse(xoff, yoff);
}

static void scrollCallback(GLFWwindow*, double, double yoff) {
    camera.processScroll((float)yoff);
}

static void keyCallback(GLFWwindow* window, int key, int, int action, int) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        cursorCaptured = !cursorCaptured;
        glfwSetInputMode(window, GLFW_CURSOR,
                         cursorCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        firstMouse = true;
    }
}

static void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.processKeyboard(Camera::FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.processKeyboard(Camera::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.processKeyboard(Camera::LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.processKeyboard(Camera::RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) camera.processKeyboard(Camera::UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camera.processKeyboard(Camera::DOWN, deltaTime);
}

int main() {
    // ── Init GLFW ─────────────────────────────────────────────────────────────
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(1600, 900, "3-Body Problem - General Relativity", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2.0f);

    // ── ImGui ─────────────────────────────────────────────────────────────────
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450");

    // ── Shaders ───────────────────────────────────────────────────────────────
    Shader bodyShader("shaders/body.vert", "shaders/body.frag");
    Shader gridShader("shaders/grid.vert", "shaders/grid.frag");
    Shader traceShader("shaders/trace.vert", "shaders/trace.frag");

    // ── Scene objects ─────────────────────────────────────────────────────────
    Simulation sim;
    sim.initDefaultBodies();

    SphereMesh sphere;
    sphere.init();

    std::array<TraceRenderer, 3> traces;
    for (auto& t : traces) t.init(Body::MAX_TRACE_POINTS);

    SpacetimeGrid grid;
    grid.init(200.0f, 80);

    // UI state for position editing
    struct BodyUI {
        float pos[3];
        float vel[3];
        float mass;
    };
    std::array<BodyUI, 3> bodyUI;
    auto syncUIFromSim = [&]() {
        for (int i = 0; i < 3; ++i) {
            bodyUI[i].pos[0] = (float)sim.bodies[i].position.x;
            bodyUI[i].pos[1] = (float)sim.bodies[i].position.y;
            bodyUI[i].pos[2] = (float)sim.bodies[i].position.z;
            bodyUI[i].vel[0] = (float)sim.bodies[i].velocity.x;
            bodyUI[i].vel[1] = (float)sim.bodies[i].velocity.y;
            bodyUI[i].vel[2] = (float)sim.bodies[i].velocity.z;
            bodyUI[i].mass = (float)sim.bodies[i].mass;
        }
    };
    auto syncSimFromUI = [&]() {
        for (int i = 0; i < 3; ++i) {
            sim.bodies[i].position = glm::dvec3(bodyUI[i].pos[0], bodyUI[i].pos[1], bodyUI[i].pos[2]);
            sim.bodies[i].velocity = glm::dvec3(bodyUI[i].vel[0], bodyUI[i].vel[1], bodyUI[i].vel[2]);
            sim.bodies[i].mass = bodyUI[i].mass;
            sim.bodies[i].radius = std::cbrt((float)sim.bodies[i].mass) * 0.5f;
        }
    };
    syncUIFromSim();

    float timeScaleUI = 1.0f;
    bool showGrid = true;
    bool showTraces = true;

    // ── Main loop ─────────────────────────────────────────────────────────────
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Simulation step
        sim.step(deltaTime);

        // Update spacetime grid
        if (showGrid) grid.update(sim.bodies);

        // Update trace buffers
        for (int i = 0; i < 3; ++i) {
            traces[i].update(sim.bodies[i].tracePath);
        }

        // ── Render ────────────────────────────────────────────────────────────
        glClearColor(0.02f, 0.02f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (float)width / (float)height;
        glm::mat4 projection = glm::perspective(glm::radians(camera.fov), aspect, 0.1f, 1000.0f);
        glm::mat4 view = camera.viewMatrix();

        // Draw spacetime grid
        if (showGrid) {
            gridShader.use();
            gridShader.setMat4("view", view);
            gridShader.setMat4("projection", projection);
            glDepthMask(GL_FALSE);
            grid.draw();
            glDepthMask(GL_TRUE);
        }

        // Draw trace paths
        if (showTraces) {
            traceShader.use();
            traceShader.setMat4("view", view);
            traceShader.setMat4("projection", projection);
            for (int i = 0; i < 3; ++i) {
                glm::vec4 tc = sim.bodies[i].color;
                tc.a = 0.7f;
                traceShader.setVec4("traceColor", tc);
                traces[i].draw((int)sim.bodies[i].tracePath.size());
            }
        }

        // Draw bodies
        bodyShader.use();
        bodyShader.setMat4("view", view);
        bodyShader.setMat4("projection", projection);
        bodyShader.setVec3("viewPos", camera.position);

        for (int i = 0; i < 3; ++i) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(sim.bodies[i].position));
            model = glm::scale(model, glm::vec3(sim.bodies[i].radius));
            bodyShader.setMat4("model", model);
            bodyShader.setVec4("bodyColor", sim.bodies[i].color);
            sphere.draw();
        }

        // ── ImGui UI ──────────────────────────────────────────────────────────
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(340, 0), ImGuiCond_FirstUseEver);
        ImGui::Begin("3-Body GR Simulation");

        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Elapsed: %.2f s", sim.elapsedTime);
        ImGui::Separator();

        // Controls
        const char* bodyNames[] = { "Body 1 (Red)", "Body 2 (Blue)", "Body 3 (Green)" };
        ImVec4 bodyImColors[] = {
            ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
            ImVec4(0.3f, 0.5f, 1.0f, 1.0f),
            ImVec4(0.3f, 1.0f, 0.4f, 1.0f)
        };

        if (!sim.running) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Setup Mode - Configure bodies then Start");
            ImGui::Separator();

            for (int i = 0; i < 3; ++i) {
                ImGui::PushID(i);
                ImGui::TextColored(bodyImColors[i], "%s", bodyNames[i]);
                ImGui::DragFloat3("Position", bodyUI[i].pos, 0.5f, -100.0f, 100.0f, "%.1f");
                ImGui::DragFloat3("Velocity", bodyUI[i].vel, 0.1f, -20.0f, 20.0f, "%.2f");
                ImGui::DragFloat("Mass", &bodyUI[i].mass, 1.0f, 1.0f, 500.0f, "%.0f");
                ImGui::Spacing();
                ImGui::PopID();
            }

            if (ImGui::Button("Start Simulation", ImVec2(-1, 30))) {
                syncSimFromUI();
                sim.running = true;
            }
            if (ImGui::Button("Reset to Defaults", ImVec2(-1, 0))) {
                sim.initDefaultBodies();
                syncUIFromSim();
            }
        } else {
            for (int i = 0; i < 3; ++i) {
                ImGui::TextColored(bodyImColors[i], "%s: (%.1f, %.1f, %.1f)",
                    bodyNames[i],
                    sim.bodies[i].position.x,
                    sim.bodies[i].position.y,
                    sim.bodies[i].position.z);
            }
            ImGui::Separator();

            ImGui::SliderFloat("Time Scale", &timeScaleUI, 0.1f, 10.0f, "%.1fx");
            sim.timeScale = timeScaleUI;

            if (ImGui::Button("Pause")) sim.running = false;
            ImGui::SameLine();
            if (ImGui::Button("Resume")) sim.running = true;
            ImGui::SameLine();
            if (ImGui::Button("Reset")) {
                sim.reset();
                sim.initDefaultBodies();
                syncUIFromSim();
            }
        }

        ImGui::Separator();
        ImGui::Checkbox("Show Spacetime Grid", &showGrid);
        ImGui::Checkbox("Show Trace Paths", &showTraces);

        ImGui::Separator();
        ImGui::TextWrapped("Camera: WASD + Space/Shift to move. ESC to toggle mouse look. Scroll to zoom.");

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────
    sphere.cleanup();
    for (auto& t : traces) t.cleanup();
    grid.cleanup();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
