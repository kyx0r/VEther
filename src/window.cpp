#include "window.h"

GLFWwindow* window;

void initWindow() 
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(640, 480, "VEther", nullptr, nullptr);
}

void mainLoop() 
{
    while (!glfwWindowShouldClose(window))
	{
        glfwPollEvents();
    }
}