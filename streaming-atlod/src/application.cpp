#include "application.h"

#include "camera.h"
#include "configmanager.h"
#include "renderstatistics.h"
#include "skybox.h"
#include "terrainmanager.h"
#include "terrainnode.h"
#include "util.h"

#include <glm/glm.hpp>
#include <iostream>

#include <imgui.h>

#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <curl/curl.h>

namespace Application {

/* Window variables */
const unsigned DEFAULT_SCREEN_WIDTH = 1280;
const unsigned DEFAULT_SCREEN_HEIGHT = 720;

unsigned windowWidth = DEFAULT_SCREEN_WIDTH;
unsigned windowHeight = DEFAULT_SCREEN_HEIGHT;

GLFWwindow* window;

/* FPS */
float deltaTime = 0.0f;
float lastFrame = 0.0f;

TerrainManager* terrainManager;
Skybox* skybox;
std::string skyboxFolderName = "simple-gradient"; /* Default skybox, can be overwritten */

Camera camera = Camera(glm::vec3(450.0f, 0.0f, 450.0f),
    glm::vec3(0.0f, 1.0f, 0.0f),
    0.01f, 1300.0f, (float)windowWidth / (float)windowHeight,
    -180.0f, -55.0f);

/*Camera camera = Camera(glm::vec3(-450.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f),
    800.0f, 0.01f, (float)windowWidth / (float)windowHeight,
    0.0f, 0.0f);*/

glm::vec3 skyColor = glm::vec3(162.0f, 193.0f, 215.0f) * (1.0f / 255.0f);
glm::vec3 terrainColor = glm::vec3(120.0f, 117.0f, 115.0f) * (1.0f / 255.0f);

glm::vec3 lightDirection = camera.front();
bool doFog = true;
float fogDensity = 0.04;

float yScale = 1.0f / 2.0f;

Camera lastCam = camera;
bool freezeCam = false;
bool doWire = false;
bool doAabb = false;

RenderStatistics globalRenderStats;
bool leftMouseButtonPressed = false;
double lastX = 0.0, lastY = 0.0;

std::string configPath;

void parseArgs(int argc, char** argv)
{
    if (argc >= 3) {
        std::cerr << "Too many arguments" << std::endl;
        std::exit(1);
    }

    if (argc <= 1) {
        std::cerr << "Please pass a config file path" << std::endl;
        std::exit(1);
    }

    configPath = std::string(argv[1]);
}

/**
 * @brief showSidebar
 */
void showSidebar()
{
    float wH = windowHeight; // ImGui::GetIO().DisplaySize.y;

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(300, wH), ImGuiCond_Once);

    if (ImGui::Begin("Sidebar", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::Text("Draw calls: %d", globalRenderStats.drawCalls);
        ImGui::Text("Rendered triangles: %d", globalRenderStats.renderedTriangles);
        ImGui::Text("Number of visible nodes: %d", globalRenderStats.visibleTiles);
        ImGui::Text("Number of traversed nodes: %d", globalRenderStats.traversedNodes);
        ImGui::Text("Number of requested nodes: %d", globalRenderStats.currentlyRequested);
        ImGui::Text("Number of allocated nodes: %d", globalRenderStats.numberOfNodes);
        ImGui::Text("Number of nodes in the disk cache: %d", globalRenderStats.numberOfDiskCacheEntries);
        ImGui::Text("API requests: %d", globalRenderStats.apiRequests);
        ImGui::Text("Lon/Lat/Height: : (%.2f, %.2f, %.2f)", glm::degrees(camera._lon), glm::degrees(camera._lat), camera._height);
        ImGui::Text("Wait for deallocation: %s", globalRenderStats.waitDealloc ? "true" : "false");
        ImGui::Text("Memory usage (nodes): %.2f MB", (float)globalRenderStats.numberOfNodes * ((512 * 512 * 3) + (512 * 512 * 4 * 1.33) + sizeof(TerrainNode)) / 1000000.0f);
        ImGui::Text("Deepest level: %d", globalRenderStats.deepestZoomLevel);
        ImGui::Text("Cam pos (WS): (%.2f, %.2f, %.2f)", camera.position().x, camera.position().y, camera.position().z);
        ImGui::Text("Cam front: %s", Util::vec3ToString(camera.front()).c_str());
        ImGui::Text("Cam up: %s", Util::vec3ToString(camera.up()).c_str());
        ImGui::Text("Cam right: %s", Util::vec3ToString(camera.right()).c_str());
    }
    ImGui::End();
}

void welcome()
{
    std::cout << "   _____ __                            _             ___  ________    ____  ____ " << std::endl;
    std::cout << "  / ___// /_________  ____ _____ ___  (_)___  ____ _/   |/_  __/ /   / __ \\/ __ \\" << std::endl;
    std::cout << "  \\__ \\/ __/ ___/ _ \\/ __ `/ __ `__ \\/ / __ \\/ __ `/ /| | / / / /   / / / / / / /" << std::endl;
    std::cout << " ___/ / /_/ /  /  __/ /_/ / / / / / / / / / / /_/ / ___ |/ / / /___/ /_/ / /_/ / " << std::endl;
    std::cout << "/____/\\__/_/   \\___/\\__,_/_/ /_/ /_/_/_/ /_/\\__, /_/  |_/_/ /_____/\\____/_____/  " << std::endl;
    std::cout << "                                           /____/                                 " << std::endl;
    std::cout << "StreamingATLOD version 1.0.0" << std::endl;
    std::cout << "2024 (c) Amar Tabakovic, data from Maptiler and OpenStreetMap contributors" << std::endl;
}

void setupCurl()
{
    curl_global_init(CURL_GLOBAL_ALL);
}

/**
 * @brief setup
 */
void setup()
{
    welcome();
    setupCurl();
    setupGlfw();
    setupImGui();

    ConfigManager::getInstance()->loadConfig(configPath);
}

/**
 * @brief setupGlfw
 */
void setupGlfw()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(windowWidth, windowHeight, "StreamingATLOD", NULL, NULL);

    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        std::exit(1);
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyboardInputCallback);
    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
}

/**
 * @brief setupImGui
 */
void setupImGui()
{

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
}

/**
 * @brief run
 */
void run()
{

    GLenum err = glewInit();

    if (err != GLEW_OK) {
        glfwTerminate();
        shutDown();
        std::exit(1);
    }

    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    /* Load skybox */
    skybox = new Skybox();
    skybox->loadBuffers();
    skybox->loadTexture("../../data/skybox/" + skyboxFolderName + "/");

    terrainManager = new TerrainManager(globalRenderStats, 16, 32, 32, 400, 2000);
    terrainManager->setup();

    while (!glfwWindowShouldClose(window)) {
        frame();
    }
}

/**
 * @brief frame
 *
 * Per frame ops:
 * - Update terrain
 *   - Check streaming clipmap
 *   - Load new tiles and free up memory for tiles out of clipmap
 * - Render terrain
 *   - Collect visible tiles and blocks
 *   - Update LODs
 *   - Render
 */
void frame()
{
    float verticalCollisionOffset = 0.0;
    glfwPollEvents();

    /* ImGui */
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    /* Update frame time */
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    processInput();

    /* Update window if resized */
    int wWidth, wHeight;
    glfwGetFramebufferSize(window, &wWidth, &wHeight);
    windowWidth = wWidth;
    windowHeight = wHeight;

    camera.updateFrustum();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!freezeCam)
        lastCam = camera;

    updateGlobalUniforms();

    /*if (doWire)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);*/

    bool collided = false;
    terrainManager->render(lastCam, doWire, doAabb, collided, verticalCollisionOffset);

    camera.verticalCollisionOffset = verticalCollisionOffset;
    if (collided) {
    }

    // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    skybox->shader().use();
    skybox->render();

    showSidebar();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
}

void updateGlobalUniforms()
{
    glm::mat4 projection = glm::perspective(glm::radians(camera.zoom()), (float)windowWidth / (float)windowHeight, 0.01f, 1300.0f);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 model = glm::mat4(1.0f);

    terrainManager->_terrainShader.use();
    terrainManager->_terrainShader.setMat4("projection", projection);
    terrainManager->_terrainShader.setMat4("view", view);
    terrainManager->_terrainShader.setVec3("cameraPos", lastCam.position());
    terrainManager->_terrainShader.setVec3("lightDirection", lightDirection);
    terrainManager->_terrainShader.setVec3("skyColor", skyColor);
    terrainManager->_terrainShader.setFloat("fogDensity", fogDensity);
    terrainManager->_terrainShader.setFloat("doFog", (float)doFog);
    terrainManager->_terrainShader.setFloat("yScale", yScale);
    terrainManager->_terrainShader.setMat4("model", model);
    terrainManager->_terrainShader.setFloat("useWire", (float)doWire);

    terrainManager->_skirtShader.use();
    terrainManager->_skirtShader.setMat4("projection", projection);
    terrainManager->_skirtShader.setMat4("view", view);
    terrainManager->_skirtShader.setVec3("cameraPos", lastCam.position());
    terrainManager->_skirtShader.setVec3("lightDirection", lightDirection);
    terrainManager->_skirtShader.setVec3("skyColor", skyColor);
    terrainManager->_skirtShader.setFloat("fogDensity", fogDensity);
    terrainManager->_skirtShader.setFloat("doFog", (float)doFog);
    terrainManager->_skirtShader.setFloat("yScale", yScale);
    terrainManager->_skirtShader.setMat4("model", model);
    terrainManager->_skirtShader.setFloat("useWire", (float)doWire);

    terrainManager->_poleShader.use();
    terrainManager->_poleShader.setMat4("projection", projection);
    terrainManager->_poleShader.setMat4("view", view);
    terrainManager->_poleShader.setVec3("cameraPos", lastCam.position());
    terrainManager->_poleShader.setVec3("lightDirection", lightDirection);
    terrainManager->_poleShader.setVec3("skyColor", skyColor);
    terrainManager->_poleShader.setFloat("fogDensity", fogDensity);
    terrainManager->_poleShader.setFloat("doFog", (float)doFog);
    terrainManager->_poleShader.setFloat("yScale", yScale);
    terrainManager->_poleShader.setMat4("model", model);
    terrainManager->_poleShader.setFloat("useWire", (float)doWire);

    terrainManager->_aabbShader.use();
    terrainManager->_aabbShader.setMat4("projection", projection);
    terrainManager->_aabbShader.setMat4("view", view);
    terrainManager->_aabbShader.setMat4("model", model);

    skybox->shader().use();
    view = glm::mat4(glm::mat3(camera.getViewMatrix()));
    skybox->shader().setMat4("projection", projection);
    skybox->shader().setMat4("view", view);
    skybox->shader().setVec3("cameraPos", lastCam.position());
}

/**
 * @brief shutDown
 */
void shutDown()
{
    curl_global_cleanup();
}

/**
 * @brief processInput
 */
void processInput()
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.processKeyboard(CameraAction::SPEED_UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(CameraAction::MOVE_FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(CameraAction::MOVE_BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(CameraAction::MOVE_LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(CameraAction::MOVE_RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.processKeyboard(CameraAction::MOVE_UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.processKeyboard(CameraAction::MOVE_DOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        camera.processKeyboard(CameraAction::LOOK_UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        camera.processKeyboard(CameraAction::LOOK_DOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        camera.processKeyboard(CameraAction::LOOK_LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        camera.processKeyboard(CameraAction::LOOK_RIGHT, deltaTime);
}

/**
 * @brief keyboardInputCallback
 * @param window
 * @param key
 * @param scanCode
 * @param action
 * @param modifiers
 */
void keyboardInputCallback(GLFWwindow* window, int key, int scanCode, int action, int modifiers)
{
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_F:
            freezeCam = !freezeCam;
            break;
        case GLFW_KEY_G:
            doWire = !doWire;
            break;
        case GLFW_KEY_L:
            lightDirection = camera.front();
            break;
        case GLFW_KEY_B:
            doAabb = !doAabb;
            break;
        }
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            leftMouseButtonPressed = true;
            glfwGetCursorPos(window, &lastX, &lastY);
        } else if (action == GLFW_RELEASE) {
            leftMouseButtonPressed = false;
        }
    }
}

void cursorPositionCallback(GLFWwindow* window, double xPos, double yPos)
{
    if (leftMouseButtonPressed) {
        float xOffset = xPos - lastX;
        float yOffset = lastY - yPos; // Reversed since y-coordinates go from bottom to top
        lastX = xPos;
        lastY = yPos;

        camera.processMouseMovement(xOffset, yOffset);
    }
}

/**
 * @brief framebufferSizeCallback
 * @param window
 * @param width
 * @param height
 */
void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    camera.aspectRatio((float)width / (float)height);
    camera.updateCameraVectors();
}

};
