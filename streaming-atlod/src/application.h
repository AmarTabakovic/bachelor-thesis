#ifndef APPLICATION_H
#define APPLICATION_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace Application {

struct GlobalState {
};

void parseArgs(int argc, char** argv);
void setup();
void setupGlfw();
void setupImGui();
void run();
void frame();
void shutDown();
void updateGlobalUniforms();
void processInput();
void keyboardInputCallback(GLFWwindow* window, int key, int scanCode, int action, int modifiers);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void cursorPositionCallback(GLFWwindow* window, double xPos, double yPos);
void framebufferSizeCallback(GLFWwindow* window, int width, int height);
}

#endif // APPLICATION_H
