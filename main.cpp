// cmake . && make && __NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia ./icp

// Author: JJ

//
// WARNING:
// In general, you can NOT freely reorder includes!
//

// C++
// include anywhere, in any order
#include <iostream>
#include <chrono>
#include <stack>
#include <random>

// OpenCV (does not depend on GL)
#include <opencv2/opencv.hpp>

// OpenGL Extension Wrangler: allow all multiplatform GL functions
#include <GL/glew.h> 
// WGLEW = Windows GL Extension Wrangler (change for different platform) 
// platform specific functions (in this case Windows)
#include <GL/glxew.h> 

// GLFW toolkit
// Uses GL calls to open GL context, i.e. GLEW __MUST__ be first.
#include <GLFW/glfw3.h>

// OpenGL math (and other additional GL libraries, at the end)
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// #include "cpunoise.hpp"

#include "assets.hpp"
#include <thread>

#include "ShaderProgram.hpp"
#include "Model.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <unordered_set>

#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "irrKlang/include/irrKlang.h"

#include "planet.hpp"

//---------------------------------------------------------------------
// global variables

GLFWwindow * window = NULL; // move App class 

ShaderProgram shader_program;
ShaderProgram tex_shader;

Model player;
GLuint player_texture;

Planet cb_earth;
Planet cb_moon;

// std::vector<vertex> triangle_vertices =
// {
// 	{{0.0f,  0.5f,  0.0f}},
// 	{{0.5f, -0.5f,  0.0f}},
// 	{{-0.5f, -0.5f,  0.0f}}
// };

// glm::quat cam;
glm::vec3 rx(1.0, 0.0, 0.0);
glm::vec3 ry(0.0, 1.0, 0.0);
glm::vec3 rz(0.0, 0.0, 1.0);
glm::vec3 co( 0.0, 0.0, 1.0);

int pp = 0;
glm::mat4 pp_t = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.022f, -0.05f));
glm::mat4 pp_r = glm::mat4(1.0f);// glm::rotate(glm::rotate(glm::mat4(1.0f), (float)M_PI))

// glm::mat3 basis(1.0);

glm::vec3 momentum(0.0);
glm::vec3 thrust(0.0);
glm::vec3 angular_momentum(0.0);
glm::vec3 angular_thrust(0.0);
float ftl_factor(1.0);

cv::CascadeClassifier face_cascade = cv::CascadeClassifier("haarcascade_frontalface_default.xml");
float face_lag = 0.85f;
cv::Point2f face_center = cv::Point2f(0.5f, 0.5f);
cv::Point2f face_true = cv::Point2f(0.5f, 0.5f);
cv::Point2f face = cv::Point2f(0.5f, 0.5f);
std::thread face_thread;
bool shutdown = false;
//---------------------------------------------------------------------

std::unordered_set<int> held_keys;

// std::unordered_set<Model> solar_system;

double sim_time = glfwGetTime();
int sim_speed = 60;
float sim_dt = 1.0 / sim_speed;

double fps_time = glfwGetTime();
int fps_frames = 0;

int vsync = 1;
int fullscreen = 0;
bool moved_mouse = false;
bool moved_face = false;
int mouse_xc, mouse_yc;
int mouse_xpos, mouse_ypos;
int click_xpos, click_ypos;
bool lmb_down = false;
// TODO too basic for 6DoF flight
float yaw, pitch;

bool flashlight = false;

GLint prev_window[4];

irrklang::ISoundEngine* engine = nullptr;

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if(action == GLFW_PRESS)
    {
        held_keys.insert(key);
    }
    
    if(action == GLFW_RELEASE)
    {
        held_keys.erase(key);
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
        glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

    if(key == GLFW_KEY_F3 && action == GLFW_PRESS)
    {
        pp = (pp + 1) % 3;
        if(pp == 0)
        {
            pp_t = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.022f, -0.05f));
            pp_r = glm::mat4(1.0f);
        }
        if(pp == 1)
        {
            pp_t = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.01f, -0.025f));
            pp_r = glm::rotate(glm::mat4(1.0f), (float)M_PI, glm::vec3(0.0f, 1.0f, 0.0f))
                *  glm::rotate(glm::mat4(1.0f),       -0.4f, glm::vec3(1.0f, 0.0f, 0.0f));
        }
        if(pp == 2)
        {
            pp_t = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -0.025f));
            pp_r = glm::rotate(glm::mat4(1.0f), 0.5f * (float)M_PI, glm::vec3(0.0f, 1.0f, 0.0f));
        }
    }

    if (key == GLFW_KEY_F11 && action == GLFW_PRESS)
	{
        vsync = 1 - vsync;
        glfwSwapInterval(vsync);
	}

    if (key == GLFW_KEY_F12 && action == GLFW_PRESS)
	{
        fullscreen = 1 - fullscreen;
        GLFWmonitor *monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        if (fullscreen)
        {
            glfwGetWindowPos(window, &prev_window[0], &prev_window[1]);
            glfwGetWindowSize(window, &prev_window[2], &prev_window[3]);
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            glfwSwapInterval(vsync); // Not sure
        }
        else
        {
            glfwSetWindowMonitor(window, nullptr, prev_window[0], prev_window[1], prev_window[2], prev_window[3], 0);
            glfwSwapInterval(vsync); // Not sure
        }
	}






    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        face_center = face_true;
        face = face_true;
    }

    if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        flashlight = !flashlight;
    }
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        std::cout << "Mouse button: " << button << " @ [" << std::setw(4) << mouse_xpos << ", " << std::setw(4) << mouse_ypos << "]\n";
        click_xpos = mouse_xpos;
        click_ypos = mouse_ypos;
        lmb_down = true;
    }
    else if (action == GLFW_RELEASE)
    {
        lmb_down = false;
    }
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    // TODO screen buffer dependency
    // if (lmb_down)
    // {
    //     co += glm::vec3((xpos - mouse_xpos) * 0.005f, (ypos - mouse_ypos) * 0.005f, 0.0f);
    //     // yaw += (xpos - mouse_xpos) * 0.005f;
    //     // pitch += (ypos - mouse_ypos) * 0.005f;
    //     // pitch = std::clamp(pitch, -0.9f, 0.9f);
    // }
    if(!moved_mouse)
    {
        mouse_xc = xpos;
        mouse_yc = ypos;
        moved_mouse = true;
    }
    int dx = xpos - mouse_xc;
    int dy = ypos - mouse_yc;
    mouse_xc = xpos;
    mouse_yc = ypos;
	mouse_xpos = std::clamp(mouse_xpos + dx, -200, 200);
	mouse_ypos = std::clamp(mouse_ypos + dy, -200, 200);
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    std::cout << "Scroll: [" << std::setw(6) << xoffset << ", " << std::setw(6) << yoffset << "] @ [" << std::setw(4) << mouse_xpos << ", " << std::setw(4) << mouse_ypos << "]\n";
}

void init_assets(void) {
    //
    // Initialize pipeline: compile, link and use shaders
    //

    shader_program = ShaderProgram("res/shaders/noise.vert", "res/shaders/noise.frag");
    tex_shader = ShaderProgram("res/shaders/tex.vert", "res/shaders/tex.frag");
    // model = Model("res/models/sphere_tri_vnt.obj", shader_program);
    // 8, 9, 10; Fixed LoD isn't the best; ends with s for smooth
    auto planetoid = ModelAsset("res/models/ico8s.obj");

    cb_earth = Planet(
        planetoid,
        shader_program,
        1.0f,
        std::vector<glm::vec3>{
            glm::vec3(1.0f, 1.0f, 1.0f),
            glm::vec3(1.0f, 1.0f, 1.0f),
            glm::vec3(0.0f, 0.0f, 0.7f),
            glm::vec3(0.0f, 0.0f, 0.7f),
            glm::vec3(1.0f, 1.0f, 0.5f),
            glm::vec3(0.0f, 0.5f, 0.0f),
            glm::vec3(0.5f, 0.5f, 0.5f),
            glm::vec3(1.0f, 1.0f, 1.0f),
            glm::vec3(1.0f, 1.0f, 1.0f),
        },
        std::vector<float>{
            0.71f, 0.71f, 0.70f, 0.70f, 0.70f, 0.71f, 0.73f, 0.78f, 1.00f, 
        },
        std::vector<float>{
            -0.50f, 0.20f, 0.25f, 0.45f, 0.50f, 0.55f, 0.65f, 0.90f, 2.00f,
        },
        0.5f, glm::vec3(0.2f), 4.0f, glm::vec3(1.5f), 2.0f, GL_TRUE
    );

    cb_moon = Planet(
        planetoid,
        shader_program,
        0.5f,
            std::vector<glm::vec3>{
            glm::vec3(0.5f, 0.5f, 0.5f),
            glm::vec3(0.5f, 0.5f, 0.5f),
            glm::vec3(0.7f, 0.7f, 0.7f),
            glm::vec3(0.4f, 0.4f, 0.4f),
            glm::vec3(0.4f, 0.4f, 0.4f),
            glm::vec3(0.4f, 0.4f, 0.4f),
            glm::vec3(0.4f, 0.4f, 0.4f),
            glm::vec3(0.4f, 0.4f, 0.4f),
            glm::vec3(0.4f, 0.4f, 0.4f),
        },
        std::vector<float>{
            0.70f, 0.70f, 0.72f, 0.70f, 0.40f, 0.70f, 0.70f, 0.70f, 0.70f,
        },
        std::vector<float>{
            -0.50f, 0.45f, 0.65f, 0.70f, 2.00f, 2.00f, 2.00f, 2.00f, 2.00f,
        },
        0.1f, glm::vec3(0.00f), 8.0f, glm::vec3(4.0f), 2.0f, GL_FALSE
    );

    cb_earth.set_default_pos(glm::vec3(0.0f));
    cb_moon.set_orbit(&cb_earth, 3.0f, 360.0f, 0.750f, -60.0f, 0.0f);

    player = Model(std::vector<std::filesystem::path>{"res/models/player-opaque.obj", "res/models/player-transparent.obj"}, tex_shader); // So far works since only one transparent obj
    
    // -- TODO couldn't get this to work
    // // cv::Mat image = cv::imread("res/textures/player.png", cv::IMREAD_UNCHANGED).t();
    // // Untitled.001.png
    // cv::Mat image = cv::imread("res/textures/Untitled.001.png", cv::IMREAD_UNCHANGED);
    // cv::flip(image, image, 0);
    // glPixelStorei(GL_UNPACK_ALIGNMENT, (image.step & 3) ? 1 : 4);
    // glPixelStorei(GL_UNPACK_ROW_LENGTH, image.step/image.elemSize());
    // glCreateTextures(GL_TEXTURE_2D, 1, &player_texture);
    // glTextureStorage2D(player_texture, 1, GL_RGBA8, image.cols, image.rows);
    // glTextureSubImage2D(player_texture, 0, 0, 0, image.cols, image.rows, GL_BGRA, GL_UNSIGNED_BYTE, image.ptr());
    // -- Using this instead
    int width, height, nrChannels;
    unsigned char *data = stbi_load("res/textures/player.png", &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);
    // --
    glTextureParameteri(player_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    engine = irrklang::createIrrKlangDevice();
    if (!engine)
        throw std::runtime_error("Can not initialize sound engine.");
    // Irrklang won't understand quaternion camera afaik -> viewer pos is managed via untransforming sound pos in play_audio
    engine->setListenerPosition(irrklang::vec3df(0,0,0), irrklang::vec3df(0, 0, 1));
}

void play_audio(std::string audio, glm::vec3 at, glm::mat4 v)
{
    audio = "res/audio/" + audio;
    if (!engine->isCurrentlyPlaying(audio.c_str()))
    {
        at = v * glm::vec4(at, 1.0f);
        engine->play3D(audio.c_str(), irrklang::vec3df(at.x, at.y, at.z));
    }
}

void init_imgui()
{
	// see https://github.com/ocornut/imgui/wiki/Getting-Started
	
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();
	std::cout << "ImGUI version: " << ImGui::GetVersion() << "\n";
}

void draw_imgui()
{
    // Begin
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    auto CROSSHAIR_COL = IM_COL32(255, 255, 255, 255);
    auto FACE_COL      = IM_COL32(  0, 255, 255, 255);
    auto MOUSE_COL     = IM_COL32(255, 127,   0, 255);

    GLint m_viewport[4];
    glGetIntegerv( GL_VIEWPORT, m_viewport );
    ImVec2 C = ImVec2((m_viewport[0] + m_viewport[2]) / 2.0f, (m_viewport[1] + m_viewport[3]) / 2.0f);

    auto face_relative = (face - face_center) * (8.0f * 200.0f);
    auto F = C + ImVec2(-face_relative.x, face_relative.y);

    auto M = C + ImVec2(mouse_xpos, mouse_ypos);

    // Input display
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    draw_list->AddLine(C + ImVec2(- 10.0f, -200.0f), C + ImVec2(+ 10.0f, -200.0f), CROSSHAIR_COL, 1.0f); // Max
    draw_list->AddLine(C + ImVec2(- 10.0f, - 60.0f), C + ImVec2(+ 10.0f, - 60.0f), CROSSHAIR_COL, 1.0f); // Deadzone
    draw_list->AddLine(C + ImVec2(- 10.0f,    0.0f), C + ImVec2(+ 10.0f,    0.0f), CROSSHAIR_COL, 1.0f); // Crosshair
    draw_list->AddLine(C + ImVec2(- 10.0f, + 60.0f), C + ImVec2(+ 10.0f, + 60.0f), CROSSHAIR_COL, 1.0f); // Deadzone
    draw_list->AddLine(C + ImVec2(- 10.0f, +200.0f), C + ImVec2(+ 10.0f, +200.0f), CROSSHAIR_COL, 1.0f); // Max

    draw_list->AddLine(C + ImVec2(-200.0f, - 10.0f), C + ImVec2(-200.0f, + 10.0f), CROSSHAIR_COL, 1.0f); // Max
    draw_list->AddLine(C + ImVec2(- 60.0f, - 10.0f), C + ImVec2(- 60.0f, + 10.0f), CROSSHAIR_COL, 1.0f); // Deadzone
    draw_list->AddLine(C + ImVec2(   0.0f, - 10.0f), C + ImVec2(   0.0f, + 10.0f), CROSSHAIR_COL, 1.0f); // Crosshair
    draw_list->AddLine(C + ImVec2(+ 60.0f, - 10.0f), C + ImVec2(+ 60.0f, + 10.0f), CROSSHAIR_COL, 1.0f); // Deadzone
    draw_list->AddLine(C + ImVec2(+200.0f, - 10.0f), C + ImVec2(+200.0f, + 10.0f), CROSSHAIR_COL, 1.0f); // Max

    draw_list->AddLine(F + ImVec2(- 10.0f,    0.0f), F + ImVec2(+ 10.0f,    0.0f), FACE_COL     , 1.0f);
    draw_list->AddLine(F + ImVec2(   0.0f, - 10.0f), F + ImVec2(   0.0f, + 10.0f), FACE_COL     , 1.0f);

    draw_list->AddLine(M + ImVec2(- 10.0f,    0.0f), M + ImVec2(+ 10.0f,    0.0f), MOUSE_COL    , 1.0f);
    draw_list->AddLine(M + ImVec2(   0.0f, - 10.0f), M + ImVec2(   0.0f, + 10.0f), MOUSE_COL    , 1.0f);

    // End
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

cv::Point2f find_face(cv::Mat & frame)
{
    auto start = std::chrono::steady_clock::now();
    cv::Point2f center(0.0f, 0.0f); // for result

	cv::Mat scene_grey;
	cv::cvtColor(frame, scene_grey, cv::COLOR_BGR2GRAY);

	std::vector<cv::Rect> faces;
	face_cascade.detectMultiScale(scene_grey, faces);
        
	if (faces.size() > 0)
	{
          // faces[0].x      -- absolute coordinates
          // faces[0].y      -- absolute coordinates
          // faces[0].width
          // faces[0].height
    
          // compute "center" as normalized coordinates of the face  
          center.x = (faces[0].x + faces[0].width  / 2) * 1.0 / frame.cols;
          center.y = (faces[0].y + faces[0].height / 2) * 1.0 / frame.rows;
	}

	std::chrono::duration<double> time = std::chrono::steady_clock::now() - start;
	// std::cout << "found normalized center: " << center << " in " << time.count() * 1000.0 << "ms" << std::endl;

    return center;      
}

void find_face_runnable(cv::VideoCapture capture)
{
	cv::Mat frame; // for captured frame 
	do {
		capture.read(frame);
		if (frame.empty())
		{
			std::cerr << "Cam disconnected? End of video?" << std::endl;
			return;
		}
		// find face
        auto face0 = find_face(frame); 
        if(face0.x != 0 || face0.y != 0)
        {    
            if(!moved_face)
            {
                face_center = face0;
                face = face0;
                moved_face = true;
            }
            face_true = face0;
            face = face_lag * face + (1.0f - face_lag) * face_true;
        }
	}
	while (!shutdown); //message loop untill ESC
}

void set_light(ShaderProgram sp) {
    sp.setUniform("uniform_AmbientM", glm::vec3(1.0f));
    sp.setUniform("uniform_DiffuseM", glm::vec3(1.0f));
    sp.setUniform("uniform_SpecularM", glm::vec3(0.3f));
    sp.setUniform("uniform_SpecularShininess", 32.0f);

    sp.setUniform("uniform_AmbientI", glm::vec3(0.1f));
    
    sp.setUniform("uniform_SunPos", glm::vec3(30.0f, 0.0f, 30.0f));
    sp.setUniform("uniform_DiffuseSunI", glm::vec3(0.7f));
    sp.setUniform("uniform_SpecularSunI", glm::vec3(0.7f));

    sp.setUniform("uniform_PlayerPos", co - 0.001f * rz + 0.0003f * ry);
    sp.setUniform("uniform_PlayerForward", -rz);
    sp.setUniform("uniform_FlashlightInner", 0.95f);
    sp.setUniform("uniform_FlashlightOuter", 0.91f);
    sp.setUniform("uniform_DiffusePlayerI", glm::vec3(flashlight ? 0.6f : 0.0f));
    sp.setUniform("uniform_SpecularPlayerI", glm::vec3(flashlight ? 0.6f : 0.0f));
}

// !!! cmake . && make && __NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia ./icp
int main(int argc, char** argv)
{
    cv::VideoCapture capture;
	capture.open(-1, cv::CAP_ANY);
	face_thread = std::thread(find_face_runnable, capture);

    std::cout << std::fixed << std::right << std::setprecision(2) << std::setfill(' ');
    // TODO: move to App::init(), add error checking! 
    {
    	// init glfw
    	// https://www.glfw.org/documentation.html
    	glfwInit();
    
    	// open window (GL canvas) with no special properties
        // https://www.glfw.org/docs/latest/quick.html#quick_create_window
 		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
 		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        window = glfwCreateWindow(800, 800, "OpenGL context", NULL, NULL);
        glfwMakeContextCurrent(window);

		glfwSetErrorCallback(error_callback);
		glfwSetKeyCallback(window, key_callback);
    	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    	glfwSetMouseButtonCallback(window, mouse_button_callback);
    	glfwSetCursorPosCallback(window, cursor_position_callback);
    	glfwSetScrollCallback(window, scroll_callback);

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // Enable stuffs
        glEnable(GL_DEPTH_TEST);
        // glEnable(GL_CULL_FACE);
        // glCullFace(GL_FRONT);  

        glEnable(GL_TEXTURE_2D);


        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
        
       	// init glew
    	// http://glew.sourceforge.net/basic.html
    	glewInit();
    	glxewInit();

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		init_assets();
        init_imgui();
    }

	const char* version = (const char*)glGetString(GL_VERSION);
	if (version == nullptr)
		std::cout << "Running unknown OpenGL version!\n";
	else 
		std::cout << "OpenGL version: " << version << '\n';

    // GLfloat r,g,b,a;
    // r=g=b=a=1.0f;

    // Activate shader program. There is only one program, so activation can be out of the loop. 
    // In more realistic scenarios, you will activate different shaders for different 3D objects.
    // glUseProgram(shader_program.id());

    // std::vector<glm::vec3> uniform_color_earth = {
    //     glm::vec3(1.0f, 1.0f, 1.0f),
    //     glm::vec3(1.0f, 1.0f, 1.0f),
    //     glm::vec3(0.0f, 0.0f, 0.7f),
    //     glm::vec3(0.0f, 0.0f, 0.7f),
    //     glm::vec3(1.0f, 1.0f, 0.5f),
    //     glm::vec3(0.0f, 0.5f, 0.0f),
    //     glm::vec3(0.5f, 0.5f, 0.5f),
    //     glm::vec3(1.0f, 1.0f, 1.0f),
    //     glm::vec3(1.0f, 1.0f, 1.0f),
    // };

    // std::vector<float> uniform_height_earth = {
    //      0.71f, 0.71f, 0.70f, 0.70f, 0.70f, 0.71f, 0.73f, 0.78f, 1.00f, 
    // };

    // std::vector<float> uniform_threshold_earth = {
    //     -0.50f, 0.20f, 0.25f, 0.45f, 0.50f, 0.55f, 0.65f, 0.90f, 2.00f,
    // };

    std::vector<glm::vec3> uniform_color_dune = {
        glm::vec3(0.7f, 0.6f, 0.3f),
        glm::vec3(0.7f, 0.6f, 0.3f),
        glm::vec3(1.0f, 0.9f, 0.5f),
        glm::vec3(0.7f, 0.6f, 0.3f),
        glm::vec3(1.0f, 0.9f, 0.5f),
        glm::vec3(0.7f, 0.6f, 0.3f),
        glm::vec3(1.0f, 0.9f, 0.5f),
        glm::vec3(0.7f, 0.6f, 0.3f),
        glm::vec3(1.0f, 0.9f, 0.5f),
    };

    std::vector<float> uniform_height_dune = {
         0.70f, 0.70f, 0.73f, 0.70f, 0.73f, 0.70f, 0.73f, 0.70f, 0.73f,
    };

    std::vector<float> uniform_threshold_dune = {
        -0.50f, 0.20f, 0.35f, 0.40f, 0.55f, 0.60f, 0.75f, 0.80f, 2.00f,
    };

    // std::vector<glm::vec3> uniform_color_moon = {
    //     glm::vec3(0.5f, 0.5f, 0.5f),
    //     glm::vec3(0.5f, 0.5f, 0.5f),
    //     glm::vec3(0.7f, 0.7f, 0.7f),
    //     glm::vec3(0.4f, 0.4f, 0.4f),
    //     glm::vec3(0.4f, 0.4f, 0.4f),
    //     glm::vec3(0.4f, 0.4f, 0.4f),
    //     glm::vec3(0.4f, 0.4f, 0.4f),
    //     glm::vec3(0.4f, 0.4f, 0.4f),
    //     glm::vec3(0.4f, 0.4f, 0.4f),
    // };

    // std::vector<float> uniform_height_moon = {
    //      0.70f, 0.70f, 0.72f, 0.70f, 0.40f, 0.70f, 0.70f, 0.70f, 0.70f,
    // };

    // std::vector<float> uniform_threshold_moon = {
    //     -0.50f, 0.45f, 0.65f, 0.70f, 2.00f, 2.00f, 2.00f, 2.00f, 2.00f,
    // };

    // shader_program.setUniform("uniform_Color", 9, )

    // Application loop
    // move to App::run()
	while (!glfwWindowShouldClose(window))
	{
		// ... do_something();
		// 
		// if (condition)
        //   glfwSetWindowShouldClose(window, GLFW_TRUE);
		// 
		
		// Clear OpenGL canvas, both color buffer and Z-buffer
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        

        // glm::mat4 v = glm::toMat4(cam);

        // TODO quaternion camera and player model matrix are inherently tied which is currently poorly supported
        glm::mat4 pmod = glm::translate(glm::mat4(1.0f), co) * glm::mat4(glm::mat3(
            glm::vec3(rx.x, rx.y, rx.z),
            glm::vec3(ry.x, ry.y, ry.z),
            glm::vec3(rz.x, rz.y, rz.z)
        ))  * glm::scale(glm::mat4(1.0f), glm::vec3(0.01f)) * glm::rotate(glm::mat4(1.0f), 0.5f * (float)M_PI, glm::vec3(0.0f, 1.0f, 0.0f));

        // TODO different perspectives for last mat
        glm::mat4 v = pp_t * pp_r * glm::mat4(glm::inverse(glm::mat3(
            glm::vec3(rx.x, rx.y, rx.z),
            glm::vec3(ry.x, ry.y, ry.z),
            glm::vec3(rz.x, rz.y, rz.z)
        ))) * glm::translate(glm::mat4(1.0f),  -co/*  -0.05f * rz - 0.022f * ry*/);

        

        GLint m_viewport[4];
        glGetIntegerv( GL_VIEWPORT, m_viewport );
        glm::vec2 C = glm::vec2((m_viewport[0] + m_viewport[2]) / 2.0f, (m_viewport[1] + m_viewport[3]) / 2.0f);
        glm::mat4 p = glm::perspective((float) glm::radians(60.0), C.x / C.y, 1e-3f, 1e2f);
        shader_program.activate(); // TODO do away with these
        shader_program.setUniform("uV_m", v);
        shader_program.setUniform("uP_m", p);

        set_light(shader_program);

        cb_earth.draw();
        cb_moon .draw();

        // ============================================
        // "Dune" 
        // ============================================
        // shader_program.setUniform("uniform_Color", uniform_color_dune);
        // shader_program.setUniform("uniform_Height", uniform_height_dune);
        // shader_program.setUniform("uniform_Threshold", uniform_threshold_dune);

        // shader_program.setUniform("uniform_AmplitudeRatio",    0.4f);
        // shader_program.setUniform("uniform_DistortionAmount",  glm::vec3(0.10f, 0.10f, 0.10f));
        // shader_program.setUniform("uniform_DistortionSpatial", 2.0f);
        // shader_program.setUniform("uniform_BaseSpatial", glm::vec3(1.5f, 1.5f, 1.5f));
        // shader_program.setUniform("uniform_SpatialRatio",      2.0f);
        // shader_program.setUniform("uniform_DoMakePoles",      GL_FALSE);
        
        // Player, only obj with transparency, drawn last - no need for painter's at this point
        tex_shader.activate(); // TODO do away with these
        tex_shader.setUniform("uV_m", v);
        tex_shader.setUniform("uP_m", p);
        // glBindTextureUnit(GL_TEXTURE0, player_texture);
        glEnable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0); // TODO get this outta here
        glBindTexture(GL_TEXTURE_2D, player_texture);
        glUniform1i(glGetUniformLocation(tex_shader.id(), "tex0"), 0);
        player.draw(pmod);
        glDisable(GL_TEXTURE_2D);

		double t = glfwGetTime();
		fps_frames += 1;

        draw_imgui();

		// Poll for and process events
		glfwPollEvents();

		// Swap front and back buffers
		glfwSwapBuffers(window);

        // The simulation runs in discrete time steps, dt is used to visually interpolate between steps
        if (t - sim_time >= sim_dt)
        {
            sim_time += sim_dt;

            cb_earth.update(sim_dt);
            cb_moon.update(sim_dt);

            // Angular thrust
            float dx = std::clamp((float)(face.x - face_center.x) * 8.0f, -1.0f, 1.0f);
            float dy = std::clamp((float)(face.y - face_center.y) * 8.0f, -1.0f, 1.0f);
            float dz = std::clamp((float)mouse_xpos * 0.005f, -1.0f, 1.0f);
            // deadzone
            angular_thrust.x = (std::abs(dx) > 0.3f) ? (-dx * 0.008f) : (0.0f);
            angular_thrust.y = (std::abs(dy) > 0.3f) ? ( dy * 0.008f) : (0.0f);
            angular_thrust.z = (std::abs(dz) > 0.3f) ? (-dz * 0.008f) : (0.0f);
            
            angular_momentum += angular_thrust * sim_dt;
            angular_momentum *= 0.99f;

            dx = angular_momentum.x;
            dy = angular_momentum.y;
            dz = angular_momentum.z;

            auto dx_ax = ry;
            auto dy_ax = rx;
            auto dz_ax = rz;
            rx = glm::angleAxis(dx, dx_ax) * glm::angleAxis(dy, dy_ax) * glm::angleAxis(dz, dz_ax) * rx;
            ry = glm::angleAxis(dx, dx_ax) * glm::angleAxis(dy, dy_ax) * glm::angleAxis(dz, dz_ax) * ry;
            rz = glm::angleAxis(dx, dx_ax) * glm::angleAxis(dy, dy_ax) * glm::angleAxis(dz, dz_ax) * rz;

            // Regular thrust
            bool w = held_keys.find(GLFW_KEY_W) != held_keys.end();
            bool s = held_keys.find(GLFW_KEY_S) != held_keys.end();
            bool a = held_keys.find(GLFW_KEY_A) != held_keys.end();
            bool d = held_keys.find(GLFW_KEY_D) != held_keys.end();
            float asc = std::clamp((float)mouse_ypos * 0.005f, -1.0f, 1.0f);
            asc = (std::abs(asc) > 0.3f) ? (-asc * 0.0003f) : (0.0f);
            bool boost = held_keys.find(GLFW_KEY_LEFT_SHIFT) != held_keys.end();

            thrust = rz * (float)((s - w) * (0.0004f + 0.0008f * boost)) + rx * (float)((d - a) * 0.0003f) + ry * asc;
            momentum += thrust * sim_dt;
            momentum *= 0.99f;
            co += momentum;

            // Collisions TODO still a little too much here and the sound is playing from the planetoid
            auto n = cb_moon.handle_player(co, momentum);
            if(glm::length2(n) >= 0.1) // eps, effectively a zero vector
            {
                co -= momentum;
                momentum = 0.5f * (momentum - 2.0f * glm::dot(momentum, n) * n);
                play_audio("impact.wav", cb_moon.get_pos(), v);
            }
            n = cb_earth.handle_player(co, momentum);
            if(glm::length2(n) >= 0.1) // eps, effectively a zero vector
            {
                co -= momentum;
                momentum = 0.5f * (momentum - 2.0f * glm::dot(momentum, n) * n);
                play_audio("impact.wav", cb_earth.get_pos(), v);
            }

            // Gravity TODO needs to be handled by handle_player
            // momentum -= apos * 0.0001f / glm::length2(fromobj) * sim_dt;
        }



		if (t - fps_time >= 1.0)
		{
            auto title = "Elite: Not Very Dangerous | " + std::to_string(fps_frames) + " FPS | VSync " + (vsync ? "ON" : "OFF");
			glfwSetWindowTitle(window, title.c_str()); // !!! std::string must be STORED in a variable or better to use c_str
			fps_frames = 0;
			fps_time = t;

            // std::cout << "cam: " << glm::eulerAngles(cam).x << ", " << glm::eulerAngles(cam).y << ", " << glm::eulerAngles(cam).z << std::endl;
            // std::cout << "camnt: " << camnt.x << ", " << camnt.y << ", " << camnt.z << std::endl;
            std::cout << "cam: " << rx.x << ", " << ry.x << ", " << rz.x << std::endl;
            std::cout << "     " << rx.y << ", " << ry.y << ", " << rz.y << std::endl;
            std::cout << "     " << rx.z << ", " << ry.z << ", " << rz.z << std::endl;
            std::cout << "pos: " << co.x << ", " << co.y << ", " << co.z << std::endl;
		}
	}

	shutdown = true;
	face_thread.join();

    // move to App destructor
	if (window)
		glfwDestroyWindow(window);
	glfwTerminate();

	exit(EXIT_SUCCESS);
}