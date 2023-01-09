#include "MediaApp.h"

#include "Logger.h"
#include <boost/make_unique.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/filesystem.hpp>

// image loading 
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

static bool CreateDirectoryRecursivelyVerboseIfNotExist(const boost::filesystem::path & path) {
    if (!boost::filesystem::is_directory(path)) {
        LOG_INFO(subprocess) << "directory does not exist.. creating directory recursively..";
        try {
            if (boost::filesystem::create_directories(path)) {
                LOG_INFO(subprocess) << "successfully created directory";
            }
            else {
                LOG_ERROR(subprocess) << "unable to create directory";
                return false;
            }
        }
        catch (const boost::system::system_error & e) {
            LOG_ERROR(subprocess) << e.what() << "..unable to create directory";
            return false;
        }
    }
    return true;
}

MediaApp::MediaApp(const boost::filesystem::path& saveDirectory) :   BpSinkPattern(),  m_saveDirectory(saveDirectory) 
{
    // setup file logging 
    if (m_saveDirectory.empty()) {
        LOG_INFO(subprocess) << "not saving files";
    }
    else {
        LOG_INFO(subprocess) << "saving files to directory: " << m_saveDirectory;
        if (!CreateDirectoryRecursivelyVerboseIfNotExist(m_saveDirectory)) {
            LOG_INFO(subprocess) << "not saving files";
            m_saveDirectory.clear();
        }
    }

    // Setup window
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

bool MediaApp::LoadTextureFromMemory(char* img_location, int size ) {
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
void MediaApp::UpdateImage() {
    if (should_update_image == true) {
        bool image_loaded = LoadTextureFromFile(nextFileFullPathFileName.c_str());
        // IM_ASSERT(image_loaded);
        if (image_loaded == true) 
            should_update_image = false;
    }
}


static bool IsFileFullyReceived(std::set<FragmentSet::data_fragment_t> & fragmentSet, const uint64_t totalFileSize) {
    if (fragmentSet.size() != 1) {
        return false;
    }
    const FragmentSet::data_fragment_t & df = *fragmentSet.begin();
    return ((df.beginIndex == 0) && (df.endIndex == (totalFileSize - 1)));
}

bool MediaApp::ProcessPayload(const uint8_t * data, const uint64_t size) {
    SendFileMetadata sendFileMetadata;
    if (size < sizeof(sendFileMetadata)) {
        return false;
    }
    memcpy(&sendFileMetadata, data, sizeof(sendFileMetadata));
    data += sizeof(sendFileMetadata);
    boost::endian::little_to_native_inplace(sendFileMetadata.totalFileSize);
    boost::endian::little_to_native_inplace(sendFileMetadata.fragmentOffset);
    boost::endian::little_to_native_inplace(sendFileMetadata.fragmentLength);
    //safety checks
    if (sendFileMetadata.fragmentOffset > 8000000000) { //8GB ignore
        LOG_ERROR(subprocess) << "fragmentOffset > 8GB";
        return false;
    }
    if (sendFileMetadata.fragmentLength > 2000000000) { //2GB ignore
        LOG_ERROR(subprocess) << "fragmentLength > 2GB";
        return false;
    }
    const uint64_t fragmentOffsetPlusFragmentLength = sendFileMetadata.fragmentOffset + sendFileMetadata.fragmentLength;
    if (fragmentOffsetPlusFragmentLength > sendFileMetadata.totalFileSize) {
        LOG_ERROR(subprocess) << "fragment exceeds total file size";
        return false;
    }
    const boost::filesystem::path fileName(std::string(data, data + sendFileMetadata.pathLen));
    data += sendFileMetadata.pathLen;
    fragments_ofstream_pair_t & fragmentsOfstreamPair = m_filenameToWriteInfoMap[fileName];
    std::set<FragmentSet::data_fragment_t> & fragmentSet = fragmentsOfstreamPair.first;
    std::unique_ptr<boost::filesystem::ofstream> & ofstreamPtr = fragmentsOfstreamPair.second;
    bool doWriteFragment = false;
    if (sendFileMetadata.totalFileSize == 0) { //0 length file, create an empty file
        if (fragmentSet.empty() && m_saveDirectory.size()) {
            boost::filesystem::path fullPathFileName = boost::filesystem::path(m_saveDirectory) / boost::filesystem::path(fileName);
            if (!CreateDirectoryRecursivelyVerboseIfNotExist(fullPathFileName.parent_path())) {
                return false;
            }
            if (boost::filesystem::is_regular_file(fullPathFileName)) {
                LOG_INFO(subprocess) << "skipping writing zero-length file " << fullPathFileName << " because it already exists";
                return true;
            }
            boost::filesystem::ofstream ofs(fullPathFileName, boost::filesystem::ofstream::out | boost::filesystem::ofstream::binary);
            if (!ofs.good()) {
                LOG_ERROR(subprocess) << "unable to open file " << fullPathFileName << " for writing";
                return false;
            }
            ofs.close();
        }
    }
    else if (sendFileMetadata.fragmentLength == 0) { //0 length fragment.. ignore
        LOG_INFO(subprocess) << "ignoring 0 length fragment";
    }
    else if (fragmentSet.empty()) { //first reception of this file
        boost::filesystem::path fullPathFileName = boost::filesystem::path(m_saveDirectory) / boost::filesystem::path(fileName);
        if (m_saveDirectory.size()) { //if we are actually saving the files
            if (!CreateDirectoryRecursivelyVerboseIfNotExist(fullPathFileName.parent_path())) {
                return false;
            }
            if (boost::filesystem::is_regular_file(fullPathFileName)) {
                if (sendFileMetadata.fragmentOffset == 0) {
                    LOG_INFO(subprocess) << "skipping writing file " << fullPathFileName << " because it already exists";
                }
                else {
                    LOG_INFO(subprocess) << "ignoring fragment for " << fullPathFileName << " because file already exists";
                }
                return true;
            }
            LOG_INFO(subprocess) << "creating new file " << fullPathFileName;
            ofstreamPtr = boost::make_unique<boost::filesystem::ofstream>(fullPathFileName, boost::filesystem::ofstream::out | boost::filesystem::ofstream::binary);
            if (!ofstreamPtr->good()) {
                LOG_ERROR(subprocess) << "unable to open file " << fullPathFileName << " for writing";
                return false;
            }
        }
        else {
            LOG_INFO(subprocess) << "not creating new file " << fullPathFileName;
        }
        doWriteFragment = true;
    }
    else if (IsFileFullyReceived(fragmentSet, sendFileMetadata.totalFileSize)) { //file was already received.. ignore duplicate fragment
        LOG_INFO(subprocess) << "ignoring duplicate fragment";
    }
    else { //subsequent reception of file fragment 
        doWriteFragment = true;
    }

    if (doWriteFragment) {
        FragmentSet::InsertFragment(fragmentSet, FragmentSet::data_fragment_t(sendFileMetadata.fragmentOffset, fragmentOffsetPlusFragmentLength - 1));
        const bool fileIsFullyReceived = IsFileFullyReceived(fragmentSet, sendFileMetadata.totalFileSize);
        if (m_saveDirectory.size()) { //if we are actually saving the files
            ofstreamPtr->seekp(sendFileMetadata.fragmentOffset); //absolute position releative to beginning
            ofstreamPtr->write((char*)data, sendFileMetadata.fragmentLength);
            if (fileIsFullyReceived) {
                ofstreamPtr->close();
                ofstreamPtr.reset();
            }
        }
        if (fileIsFullyReceived) {
            LOG_INFO(subprocess) << "closed " << fileName;

            std::string file_extension;
            file_extension = boost::filesystem::extension(fileName);

            if(file_extension.compare(".png") == 0 || file_extension.compare(".jpg") == 0) { // if we have supported image types to display
                nextFileFullPathFileName = boost::filesystem::path(m_saveDirectory) / boost::filesystem::path(fileName);
                should_update_image = true;
                LOG_INFO(subprocess) << "file name match, file to load into gui";

            }
        }
    }


    return true;
}