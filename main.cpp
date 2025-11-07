#include "hmain.hpp"

struct Point
{
    float x, y, z;
};
std::vector<Point> points;
GLFWwindow* window;
std::shared_ptr<OrbitCamera> camera;
int display_w, display_h; 
std::shared_ptr<PointProcessor> current_points = nullptr;

void HandleMouseScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    camera->AddDistance(yoffset);
}

void Render3D()
{
    glClearColor(0.1, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    auto view = camera->GetModelViewMatrix();
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(&view[0].x);
    
    
    auto perspective = camera->GetProjectionMatrix();
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(&perspective[0].x);        


    // glMatrixMode(GL_PROJECTION);
    // auto projection = camera->getProjectionViewMatrix();
    // glLoadMatrixd((GLdouble*)&projection[0]);

    glBegin(GL_POINTS);
    {
        for(size_t i = 0; i < points.size(); i++)
        {
            glVertex3d(points[i].x, points[i].y, points[i].z);
        }
    }
    glEnd();

    glBegin(GL_TRIANGLES);
    {
        glVertex3d(-10, 0, 0);
        glVertex3d(10, 0, 0);
        glVertex3d(0, 10, 5);
    }
    glEnd();
    
}

void RenderGUI()
{
    ImGui::NewFrame();

    
    ImGui::Text("%f", camera->GetDistance());
    ImGui::Text("%f", camera->GetPhi());
    ImGui::Text("%f", camera->GetTheta());

    if(current_points != nullptr || true)
    {
        static bool files_open = true;
        ImGui::Begin("Files", &files_open);
        {

        }
        ImGui::End();
    }
    
    ImGui::Begin("Tools");
    {
        ImGui::Text("");
    }
    ImGui::End();

    ImGui::ShowDemoWindow(nullptr);
    
    ImGui::Render();

    
}

void Load_Points(std::string path)
{
    FILE* file = fopen(path.c_str(), "r");
    while(true)
    {
        float x, y, z;
        int is_at_end = fscanf(file, "%f%f%f", &x, &y, &z);
        if(is_at_end == EOF)
            break;
        points.push_back({x, y, z});
    }
    fclose(file);
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Main code
int main(int, char**)
{

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
    window = glfwCreateWindow((int)(1280 * main_scale), (int)(800 * main_scale), "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    glfwSetScrollCallback(window, HandleMouseScroll);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    Load_Points("./test_data/data_1.txt");

    camera = std::make_shared<OrbitCamera>(60.0f, 0, 100, 16.0/9.0);
    camera->SetDistance(20);    
    

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
        glfwGetFramebufferSize(window, &display_w, &display_h);
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        
        if(io.MouseDown[0])
        {
            camera->Rotate(io.MouseDelta.x * 0.05f, io.MouseDelta.y * 0.05f);
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        RenderGUI();


        
        glViewport(0, 0, display_w, display_h);
        Render3D();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    

    return 0;
}
