#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>

#include <yaml-cpp/yaml.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <utility>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>

namespace YAML {
template<>
struct convert<glm::vec2> {
  static Node encode(const glm::vec2& rhs) {
    Node node;
    node.SetStyle(EmitterStyle::Flow);
    node.push_back(rhs.x);
    node.push_back(rhs.y);
    return node;
  }

  static bool decode(const Node& node, glm::vec2& rhs) {
    if (!node.IsSequence() || node.size() != 2) {
      return false;
    }

    rhs.x = node[0].as<float>();
    rhs.y = node[1].as<float>();
    return true;
  }
};
}// namespace YAML

static void glfw_error_callback(int error, const char* description) {
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
  glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

// fucking around with window abstraction, this stuff actually works pretty well, need to do docking stuff next.

struct WindowDef {
  std::string Name;
  std::function<void(void)> Render;

  WindowDef(std::string name, std::function<void(void)> render) : Name(std::move(name)), Render(std::move(render)) {}
};

struct Window {
  std::string Name;
  bool Visible;
  bool Docked;
  glm::vec2 Position;
  glm::vec2 Size;
  std::function<void(void)> Render;

  Window(std::string name, bool visible, bool docked, glm::vec2 pos, glm::vec2 size, std::function<void(void)> render)
      : Name(std::move(name)), Visible(visible), Docked(docked), Position(pos), Size(size), Render(std::move(render)) {
  }
};

struct Application {
  YAML::Node Config;
  glm::vec2 Position;
  glm::vec2 Size;
  std::vector<WindowDef> WindowDefinitions;
  std::vector<Window> Windows;
  bool ResetLayout = true;
  bool ListDockNodes = false;

  Application() {
    // If config exists, load it
    if (std::filesystem::exists("workspace.yaml")) {
      Config = YAML::LoadFile("workspace.yaml");
    } else {
      Config = YAML::LoadFile("default_workspace.yaml");
    }

    // Define pertinent windows
    WindowDefinitions.emplace_back("Window 1", [&] { return DrawWindow1(); });
    WindowDefinitions.emplace_back("Window 2", [&] { return DrawWindow2(); });
    WindowDefinitions.emplace_back("Window 3", [&] { return DrawWindow3(); });

    // Read state from persistent config and create windows
    for (auto& w : WindowDefinitions) {
      Windows.emplace_back(
        w.Name,
        Config["windows"][w.Name]["visible"].as<bool>(false),
        Config["windows"][w.Name]["docked"].as<bool>(false),
        Config["windows"][w.Name]["position"].as<glm::vec2>(glm::vec2(0.0f)),
        Config["windows"][w.Name]["size"].as<glm::vec2>(glm::vec2(100.0f)),
        w.Render);
    }
  }

  ~Application() {
    SaveLayout();
  }

  // TODO: OnSizeChanged
  void OnPositionChanged(GLFWwindow* window, int xpos, int ypos) {

  }

  void SaveLayout() {
    for (const auto& w : Windows) {
      Config["windows"][w.Name]["visible"] = w.Visible;
      Config["windows"][w.Name]["docked"] = w.Docked;
      Config["windows"][w.Name]["position"] = w.Position;
      Config["windows"][w.Name]["size"] = w.Size;
    }

    // Write config to file
    std::ofstream fout("workspace.yaml");
    fout << Config;
  }

  void Render() {
    ImGuiIO& io = ImGui::GetIO();

    ImGuiDockNodeFlags dockspace_flags = 0;

    auto vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->GetWorkPos());
    ImGui::SetNextWindowSize(vp->GetWorkSize());
    ImGui::SetNextWindowViewport(vp->ID);

    ImGuiWindowFlags host_window_flags = 0;
    host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
    host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
      host_window_flags |= ImGuiWindowFlags_NoBackground;

    char label[32];
    ImFormatString(label, IM_ARRAYSIZE(label), "DockSpaceViewport_%08X", vp->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(label, nullptr, host_window_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspaceID = ImGui::GetID("Dock");
    ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), dockspace_flags, nullptr);

    if (ResetLayout) {
      std::cout << "Resetting Layout" << std::endl;
      ImGui::DockBuilderRemoveNode(dockspaceID);
      ImGui::DockBuilderAddNode(dockspaceID, 1 << 10);           // tbd figure out this flag
      ImGui::DockBuilderSetNodeSize(dockspaceID, io.DisplaySize);// full size of window

      ImGuiID left;
      ImGuiID right;
      ImGui::DockBuilderSplitNode(dockspaceID, ImGuiDir_Left, 0.25f, &left, &right);
      ImGui::DockBuilderDockWindow("Window 1", left);
      ImGui::DockBuilderDockWindow("Window 2", left);
      ImGui::DockBuilderDockWindow("Window 3", right);
      ImGui::DockBuilderFinish(dockspaceID);
      ResetLayout = false;
    }

    if (ListDockNodes) {
      std::cout << "Listing dock nodes...\n";

      ListDockNodes = false;
      std::cout << "Finished listing dock nodes!\n";
    }

    ImGui::End();

    for (auto& w : Windows) {
      if (w.Visible) {
        // TODO: Save dock nodes to configuration
        if (!w.Docked) {// If window is docked size and position will be set from the dock nodes
          ImGui::SetNextWindowPos({w.Position.x, w.Position.y}, ImGuiCond_Once);
          ImGui::SetNextWindowSize({w.Size.x, w.Size.y}, ImGuiCond_Once);
        }

        ImGui::Begin(w.Name.c_str(), &w.Visible);
        // Read state back into the window list
        auto [x, y] = ImGui::GetWindowPos();
        auto [width, height] = ImGui::GetWindowSize();
        w.Docked = ImGui::IsWindowDocked();
        w.Position = {x, y};
        w.Size = {width, height};

        w.Render();

        ImGui::End();
      }
    }

    if (io.WantSaveIniSettings) {
      SaveLayout();
      io.WantSaveIniSettings = false;
    }
  }

  void DrawWindow1() {
    ImGui::Text("Hello 1!");
    if (ImGui::Button("List Dock Nodes")) {
      ListDockNodes = true;
    }
  }

  void DrawWindow2() {
    ImGui::Text("Hello 2!");
  }

  void DrawWindow3() {
    ImGui::Text("Hello 3!");
  }
};

// TODO: Main window position saved to config
int main(int argc, char** argv) {

  Application application;

  application.Position = {50.0f, 50.0f};
  application.Size = {800.0f, 600.0f};
  // TODO: Maximized

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  // glfwWindowHint(GLFW_DECORATED, false); // Not used, just here for illustration

  GLFWwindow* window = glfwCreateWindow(application.Size.x, application.Size.y, "GUI Sample", nullptr, nullptr);
  if (window == nullptr) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }

  glfwSetWindowPos(window, application.Position.x, application.Position.y);
  glfwSetWindowSize(window, application.Size.x, application.Size.y);

  glfwMakeContextCurrent(window);
  glfwSwapInterval(0);// Disable vsync

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;// Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;    // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // Enable Multi-Viewport / Platform Windows

  ImGui::StyleColorsDark();
  ImGuiStyle& style = ImGui::GetStyle();
  style.WindowRounding = 0.0f;
  style.Colors[ImGuiCol_WindowBg].w = 1.0f;

  if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 450");

  while (!glfwWindowShouldClose(window)) {
    processInput(window);

    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    application.Render();
    ImGui::Render();
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    GLFWwindow* backup_current_context = glfwGetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(backup_current_context);

    glfwSwapBuffers(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
