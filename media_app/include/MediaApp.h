#pragma once

// std
#include <set>
#include <map>

// HDTN
#include "app_patterns/BpSinkPattern.h"
#include "codec/Cbhe.h"
#include "LtpFragmentSet.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

// graphical user interface
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
 


class MediaApp : public BpSinkPattern {
private:
    GLFWwindow* window;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
public:
    struct SendFileMetadata {
        SendFileMetadata() : totalFileSize(0), fragmentOffset(0), fragmentLength(0), pathLen(0) {}
        uint64_t totalFileSize;
        uint64_t fragmentOffset;
        uint32_t fragmentLength;
        uint8_t pathLen;
        uint8_t unused1;
        uint8_t unused2;
        uint8_t unused3;
    };

    struct ImageData {
        int image_width;
        int image_height;
        GLuint image_texture = 0;
    } imageData;

    typedef std::pair<std::set<FragmentSet::data_fragment_t>, std::unique_ptr<boost::filesystem::ofstream> > fragments_ofstream_pair_t;
    typedef std::map<boost::filesystem::path, fragments_ofstream_pair_t> filename_to_writeinfo_map_t;
    boost::filesystem::path saveDirectory;

    MediaApp(const boost::filesystem::path& saveDirectory);
    virtual ~MediaApp();

    // Gui related functions
    void NewFrame();
    void Render();
    void Close();
    void glfw_error_callback(int error, const char* description);
    
    void DisplayImage(); // loads image into imgui buffer
    void UpdateImage(); // if a new image has been delivered, load it
    bool LoadTextureFromFile(const char* filename); // loads image from storage (ie disk) into RAM
    bool LoadTextureFromMemory(char * img_location, int size); //loads image from another location (like kernel) into our memory space
    void DataToOpenGLTexture(void * image_data, int image_width, int image_height); // converts raw memory into open GL viewable texture
    void ExitButton();

    bool should_close = false;
    volatile bool should_update_image = false;
    unsigned int file_number; // if we are saving frames to a local file, this is the  most recent file written to storage
    boost::filesystem::path nextFileFullPathFileName;

protected:
    virtual bool ProcessPayload(const uint8_t * data, const uint64_t size);
public:
    boost::filesystem::path m_saveDirectory;
    filename_to_writeinfo_map_t m_filenameToWriteInfoMap;
};
