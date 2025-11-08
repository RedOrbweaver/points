#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuiFileDialog/ImGuiFileDialog.h"

#include "GL/glew.h"


#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers



#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <stdio.h>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <filesystem>
#include <thread>
#include <algorithm>

#include "RedCppLib/RedCppLib.hpp"

using namespace Red;

#include "camera.hpp"

#include "PointProcessor.hpp"