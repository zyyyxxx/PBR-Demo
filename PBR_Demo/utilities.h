#pragma once
#ifndef UTILITIES_H
#define UTILITIES_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "camera.h"
#include "model.h"
#include "MainRenderProgram.h"

#include "imgui/imgui.h"
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <iostream>
#include <random>

// settings
extern const unsigned int SCR_WIDTH;
extern const unsigned int SCR_HEIGHT;

extern bool shadows ;

// camera
extern Camera camera;
extern float lastX;
extern float lastY;
extern bool firstMouse;

// timing
extern float deltaTime;
extern float lastFrame;

// »Øµ÷º¯Êý
void processInput(GLFWwindow* window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);


void renderSphere();
void renderCube();
void renderQuad();
void renderFloor();


// texture
unsigned int loadTexture(char const* path);

#endif