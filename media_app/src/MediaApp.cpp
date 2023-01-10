#include "MediaApp.h"

#include "Logger.h"
#include <boost/make_unique.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/filesystem.hpp>
#include <string>

// image loading 
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;


MediaApp::MediaApp()
{
    // glfwSetErrorCallback((void *) glfw_error_callback);
    glfwInit();

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create window with graphics context
    window = glfwCreateWindow(1280, 720, "HDTN Media Stream", NULL, NULL);
    if (window == NULL)
        printf("null winfow");
        // return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.DeltaTime = 1/120; // 120 fps 
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
}

MediaApp::~MediaApp(){

}

// Start the Dear ImGui frame
void MediaApp::NewFrame() {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

}

void MediaApp::Render() {
    ImGui::Render();
    static int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}

// Close GUI, NOT networking
void MediaApp::Close() {
    LOG_INFO(subprocess) << "Closing GUI";
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    LOG_INFO(subprocess) << "Successfully closed GUI";
}

// void MediaApp::glfw_error_callback(int error, const char* description) {
//     fprintf(stderr, "Glfw Error %d: %s\n", error, description);
// }

bool MediaApp::LoadTextureFromVideoDevice(char* img_location, int size ) {
    int image_width = 0;
    int image_height = 0;

    char * image_data = (char *) stbi_load_from_memory(
        reinterpret_cast<unsigned char *>(img_location), size, &image_width, &image_height, NULL, 4);

    if (image_data == NULL) {
        LOG_ERROR(subprocess) << "Null image data";
        return false;
    }

    DataToOpenGLTexture(image_data, image_width, image_height);

    return true;
}

// Simple helper function to load an image into a OpenGL texture with common settings
bool MediaApp::LoadTextureFromFile(const char* filename) {
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    
    if (image_data == NULL) {
        LOG_ERROR(subprocess) << "Null image data";
        return false;
    }

    DataToOpenGLTexture(image_data, image_width, image_height);
    
    return true;
}

void MediaApp::DataToOpenGLTexture(void * image_data, int image_width, int image_height) {
     // Create a OpenGL texture identifier
    glGenTextures(1, &imageData.image_texture);
    glBindTexture(GL_TEXTURE_2D, imageData.image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    imageData.image_width = image_width;
    imageData.image_height = image_height;
}


void MediaApp::DisplayImage() {
    ImGui::SetNextWindowSize(ImVec2(1920,1080));
    ImGui::Begin("OpenGL Texture Text");
    ImGui::Text("pointer = %p", &imageData.image_texture);
    ImGui::Text("size = %d x %d", imageData.image_width, imageData.image_height);
    ImGui::Image((void*)(intptr_t)imageData.image_texture, ImVec2(imageData.image_width, imageData.image_height));
    ImGui::End();
}

void MediaApp::ExitButton() {
    ImGui::Begin("Exit Btn");
    if (ImGui::Button("Exit")) 
        should_close = true;
    ImGui::End();
}

/*
If a file has been processed by ProcessPayload and has been determined to be a 
supported filetype, the file loaded into RAM here. The previous file is deleted. 
Performing the update here prevents race conditions between the back end process 
and the GUI.
*/
void MediaApp::UpdateImage(boost::filesystem::path filePath) {
    if (should_update_image == true) {
        bool image_loaded = LoadTextureFromFile(filePath.c_str());
        // IM_ASSERT(image_loaded);
        if (image_loaded == true) 
            should_update_image = false;
    }
}

