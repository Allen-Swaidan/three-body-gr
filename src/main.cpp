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
#include <cstring>
#include <fstream>
#include <filesystem>
#include <cstdio>

// ── Globals ───────────────────────────────────────────────────────────────────
static bool recordMode = false;
static int recordWidth = 960;
static int recordHeight = 540;
static int recordFPS = 30;
static float recordDuration = 15.0f;   // seconds of demo
static std::string recordDir = "frames";
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

static void saveFramePPM(const std::string& path, int w, int h, const std::vector<unsigned char>& pixels) {
    std::ofstream f(path, std::ios::binary);
    f << "P6\n" << w << " " << h << "\n255\n";
    // OpenGL gives bottom-up, flip vertically
    for (int y = h - 1; y >= 0; --y)
        f.write(reinterpret_cast<const char*>(&pixels[y * w * 3]), w * 3);
}

// Autopilot camera: orbits around the scene, rises and dips.
// Look target is offset to the right so the compact UI panel in the
// upper-left corner doesn't occlude the bodies.
static void autopilotCamera(float t, float duration) {
    float phase = t / duration; // 0..1
    constexpr float PI = 3.14159265f;

    // Orbit: 1.5 full rotations with radius and height variation
    float angle = phase * 2.0f * PI * 1.5f;
    float radius = 55.0f + 15.0f * std::sin(phase * PI * 2.0f);
    float height = 25.0f + 20.0f * std::sin(phase * PI * 3.0f);

    camera.position = glm::vec3(
        radius * std::cos(angle),
        height,
        radius * std::sin(angle)
    );

    // Look at a point offset to the right of screen-center so the action
    // sits in the right ~75% of the frame, away from the UI overlay
    glm::vec3 target(0.0f, -3.0f, 0.0f);
    glm::vec3 toTarget = glm::normalize(target - camera.position);
    glm::vec3 tempRight = glm::normalize(glm::cross(toTarget, camera.worldUp));
    // Nudge the target to the right in screen space
    target += tempRight * 12.0f;

    camera.front = glm::normalize(target - camera.position);
    camera.right = glm::normalize(glm::cross(camera.front, camera.worldUp));
    camera.up = glm::normalize(glm::cross(camera.right, camera.front));
}

int main(int argc, char* argv[]) {
    // Parse --record flag
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--record") == 0) recordMode = true;
        if (std::strcmp(argv[i], "--duration") == 0 && i + 1 < argc)
            recordDuration = std::stof(argv[++i]);
    }

    if (recordMode) {
        std::cout << "Recording mode: " << recordDuration << "s at " << recordFPS
                  << " FPS (" << recordWidth << "x" << recordHeight << ")\n";
        std::filesystem::create_directories(recordDir);
    }

    // ── Init GLFW ─────────────────────────────────────────────────────────────
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    int winW = recordMode ? recordWidth : 1600;
    int winH = recordMode ? recordHeight : 900;
    if (recordMode) glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // offscreen OK
    GLFWwindow* window = glfwCreateWindow(winW, winH, "3-Body Problem - General Relativity", nullptr, nullptr);
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
    if (recordMode) {
        // Smaller font and tighter padding for compact overlay
        ImGuiIO& io = ImGui::GetIO();
        io.FontGlobalScale = 0.9f;
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowPadding = ImVec2(8, 8);
        style.ItemSpacing = ImVec2(6, 3);
        style.FramePadding = ImVec2(4, 3);
    } else {
        // Normal mode: ensure full-size readable UI
        ImGuiIO& io = ImGui::GetIO();
        io.FontGlobalScale = 1.0f;
    }
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
    grid.init(2.5f, 120);  // 2.5 unit cells, 120 divisions = 300 unit span, follows camera

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

    // In record mode, auto-start simulation with higher time scale
    if (recordMode) {
        syncSimFromUI();
        sim.running = true;
        sim.timeScale = 2.0;
        timeScaleUI = 2.0f;
    }

    int frameIndex = 0;
    int totalFrames = (int)(recordDuration * recordFPS);
    float fixedDt = 1.0f / (float)recordFPS;
    float recordTime = 0.0f;
    std::vector<unsigned char> pixelBuffer;
    if (recordMode) pixelBuffer.resize(recordWidth * recordHeight * 3);

    // ── Main loop ─────────────────────────────────────────────────────────────
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = recordMode ? fixedDt : (currentFrame - lastFrame);
        lastFrame = currentFrame;

        if (recordMode) {
            if (frameIndex >= totalFrames) break;
            autopilotCamera(recordTime, recordDuration);
            recordTime += fixedDt;
        } else {
            processInput(window);
        }

        // Simulation step
        sim.step(deltaTime);

        // Update spacetime grid
        if (showGrid) grid.update(sim.bodies, camera.position);

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

        const char* bodyNames[] = { "Body 1 (Red)", "Body 2 (Blue)", "Body 3 (Green)" };
        ImVec4 bodyImColors[] = {
            ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
            ImVec4(0.3f, 0.5f, 1.0f, 1.0f),
            ImVec4(0.3f, 1.0f, 0.4f, 1.0f)
        };

        if (recordMode) {
            // Compact overlay pinned to upper-left corner
            ImGui::SetNextWindowPos(ImVec2(8, 8), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(200, 0), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.75f);
            ImGui::Begin("3-Body GR Simulation", nullptr,
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse);

            ImGui::Text("Elapsed: %.2f s", sim.elapsedTime);
            ImGui::Text("Time Scale: %.1fx", sim.timeScale);
            ImGui::Separator();
            for (int i = 0; i < 3; ++i) {
                ImGui::TextColored(bodyImColors[i], "%s", bodyNames[i]);
                ImGui::Text("  (%.1f, %.1f, %.1f)",
                    sim.bodies[i].position.x,
                    sim.bodies[i].position.y,
                    sim.bodies[i].position.z);
            }
        } else {
            // Full interactive UI
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(340, 0), ImGuiCond_Once);
            ImGui::Begin("3-Body GR Simulation");

            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
            ImGui::Text("Elapsed: %.2f s", sim.elapsedTime);
            ImGui::Separator();

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
        }

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // ── Record frame ──────────────────────────────────────────────────────
        if (recordMode) {
            glReadPixels(0, 0, recordWidth, recordHeight, GL_RGB, GL_UNSIGNED_BYTE, pixelBuffer.data());
            char fname[64];
            std::snprintf(fname, sizeof(fname), "frame_%05d.ppm", frameIndex);
            saveFramePPM(recordDir + "/" + fname, recordWidth, recordHeight, pixelBuffer);
            frameIndex++;
            if (frameIndex % 30 == 0)
                std::cout << "  Captured " << frameIndex << "/" << totalFrames << " frames\n";
        }

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
