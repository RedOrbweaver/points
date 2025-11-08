#include "hmain.hpp"

using std::shared_ptr;
using std::vector;
using std::string;
using std::to_string;

const char* POPUP_LOAD_FAILED = "Failed to load file";
const char* POPUP_LOADING = "Loading";
const char* POPUP_OPEN_FILE = "Open file";
constexpr float CAMERA_MOVEMENT_RATE_X = 0.01f;
constexpr float CAMERA_MOVEMENT_RATE_Y = 0.01f;
constexpr float ZOOM_RATE = 2;
constexpr float SLICE_QUAD_SIZE = 20;


GLFWwindow* window;
shared_ptr<OrbitCamera> camera;
int display_w, display_h; 
shared_ptr<PointProcessor> current_points = nullptr;
vector<shared_ptr<PointProcessor>> loading_points;
vector<shared_ptr<PointProcessor>> open_points;
vector<shared_ptr<PointProcessor>> failed_to_load_points;
bool must_update_vbos = false;
bool slice_quads_enabled = false;
float slice_quads_opacity = 0.1f;


vector<vec3<float>> section_colors = {vec3<float>{1.0, 0, 0}, vec3<float>{0, 1.0, 0}, vec3<float>{0, 0, 1.0}};
vector<float> sections = {-1, 1.5, 100000};


void SetCurrentPoints(shared_ptr<PointProcessor> points)
{
    current_points = points;
    must_update_vbos = true;
    camera->SetDistance(points->GetFurthestDistanceFromZero() * 2.0f);
}

void Render3D()
{
    glClearColor(0.05, 0.05, 0.05, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Compute view matrices
    auto view = camera->GetModelViewMatrix();
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(&view[0].x);
    auto perspective = camera->GetProjectionMatrix();
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(&perspective[0].x);        

    static GLuint point_buffer = 0;

    if(current_points != nullptr)
    {
        // Upload data to the GPU
        if(must_update_vbos)
        {
            vector<vec3<float>> points;
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

        // Render points
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

        // Render section slices
        if(slice_quads_enabled)
        {
            float pos = 0.0f;
            for(int i = 0; i < sections.size(); i++)
            {
                pos += sections[i];
                vec3<float> p0 = {-SLICE_QUAD_SIZE, -SLICE_QUAD_SIZE, pos};
                vec3<float> p1 = {SLICE_QUAD_SIZE, -SLICE_QUAD_SIZE, pos};
                vec3<float> p2 = {SLICE_QUAD_SIZE, SLICE_QUAD_SIZE, pos};
                vec3<float> p3 = {-SLICE_QUAD_SIZE, SLICE_QUAD_SIZE, pos};
                auto color = section_colors[i];
                glBegin(GL_TRIANGLES);
                {
                    glColor4f(color.x, color.y, color.z, slice_quads_opacity);
                    glVertex3f(p0.x, p0.y, p0.z);
                    glVertex3f(p1.x, p1.y, p1.z);
                    glVertex3f(p2.x, p2.y, p2.z);

                    glVertex3f(p2.x, p2.y, p2.z);
                    glVertex3f(p3.x, p3.y, p3.z);
                    glVertex3f(p0.x, p0.y, p0.z);
                }
                glEnd();
            }
        }
    }    
}

void OpenFile(string path)
{
    loading_points.push_back(std::make_shared<PointProcessor>(path));
}

bool IsLoading()
{
    return loading_points.size() > 0 || failed_to_load_points.size() > 0;
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
    camera->AddDistance(yoffset * ZOOM_RATE);
}

string BytesToReadableString(size_t bytes)
{
    static vector<string> prefixes = 
    {
        "B", "KB", "MB", "GB", "PB", "EB"
    };
    int pos = 0;
    while(bytes > 10000)
    {
        bytes /= 1000;
        pos++;
    }
    assert(pos < prefixes.size());
    return to_string(bytes) + prefixes[pos];
}

void RenderToolsWindow()
{
    // for brevity
    auto cp = current_points;

    if(ImGui::Begin("Tools", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Info:");
        ImGui::Text("File size: %s", BytesToReadableString(cp->GetFileSize()).c_str());
        ImGui::SameLine();
        ImGui::Text("Memory used: %s", BytesToReadableString(cp->GetMemoryUsage()).c_str());
        ImGui::Text("Points: %lu", cp->GetNPoints());
        ImGui::Separator();
        static int csi = 1;
        ImGui::RadioButton("Center of points", &csi, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Average", &csi, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Zero", &csi, 2);
        if(csi == 0)
            camera->SetCenter(cp->GetCenterBounding());
        else if(csi == 1)
            camera->SetCenter(cp->GetCenterAverage());
        else if(csi == 2)
            camera->SetCenter({0.0f, 0.0f, 0.0f});
        ImGui::Separator();
        ImGui::Checkbox("Separators", &slice_quads_enabled);
        ImGui::SliderFloat("Separator opacity", &slice_quads_opacity, 0.0f, 1.0f);
        ImGui::Separator();
        ImGui::Text("Sections:");
        size_t section_pos = 0;
        auto indices = cp->GetSectionIndices();
        int to_remove = -1;
        for(int i = 0; i < indices.size(); i++)
        {
            ImGui::Text("(%i) points: %u", i, (indices[i] == 0) ? 0 : uint(indices[i]-section_pos));
            ImGui::SameLine();
            vec3<float> color = section_colors[i];
            ImGui::ColorEdit3((string("Color##") + to_string(i)).c_str(), color.data);
            section_colors[i] = color;
            if(sections.size() > 2)
            {
                ImGui::SameLine();
                if(ImGui::SmallButton((string("Remove##") + to_string(i)).c_str()))
                {
                    to_remove = i;
                }
            }
            // last section goes out to infinity, so we ignore it
            if(i != indices.size()-1)
            {
                float v = sections[i];
                // first element can go into the negatives up to the point with the lowest z-axis
                float lower_limit = (i == 0) ? cp->GetBoundingBoxLow().z : 0.0f;
                float upper_limit = cp->GetBoundingBoxHigh().z;
                ImGui::SliderFloat((string("Length##") + to_string(i)).c_str(), &v, 
                    lower_limit, upper_limit);
                if(v != sections[i])
                {
                    sections[i] = v;
                    cp->SetSections(sections);
                }
            }
            section_pos = indices[i];
        }
        if(to_remove > 0)
        {
            sections.erase(sections.begin() + to_remove);
            section_colors.erase(section_colors.begin() + to_remove);
            // last section goes out to infity
            sections[sections.size()-1] = 10000.0f;
            cp->SetSections(sections);
        }
        if(ImGui::Button("Add"))
        {
            sections[sections.size()-1] = sections[sections.size()-2] * 2;
            sections.push_back(10000.0f);
            section_colors.push_back({1.0f, 1.0f, 1.0f});
            cp->SetSections(sections);
        }
    }
    ImGui::End();
}

void RenderFilesWindow()
{
    ImVec2 size_min = {200.0f, 350.0f};
    // no constraint
    ImVec2 size_max = {FLT_MAX, FLT_MAX};
    ImGui::SetNextWindowSizeConstraints(size_min, size_max);

    if(ImGui::Begin("Files"))
    {
        if(open_points.size() > 0)
        {
            ImGui::Text("Loaded files:");
            if(ImGui::BeginListBox("##", ImVec2(250, 300)))
            {
                vector<std::shared_ptr<PointProcessor>> to_delete;
                static int points_selected = 0;
                for(int i = 0; i < open_points.size(); i++)
                {
                    auto p = open_points[i];
                    string sid = p->GetFileName() + "##" + to_string(i);
                    const bool is_selected = (points_selected == i);
                    if(ImGui::Selectable(sid.c_str(), is_selected, ImGuiSelectableFlags_AllowOverlap))
                    {
                        points_selected = i;
                    }
                    ImGui::SameLine(232);
                    string bid = "X##" + to_string(i);
                    if(ImGui::SmallButton(bid.c_str()))
                    {
                        to_delete.push_back(p);
                    }
                    
                }
                ImGui::EndListBox();
                if(points_selected < open_points.size() && open_points[points_selected] != current_points)
                    SetCurrentPoints(open_points[points_selected]);
                for(int i = 0; i < to_delete.size(); i++)
                {
                    for(int ii = 0; ii < open_points.size(); ii++)
                    {
                        if(open_points[ii] == to_delete[i])
                            open_points.erase(open_points.begin() + ii);
                    }
                    if(to_delete[i] == current_points)
                        current_points = nullptr;
                }
                if(current_points == nullptr && open_points.size() > 0)
                    SetCurrentPoints(open_points[0]);
            }
        }
        else
            ImGui::Text("No files");
        if(ImGui::Button("Open"))
        {
            IGFD::FileDialogConfig config;
            config.path = ".";
            ImGuiFileDialog::Instance()->OpenDialog(POPUP_OPEN_FILE, "Choose File", ".*", config);
        }
        if (ImGuiFileDialog::Instance()->Display(POPUP_OPEN_FILE)) 
        {
            if (ImGuiFileDialog::Instance()->IsOk()) 
            { 
                string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
                bool isalreadyopen = false;
                for(auto it : open_points)
                {
                    if(it->GetFilePath() == filePath)
                    {
                        isalreadyopen = true;
                        break;
                    }
                }
                if(!isalreadyopen && std::filesystem::is_regular_file(filePath))
                    OpenFile(filePath);
                ImGuiFileDialog::Instance()->Close();
            }
        }
    }
    ImGui::End();
}

void RenderLoadingPopups()
{
    bool loading_finished = false;
    if(IsLoading())
    {
        if(!ImGui::IsPopupOpen(POPUP_LOAD_FAILED))
        {
            int completed = 0;
            int failed = 0;
            for(auto it : loading_points)
            {
                it->Lock();
                if(it->HasFailedToLoad())
                {
                    failed++;
                    completed++;
                }
                else if(it->IsLoaded())
                {
                    completed++;
                }
                it->Unlock();
            }
            if(!ImGui::IsPopupOpen(POPUP_LOADING))
            {          
                ImGui::OpenPopup(POPUP_LOADING);
            }
            if(completed == loading_points.size())
            {
                for(auto it : loading_points)
                {
                    if(!it->HasFailedToLoad())
                    {                
                        open_points.push_back(it);
                        if(it == loading_points[0])
                            SetCurrentPoints(it);
                        it->Lock();
                        it->SetSections(sections);
                        it->Unlock();
                    }
                    else
                    {
                        failed_to_load_points.push_back(it);
                    }
                }
                loading_finished = true;
                if(failed > 0)
                    ImGui::OpenPopup(POPUP_LOAD_FAILED);
            }
        }
    }
    if(ImGui::BeginPopupModal(POPUP_LOADING))
    {
        if(loading_finished)
        {
            loading_points.clear();
            ImGui::CloseCurrentPopup();
        }
        else
        {
            if(loading_points.size() == 1)
                ImGui::Text("Loading file: %s", loading_points[0]->GetFileName().c_str());
            else
            {
                ImGui::Text("Loading %i files", (int)loading_points.size());
            }
            float loading_state = 0.0f;
            for(auto it : loading_points)
            {
                it->Lock();
                loading_state += it->LoadingState();
                it->Unlock();
            }
            loading_state /= loading_points.size();
            // :)
            if(loading_state > 0.99)
                loading_state = 0.99;
            ImGui::ProgressBar(loading_state, ImVec2(0.0f, 0.0f));
        }
        ImGui::EndPopup();
    }
    if(ImGui::BeginPopupModal(POPUP_LOAD_FAILED))
    {
        assert(failed_to_load_points.size() > 0);
        if(failed_to_load_points.size() == 1)        
            ImGui::Text("Loading failed");
        else if(failed_to_load_points.size() < loading_points.size())
            ImGui::Text("(%i/%i) files failed to load",  (int)failed_to_load_points.size(), (int)loading_points.size());
        else
            ImGui::Text("All files failed to load");
        for(auto it : failed_to_load_points)
        {
            ImGui::Text("Failed to load file %s: %s", it->GetFilePath().c_str(), it->GetLoadFailureError().c_str());
        }
        //ImGui::Text("%s", loading_points->GetLoadFailureError());
        ImGui::Separator();
        if(ImGui::Button("OK"))
        {
            loading_points.clear();
            failed_to_load_points.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void RenderGUI()
{
    ImGui::NewFrame();

    // Camera debug info:
    // ImGui::Text("%f", camera->GetDistance());
    // ImGui::Text("%f", camera->GetPhi());
    // ImGui::Text("%f", camera->GetTheta());

    RenderLoadingPopups();

    RenderFilesWindow();

    if(current_points != nullptr && !IsLoading())
        RenderToolsWindow();

    // Demo window for figuring out how ImGui works
    //ImGui::ShowDemoWindow(nullptr);
    
    ImGui::Render();

    
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Main code
int main(int argc, char** argv)
{
    printf("Starting points");
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
    window = glfwCreateWindow((int)(1500 * main_scale), (int)(1000 * main_scale), "POINTS!", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    glewInit();

    glfwSetScrollCallback(window, HandleMouseScroll);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

    // OpenFile("./test_data/data_1.txt");
    // OpenFile("./test_data/data_2.txt");
    // OpenFile("./test_data/data_3.txt");
    // OpenFile("./test_data/data_4.txt");

    //Initialize camera with sane defaults;
    camera = std::make_shared<OrbitCamera>(50.0f, 0, 10000, 16.0/9.0);
    camera->SetDistance(20);    

    if(argc > 1)
    {
        for(int i = 1; i < argc; i++)
        {
            OpenFile(argv[i]);
        }
    }
    

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

        camera->SetAspectRatio(float(display_w)/float(display_h));

        if(io.MouseDown[0] && !IsMouseOverGUI())
        {
            camera->Rotate(io.MouseDelta.x * CAMERA_MOVEMENT_RATE_X, io.MouseDelta.y * CAMERA_MOVEMENT_RATE_Y);
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
