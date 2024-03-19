#include "application.h"

#include "camera.h"
#include "skybox.h"
#include "terrain/terrainmanager.h"
#include "util.h"
#include <iostream>

#include <imgui.h>

#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

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

Camera camera = Camera(glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f),
    0.0f, 100000.0f, (float)windowWidth / (float)windowHeight,
    0.0f, -40.4f);

glm::vec3 skyColor = glm::vec3(162.0f, 193.0f, 215.0f) * (1.0f / 255.0f);
glm::vec3 terrainColor = glm::vec3(120.0f, 117.0f, 115.0f) * (1.0f / 255.0f);

glm::vec3 lightDirection = camera.front();
bool doFog = true;
float fogDensity = 0.006f;

float yScale = 1.0f / 2.0f;

Camera lastCam = camera;
bool freezeCam = false;
bool doWire = false;

void setup()
{
    setupGlfw();
    setupImGui();
}

void setupGlfw()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
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
}

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

void run()
{

    GLenum err = glewInit();

    if (err != GLEW_OK) {
        glfwTerminate();
        shutDown();
        std::exit(1);
    }

    // glFrontFace(GL_CCW);
    // glCullFace(GL_BACK);
    // glEnable(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    /* Load skybox */
    skybox = new Skybox();
    skybox->loadBuffers();
    skybox->loadTexture("../data/skybox/" + skyboxFolderName + "/");

    terrainManager = new TerrainManager();
    terrainManager->setup();

    std::cout << "Start render loop" << std::endl;

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

    camera.updateFrustum();

    /* Update window if resized */
    int wWidth, wHeight;
    glfwGetFramebufferSize(window, &wWidth, &wHeight);
    windowWidth = wWidth;
    windowHeight = wHeight;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // terrainManager->update();

    if (!freezeCam)
        lastCam = camera;

    glm::mat4 projection = glm::perspective(glm::radians(camera.zoom()), (float)windowWidth / (float)windowHeight, 0.1f, 100000.0f);
    glm::mat4 view = camera.getViewMatrix();

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));

    terrainManager->_terrainShader.use();
    terrainManager->_terrainShader.setMat4("projection", projection);
    terrainManager->_terrainShader.setMat4("view", view);
    terrainManager->_terrainShader.setVec3("cameraPos", lastCam.position());
    terrainManager->_terrainShader.setVec3("lightDirection", lightDirection);
    terrainManager->_terrainShader.setVec3("skyColor", skyColor);
    terrainManager->_terrainShader.setVec3("terrainColor", terrainColor);
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
    terrainManager->_skirtShader.setVec3("terrainColor", terrainColor);
    terrainManager->_skirtShader.setFloat("fogDensity", fogDensity);
    terrainManager->_skirtShader.setFloat("doFog", (float)doFog);
    terrainManager->_skirtShader.setFloat("yScale", yScale);
    terrainManager->_skirtShader.setMat4("model", model);
    terrainManager->_skirtShader.setFloat("useWire", (float)doWire);

    if (doWire) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    terrainManager->render(lastCam);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    skybox->shader().use();
    view = glm::mat4(glm::mat3(camera.getViewMatrix()));
    skybox->shader().setMat4("projection", projection);
    skybox->shader().setMat4("view", view);
    skybox->render();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);

    // std::cout << camera.position().x << ", " << camera.position().y << ", " << camera.position().z << std::endl;
}

void shutDown()
{
}

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
        }
    }
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    camera.aspectRatio((float)width / (float)height);
}

};
