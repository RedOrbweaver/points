# Libraries used:
- glfw3, glew
- [glm](https://github.com/g-truc/glm)
- [ImGui](https://github.com/ocornut/imgui)
- [ImGuiFileDialog](https://github.com/aiekick/ImGuiFileDialog)

# Usage:
`points <(optional) list of files>`

Files can also be loaded from the "Files" menu.

# Building:
Make sure to initialize the submodules!:
`git submodule init; git submodule update`

Requires glfw3, glew as well as pkg-config.
For ubuntu run: 
`sudo apt install build-essential pkg-config libglew-dev liglfw3-dev`
