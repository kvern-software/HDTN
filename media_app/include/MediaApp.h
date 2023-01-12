#pragma once

// std
#include <set>
#include <map>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

// graphical user interface
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
 


class MediaApp {
private:
    GLFWwindow* window;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
public:

    
    MediaApp();
    virtual ~MediaApp();

    // Gui related functions
    void NewFrame();
    void Render();
    void Close();
    void glfw_error_callback(int error, const char* description);
    
    void DisplayImage(); // loads image from local video device into imgui buffer
    void UpdateImage(boost::filesystem::path filePath); // if a new image has been delivered, load it
    bool LoadTextureFromFile(const char* filename); // loads image from storage (ie disk) into RAM
    bool LoadTextureFromVideoDevice(void * img_location, int size); //loads image from another location (like kernel) into our memory space
    void DataToOpenGLTexture(void * image_data, int image_width, int image_height); // converts raw memory into open GL viewable texture
    void ExitButton();

    void CopyFrame(void * location, uint64_t length);

    bool should_close = false;
    volatile bool should_update_image = false;
    bool copying = false;
    unsigned int file_number; // if we are saving frames to a local file, this is the  most recent file written to storage
    

    struct ImageData {
        int image_width;
        int image_height;
        GLuint image_texture = 0;

    } imageData;

    struct buffer {
        void * location = nullptr;
        int size;
    } rawFrameBuffer;
};
