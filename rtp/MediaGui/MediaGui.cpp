#include "MediaGui.h"

#include "Logger.h"
#include <boost/make_unique.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/filesystem.hpp>
#include <string>


// image loading 
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;


MediaGui::MediaGui()
{
}

MediaGui::~MediaGui(){

}

void MediaGui::Init()
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
        printf("null window");
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

    // Create a OpenGL texture identifier for displaying images (textures)
    glGenTextures(1, &m_imageData.image_texture);
    glBindTexture(GL_TEXTURE_2D, m_imageData.image_texture);
    // Setup filtering parameters for displaying images
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same
    #if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    #endif
}
// Start the Dear ImGui frame
void MediaGui::NewFrame() {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

}

void MediaGui::Render() {
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
void MediaGui::Close() {
    LOG_INFO(subprocess) << "Closing GUI";
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    LOG_INFO(subprocess) << "Successfully closed GUI";
}

// void MediaGui::glfw_error_callback(int error, const char* description) {
//     fprintf(stderr, "Glfw Error %d: %s\n", error, description);
// }

bool MediaGui::LoadTextureFromVideoDevice(void* img_location, int size ) {
    if (!img_location)
        return false;
    
    int image_width = 0;
    int image_height = 0;
    // LOG_INFO(subprocess) << "LOADING IMAGE OF SIZE " << size;

    copying = true;
    char * image_data = (char *) stbi_load_from_memory(
        reinterpret_cast<unsigned char *>(img_location), size, &image_width, &image_height, NULL, 4);
    copying = false;

    if (image_data == NULL) {
        LOG_ERROR(subprocess) << "Null image data, failed to get data from VideoDriver";
        return false;
    }

    // LOG_INFO(subprocess) << "LOADED IMAGE OF DIMENSION " << image_width << " X " << image_height;

    DataToOpenGLTexture(image_data, image_width, image_height);
    // LOG_INFO(subprocess) << "IMG TO TEXTURE";

    stbi_image_free(image_data);

    return true;
}

// Simple helper function to load an image into a OpenGL texture with common settings
bool MediaGui::LoadTextureFromFile(const char* filename) {
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = nullptr; // = stbi_load(filename, &image_width, &image_height, NULL, 4);
    
    if (image_data == NULL) {
        LOG_ERROR(subprocess) << "Null image data";
        return false;
    }
    
    DataToOpenGLTexture(image_data, image_width, image_height);
    
    return true;
}

void MediaGui::DataToOpenGLTexture(void * image_data, int image_width, int image_height) {
    // Upload pixels into texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    m_imageData.image_width = image_width;
    m_imageData.image_height = image_height;
}


void MediaGui::DisplayImage() {
    ImGui::SetNextWindowSize(ImVec2(1920,1080));
    ImGui::Begin("OpenGL Texture Text");
    ImGui::Text("pointer = %p", &m_imageData.image_texture);
    ImGui::Text("size = %d x %d", m_imageData.image_width, m_imageData.image_height);
    ImGui::Image((void*)(intptr_t)m_imageData.image_texture, ImVec2(m_imageData.image_width, m_imageData.image_height));
    ImGui::End();
}

void MediaGui::ExitButton() {
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
void MediaGui::UpdateImage(boost::filesystem::path filePath) {
    if (should_update_image == true) {
        bool image_loaded = LoadTextureFromFile(filePath.c_str());
        // IM_ASSERT(image_loaded);
        if (image_loaded == true) 
            should_update_image = false;
    }
}


void MediaGui::ReceiveFrameCallback(buffer * img_buffer)
{
    LOG_INFO(subprocess) << "RECEIVE CALLBACK";
    LoadTextureFromVideoDevice(img_buffer->start, img_buffer->length);
}
