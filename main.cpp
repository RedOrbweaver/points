#include "hmain.hpp"

using std::shared_ptr;
using std::vector;

const char* POPUP_LOAD_FAILED = "Failed to load file";
const char* POPUP_LOADING = "Loading";


GLFWwindow* window;
shared_ptr<OrbitCamera> camera;
int display_w, display_h; 
shared_ptr<PointProcessor> current_points = nullptr;
shared_ptr<PointProcessor> loading_points = nullptr;
vector<shared_ptr<PointProcessor>> open_points;
bool must_update_vbos = false;

vector<vec3<float>> section_colors = {vec3<float>{255, 0, 0}, vec3<float>{0, 255, 0}, vec3<float>{0, 0, 255}};
vector<float> sections = {-1, 1.5, 100000};

bool IsLoading()
{
    return loading_points != nullptr;
}
bool IsMouseOverGUI()
{
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse;
}

void HandleMouseScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    if(IsMouseOverGUI())
        return;
    camera->AddDistance(yoffset);
}

void SetCurrentPoints(shared_ptr<PointProcessor> points)
{
    current_points = points;
    must_update_vbos = true;
    camera->SetDistance(points->GetFurthestDistanceFromZero() * 2.0f);
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

    static GLuint point_buffer = 0;

    if(point_buffer != 0)
    {
        current_points->Lock();
        vector<size_t> indices = current_points->GetSectionIndices();
        current_points->Unlock();
        glBindBuffer(GL_ARRAY_BUFFER, point_buffer);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, NULL);
        size_t pos = 0;
        for(size_t i = 0; i < indices.size(); i++)
        {
            auto color = section_colors[i];
            glColor3f(color.x, color.y, color.z);
            glDrawArrays(GL_POINTS, pos, indices[i]);
            pos = indices[i];
        }
    }
    if(must_update_vbos && current_points != nullptr)
    {
        std::vector<vec3<float>> points;
        current_points->GetPointsSorted(&points);
        if(point_buffer != 0)
        {
            glDeleteBuffers(1, &point_buffer);
        }
        glGenBuffers(1, &point_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, point_buffer);
        glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(points[0]), &points[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

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

    bool loading_finished = false;
    if(IsLoading())
    {
        if(!ImGui::IsPopupOpen(POPUP_LOAD_FAILED))
        {
            loading_points->Lock();
            if(loading_points->HasFailedToLoad())
            {
                ImGui::OpenPopup(POPUP_LOAD_FAILED);
            }
            loading_finished = loading_points->IsLoaded();
            loading_points->Unlock();
            if(!ImGui::IsPopupOpen(POPUP_LOADING))
            {          
                ImGui::OpenPopup(POPUP_LOADING);
            }
            if(loading_finished)
            {
                open_points.push_back(loading_points);
                SetCurrentPoints(loading_points);
                loading_points = nullptr;
                current_points->Lock();
                current_points->SetSections(sections);
                current_points->Unlock();
            }
        }
    }
    if(ImGui::BeginPopupModal(POPUP_LOADING))
    {
        if(loading_finished)
        {
            ImGui::CloseCurrentPopup();
        }
        else
        {
            ImGui::Text("Loading file:");
            ImGui::Text("%s", loading_points->GetFilePath().c_str());
        }
        ImGui::EndPopup();
    }
    if(ImGui::BeginPopupModal(POPUP_LOAD_FAILED))
    {
        ImGui::Text("Failed to load file %s:", loading_points->GetFilePath().c_str());
        //ImGui::Text("%s", loading_points->GetLoadFailureError());
        ImGui::Separator();
        if(ImGui::Button("OK"))
        {
            loading_points = nullptr;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    static bool files_open = true;
    ImGui::Begin("Files", &files_open);
    {

    }
    ImGui::End();

    if(current_points != nullptr && !IsLoading())
    {
        static bool tools_open = true;
        ImGui::Begin("Tools", &tools_open);
        {
            
        }
        ImGui::End();
    }

    ImGui::ShowDemoWindow(nullptr);
    
    ImGui::Render();

    
}

void OpenFile(std::string path)
{
    loading_points = std::make_shared<PointProcessor>(path);
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Main code
int main(int argc, char** argv)
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
    glewInit();

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

    OpenFile("./test_data/data_1.txt");

    camera = std::make_shared<OrbitCamera>(60.0f, 0, 100, 16.0/9.0);
    camera->SetDistance(20);    
    

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

        
        if(io.MouseDown[0] && !IsMouseOverGUI())
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
