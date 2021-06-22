#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"      // Image loading Utility functions

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera.h" // Camera class
#include "cylinder.h" // Cylinder objects
#include "tube.h"  // Modified cylinder for tube objects
#include "Sphere.h" // Sphere objects

using namespace std; // Standard namespace

// Shader program Macro
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Danica Hesemann Final Project"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Main GLFW window
    GLFWwindow* gWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);

    // Textures 
    GLuint grassTexture;
    GLuint doorTexture;
    GLuint shedTexture;
    GLuint roofTexture;
    GLuint firepitTexture;
    GLuint blueTexture;
    GLuint chairTexture;
    GLuint redTexture;
    GLuint barkTexture;
    GLuint pineTexture;
    GLuint knobTexture;

    GLint gTexWrapMode = GL_REPEAT;

    // Shader programs       
    GLuint objectShaderId;
    GLuint lightShaderId;

    // Camera
    Camera gCamera(glm::vec3(0.0f, 3.0f, 15.0f));  // Camera position
    // For mouse input
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;
    float cameraSpeed = 2.5f;  // Inital camera movement speed
    bool orthographic = false;  // For switching to orthographic view

    // Timing
    float gDeltaTime = 0.0f; // Time between current frame and last frame
    float gLastFrame = 0.0f;

    // Lighting   
    glm::vec3 firePos(0.0f, 0.5f, 2.5f);
    glm::vec3 moonPos(-3.0f, 12.0f, 9.0f);
}

//User-defined Function prototypes to initialize the program, set the window size, process mouse/keyboard 
//input, render graphics on the screen, redraw graphics on the window when resized, create/destroy textures,
// and create/destroy shaders
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);

// Shaders                    
// Object vertex shader source code
const GLchar* objectVertexShader = GLSL(440,
    layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;

    gl_Position = projection * view * vec4(FragPos, 1.0);
}
);

// Object fragment shader source code
const GLchar* objectFragmentShader = GLSL(440,
    out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform Material material;
uniform Light light;
uniform Light light2;

void main()
{
    // ambient
    vec3 ambient = light.ambient * texture(material.diffuse, TexCoords).rgb;

    // diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;

    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * texture(material.specular, TexCoords).rgb;

    // attenuation
    float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    vec3 result = ambient + diffuse + specular;

    FragColor = vec4(result, 1.0f);
}
);

// Light vertex shader source code
const GLchar* lightVertexShader = GLSL(440,
    layout(location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
);

// Light fragment shader source code
const GLchar* lightFragmentShader = GLSL(440,
    out vec4 FragColor;
struct Light {
    vec3 color;
};
uniform Light light;

void main()
{
    vec3 color = light.color;
    FragColor = vec4(color, 1.0f); // Adjust color
}
);

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}

int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the shader programs        
    if (!UCreateShaderProgram(objectVertexShader, objectFragmentShader, objectShaderId))
        return EXIT_FAILURE;
    if (!UCreateShaderProgram(lightVertexShader, lightFragmentShader, lightShaderId))
        return EXIT_FAILURE;

    // Load textures              
    const char* texFilename0 = "images/grass.jpg";
    if (!UCreateTexture(texFilename0, grassTexture))
    {
        cout << "Failed to load texture " << texFilename0 << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(objectShaderId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(objectShaderId, "uTexture"), 0);
    
    const char* texFilename1 = "images/shed.jpg";
    if (!UCreateTexture(texFilename1, shedTexture))
    {
        cout << "Failed to load texture " << texFilename1 << endl;
        return EXIT_FAILURE;
    }
    glUniform1i(glGetUniformLocation(objectShaderId, "uTexture"), 0);
    
    const char* texFilename2 = "images/door.jpg";
    if (!UCreateTexture(texFilename2, doorTexture))
    {
        cout << "Failed to load texture " << texFilename2 << endl;
        return EXIT_FAILURE;
    }
    glUniform1i(glGetUniformLocation(objectShaderId, "uTexture"), 0);
    
    const char* texFilename3 = "images/roof.jpg";
    if (!UCreateTexture(texFilename3, roofTexture))
    {
        cout << "Failed to load texture " << texFilename3 << endl;
        return EXIT_FAILURE;
    }
    glUniform1i(glGetUniformLocation(objectShaderId, "uTexture"), 0);
    
    const char* texFilename4 = "images/firepit.jpg";
    if (!UCreateTexture(texFilename4, firepitTexture))
    {
        cout << "Failed to load texture " << texFilename4 << endl;
        return EXIT_FAILURE;
    }
    glUniform1i(glGetUniformLocation(objectShaderId, "uTexture"), 0);
    
    const char* texFilename5 = "images/blue.jpg";
    if (!UCreateTexture(texFilename5, blueTexture))
    {
        cout << "Failed to load texture " << texFilename5 << endl;
        return EXIT_FAILURE;
    }
    glUniform1i(glGetUniformLocation(objectShaderId, "uTexture"), 0);
    
    const char* texFilename6 = "images/chair.jpg";
    if (!UCreateTexture(texFilename6, chairTexture))
    {
        cout << "Failed to load texture " << texFilename6 << endl;
        return EXIT_FAILURE;
    }
    glUniform1i(glGetUniformLocation(objectShaderId, "uTexture"), 0);
    
    const char* texFilename7 = "images/red.jpg";
    if (!UCreateTexture(texFilename7, redTexture))
    {
        cout << "Failed to load texture " << texFilename7 << endl;
        return EXIT_FAILURE;
    }
    glUniform1i(glGetUniformLocation(objectShaderId, "uTexture"), 0);
    
    const char* texFilename8 = "images/bark.jpg";
    if (!UCreateTexture(texFilename8, barkTexture))
    {
        cout << "Failed to load texture " << texFilename8 << endl;
        return EXIT_FAILURE;
    }
    glUniform1i(glGetUniformLocation(objectShaderId, "uTexture"), 0);
    
    const char* texFilename9 = "images/pine.jpg";
    if (!UCreateTexture(texFilename9, pineTexture))
    {
        cout << "Failed to load texture " << texFilename9 << endl;
        return EXIT_FAILURE;
    }
    glUniform1i(glGetUniformLocation(objectShaderId, "uTexture"), 0);
    
    const char* texFilename10 = "images/knob.jpg";
    if (!UCreateTexture(texFilename10, knobTexture))
    {
        cout << "Failed to load texture " << texFilename10 << endl;
        return EXIT_FAILURE;
    }
    glUniform1i(glGetUniformLocation(objectShaderId, "uTexture"), 0);
    
    // Sets the background color of the window to black-ish (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Render loop
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    // Release texture     
    UDestroyTexture(grassTexture);
    UDestroyTexture(shedTexture);
    UDestroyTexture(doorTexture);
    UDestroyTexture(roofTexture);
    UDestroyTexture(firepitTexture);
    UDestroyTexture(blueTexture);
    UDestroyTexture(chairTexture);
    UDestroyTexture(redTexture);
    UDestroyTexture(knobTexture);
    UDestroyTexture(barkTexture);
    UDestroyTexture(pineTexture);

    // Release shader program        
    UDestroyShaderProgram(objectShaderId);
    UDestroyShaderProgram(lightShaderId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}

// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    // GLFW: window creation
    // ---------------------
    *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    return true;
}

// Process keyboard input
void UProcessInput(GLFWwindow* window)
{
    // Close the window if escape key is pressed
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Control camera movement with keyboard
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.Position += gCamera.Front * cameraSpeed * gDeltaTime;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.Position -= gCamera.Front * cameraSpeed * gDeltaTime;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.Position -= gCamera.Right * cameraSpeed * gDeltaTime;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.Position += gCamera.Right * cameraSpeed * gDeltaTime;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.Position += gCamera.Up * cameraSpeed * gDeltaTime;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.Position -= gCamera.Up * cameraSpeed * gDeltaTime;

    // Switch to orthographic/perspective
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        orthographic = !orthographic;
}

// Whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Process mouse input
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}

// Function to change camera speed by scrolling
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Adjust camera speed by scrolling
    cameraSpeed -= (float)yoffset;
    if (cameraSpeed < 0.5f)
        cameraSpeed = 0.5f;
    if (cameraSpeed > 15.0f)
        cameraSpeed = 15.0f;
}

// Functioned called to render a frame
void URender()
{
    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Declare VAOs and VBOs     
    unsigned int planeVao, planeVbo;
    unsigned int shedVao, shedVbo;
    unsigned int roofVao, roofVbo;
    unsigned int cylinderVao, cylinderVbo;
    unsigned int pyramidVao, pyramidVbo;

    // Vertex data (vertex positions, normals, texture coordinates)
    // Roof vertices - 54 vertices
    GLfloat roofVerts[] = {
         -1.0f, 0.7f, 1.0f,  -1.0f, 0.0f, 0.0f,  2.0f, 0.0f, // Left front low
         -1.0f, 0.75f, 1.0f,  -1.0f, 0.0f, 0.0f,  2.0f, 2.0f, // Left front high
         -1.0f, 0.7f, -1.0f,  -1.0f, 0.0f, 0.0f,  0.0f, 0.0f, // Left back low

         -1.0f, 0.75f, 1.0f,  -1.0f, 0.0f, 0.0f,  2.0f, 2.0f, // Left front high
         -1.0f, 0.7f, -1.0f,  -1.0f, 0.0f, 0.0f,  0.0f, 0.0f, // Left back low
         -1.0f, 0.75f, -1.0f,  -1.0f, 0.0f, 0.0f,  0.f, 2.0f, // Left back high

         -1.0f, 0.7f, 1.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, // Left front low
         -1.0f, 0.75f, 1.0f,  0.0f, 0.0f, 1.0f,  0.0f, 2.0f, // Left front high
         0.0f, 1.0f, 1.0f,  0.0f, 0.0f, 1.0f,  2.0f, 2.0f, // Mid front high

         -1.0f, 0.7f, 1.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, // Left front low
         0.0f, 0.95f, 1.0f,  0.0f, 0.0f, 1.0f,  2.0f, 0.0f, // Mid front low
         0.0f, 1.0f, 1.0f,  0.0f, 0.0f, 1.0f,  2.0f, 2.0f, // Mid front high

         -1.0f, 0.7f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f, // Left back low
         -1.0f, 0.75f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 2.0f, // Left back high
         0.0f, 1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  2.0f, 2.0, // Mid back high

         -1.0f, 0.7f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f, // Left back low
         0.0f, 0.95f, -1.0f,  0.0f, 0.0f, -1.0f,  2.0f, 0.0f, // Mid back low
         0.0f, 1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  2.0f, 2.0f, // Mid back high

         0.0f, 0.95f, 1.0f,  0.0f, 0.0f, 1.0f,  2.0f, 0.0f, // Mid front low
         0.0f, 1.0f, 1.0f,  0.0f, 0.0f, 1.0f,  2.0f, 2.0f, // Mid front high
         1.0f, 0.7f, 1.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, // Right front low

         0.0f, 1.0f, 1.0f,  0.0f, 0.0f, 1.0f,  2.0f, 0.0f, // Mid front high
         1.0f, 0.7f, 1.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, // Right front low
         1.0f, 0.75f, 1.0f,  0.0f, 0.0f, 1.0f,  0.0f, 2.0f, // Right front high

         0.0f, 0.95f, -1.0f,  0.0f, 0.0f, -1.0f,  2.0f, 0.0f, // Mid back low
         0.0f, 1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  2.0f, 0.0f, // Mid back high
         1.0f, 0.7f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f, // Right back low

         0.0f, 1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  2.0f, 2.0f, // Mid back high
         1.0f, 0.7f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f, // Right back low
         1.0f, 0.75f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 4.0f, // Right back high

         -1.0f, 0.75f, 1.0f,  -0.242536f, 0.97014f, 0.0f,  4.0f, 0.0f, // Left front high
         -1.0f, 0.75f, -1.0f,  -0.242536f, 0.97014f, 0.0f,  0.0f, 0.0f, // Left back high
         0.0f, 1.0f, -1.0f,  -0.242536f, 0.97014f, 0.0f,  0.0f, 4.0f, // Mid back high

         -1.0f, 0.75f, 1.0f,  -0.242536f, 0.97014f, 0.0f,  4.0f, 0.0f, // Left front high
         0.0f, 1.0f, 1.0f,  -0.242536f, 0.97014f, 0.0f,  4.0f, 4.0f, // Mid front high
         0.0f, 1.0f, -1.0f,  -0.242536f, 0.97014f, 0.0f,  0.0f, 4.0f, // Mid back high

         -1.0f, 0.7f, 1.0f,  0.242536f, -0.97014f, 0.0f,  0.0f, 0.0f, // Left front low
         -1.0f, 0.7f, -1.0f,  0.242536f, -0.97014f, 0.0f,  4.0f, 0.0f, // Left back low
         0.0f, 0.95f, -1.0f,  0.242536f, -0.97014f, 0.0f,  4.0f, 4.0f, // Mid back low

         -1.0f, 0.7f, 1.0f,  0.242536f, -0.97014f, 0.0f,  0.0f, 0.0f, // Left front low
         0.0f, 0.95f, 1.0f,  0.242536f, -0.97014f, 0.0f,  0.0f, 4.0f, // Mid front low
         0.0f, 0.95f, -1.0f,  0.242536f, -0.97014f, 0.0f,  4.0f, 4.0f, // Mid back low

         0.0f, 1.0f, 1.0f,  0.242536f, 0.97014f, 0.0f,  0.0f, 4.0f, // Mid front high
         1.0f, 0.75f, 1.0f,  0.242536f, 0.97014f, 0.0f,  0.0f, 0.0f, // Right front high
         1.0f, 0.75f, -1.0f,  0.242536f, 0.97014f, 0.0f,  4.0f, 0.0f, // Right back high

         0.0f, 1.0f, 1.0f,  0.242536f, 0.97014f, 0.0f,  0.0f, 4.0f, // Mid front high
         0.0f, 1.0f, -1.0f,  0.242536f, 0.97014f, 0.0f,  4.0f, 4.0f, // Mid back high
         1.0f, 0.75f, -1.0f,  0.242536f, 0.97014f, 0.0f,  4.0f, 0.0f, // Right back high

         0.0f, 0.95f, 1.0f,  -0.242536f, -0.97014f, 0.0f,  0.0f, 4.0f, // Mid front low
         1.0f, 0.7f, 1.0f,  -0.242536f, -0.97014f, 0.0f,  0.0f, 0.0f, // Right front low
         1.0f, 0.7f, -1.0f,  -0.242536f, -0.97014f, 0.0f,  4.0f, 0.0f, // Right back low

         0.0f, 0.95f, 1.0f,  -0.242536f, -0.97014f, 0.0f,  0.0f, 4.0f, // Mid front low
         0.0f, 0.95f, -1.0f,  -0.242536f, -0.97014f, 0.0f,  4.0f, 4.0f, // Mid back low
         1.0f, 0.7f, -1.0f,  -0.242536f, -0.97014f, 0.0f,  4.0f, 0.0f, // Right back low
    };

    // Shed vertices - 30 vertices
    GLfloat shedVerts[] = {
        // Left
         -0.9f, -1.0f, 0.9f,  -1.0f, 0.0f, 0.0f,  0.5f, 0.0f, // Front left bottom
         -0.9f, -1.0f, -0.9f,  -1.0f, 0.0f, 0.0f,  0.0f, 0.0f, // Back left bottom
         -0.9f, 0.75f, -0.9f,  -1.0f, 0.0f, 0.0f, 0.5f, 1.0f, // Back left top

         -0.9f, -1.0f, 0.9f,  -1.0f, 0.0f, 0.0f,  0.5f, 0.0f, // Front left bottom
         -0.9f, 0.75f, 0.9f,  -1.0f, 0.0f, 0.0f,  0.5f, 1.0f, // Front left top
         -0.9f, 0.75f, -0.9f,  -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // Back left top
         // Front
         -0.9f, -1.0f, 0.9f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, // Front left bottom
         -0.9f, 0.75f, 0.9f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f, // Front left top
         0.9f, 0.75f, 0.9f,  0.0f, 0.0f, 1.0f,  0.5f, 1.0f, // Front right top

         -0.9f, -1.0f, 0.9f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, // Front left bottom
         0.9f, -1.0f, 0.9f,  0.0f, 0.0f, 1.0f,  0.5f, 0.0f, // Front right bottom
         0.9f, 0.75f, 0.9f,  0.0f, 0.0f, 1.0f,  0.5f, 1.0f, // Front right top
         // Front top
         -0.9f, 0.75f, 0.9f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, // Front left top
         0.0f, 0.95f, 0.9f,  0.0f, 0.0f, 1.0f,  0.25f, 0.12f, // Front middle
         0.9f, 0.75f, 0.9f,  0.0f, 0.0f, 1.0f,  0.5f, 0.0f, // Front right top
         // Right
         0.9f, -1.0f, 0.9f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f, // Front right bottom
         0.9f, 0.75f, 0.9f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f, // Front right top
         0.9f, 0.75f, -0.9f,  1.0f, 0.0f, 0.0f,  0.5f, 1.0f, // Back right top

         0.9f, -1.0f, 0.9f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f, // Front right bottom
         0.9f, -1.0f, -0.9f,  1.0f, 0.0f, 0.0f,  0.5f, 0.0f, // Back right bottom
         0.9f, 0.75f, -0.9f,  1.0f, 0.0f, 0.0f,  0.5f, 1.0f, // Back right top
         // Back
         0.9f, -1.0f, -0.9f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f, // Back right bottom
         0.9f, 0.75f, -0.9f,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f, // Back right top
         -0.9f, 0.75f, -0.9f,  0.0f, 0.0f, -1.0f,  0.5f, 1.0f, // Back left top

         0.9f, -1.0f, -0.9f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f, // Back right bottom
         -0.9f, -1.0f, -0.9f,  0.0f, 0.0f, -1.0f,  0.5f, 0.0f, // Back left bottom
         -0.9f, 0.75f, -0.9f,  0.0f, 0.0f, -1.0f,  0.5f, 1.0f, // Back left top
         // Back top
         0.9f, 0.75f, -0.9f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f, // Back right top
         0.0f, 0.95f, -0.9f,  0.0f, 0.0f, -1.0f,  0.25f, 0.12f, // Back middle
         -0.9f, 0.75f, -0.9f,  0.0f, 0.0f, -1.0f,  0.5f, 0.0f, // Back left top
    };

    //Plane vertices (grass, door, chairs) - 6 vertices
    GLfloat planeVerts[] = {
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
        -1.0f, 1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 5.0f,
        1.0f, 1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  5.0f, 5.0f,

        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  5.0f, 5.0f,
        1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  5.0f, 0.0f,
    };

    // Pyramid vertices (trees) - 18 vertices
    GLfloat pyramidVerts[] = {
        //Positions          // Normal vectors   //Texture Coordinates
        // Base 1
         -1.0f, -1.0f,  1.0f,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f, // Front left 
         -1.0f, -1.0f, -1.0f,  0.0f, -1.0f, 0.0f,  0.0f, 1.0f, // Back left 
         1.0f, -1.0f, -1.0f,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f, // Back right
         // Base 2
         -1.0f, -1.0f,  1.0f,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f, // Front left  
         1.0f, -1.0f, -1.0f,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f, // Back right 
         1.0f, -1.0f,  1.0f,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f, // Front right
         // Left
         -1.0f, -1.0f, -1.0f,  0.89443f, -0.44721f, 0.0f,  0.0f, 0.0f, // Back left 
         -1.0f, -1.0f,  1.0f,  0.89443f, -0.44721f, 0.0f,  5.0f, 0.0f, // Front left 
         0.0f,  1.0f,  0.0f,  0.89443f, -0.44721f, 0.0f,  2.5f, 5.0f, // Top center 
         // Back
         1.0f, -1.0f, -1.0f,  0.0f, 0.44721f, -0.89443f,  0.0f, 0.0f, // Back right 
         -1.0f, -1.0f, -1.0f,  0.0f, 0.44721f, -0.89443f,  5.0f, 0.0f, // Back left 
         0.0f,  1.0f,  0.0f,  0.0f, 0.44721f, -0.89443f,  2.5f, 5.0f, // Top center 
         //Right
         1.0f, -1.0f,  1.0f,  0.89443f, 0.44721f, 0.0f,  0.0f, 0.0f, // Front right
         1.0f, -1.0f, -1.0f,  0.89443f, 0.44721f, 0.0f,  5.0f, 0.0f, // Back right 
         0.0f,  1.0f,  0.0f,  0.89443f, 0.44721f, 0.0f,  2.5f, 5.0f, // Top center
         // Front
         -1.0f, -1.0f,  1.0f,  0.0f, 0.44721f, 0.89443f,  0.0f, 0.0f, // Front left 
         1.0f, -1.0f,  1.0f,  0.0f, 0.44721f, 0.89443f,  5.0f, 0.0f, // Front right 
         0.0f,  1.0f,  0.0f,  0.0f, 0.44721f, 0.89443f,  2.5f, 5.0f, // Top center 
    };
    
    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;
    // Strides between vertex coordinates
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

    // Set up object VAOs and VBOs   
    // Plane
    glGenVertexArrays(1, &planeVao); // Generate VAO
    glBindVertexArray(planeVao);
    glGenBuffers(1, &planeVbo); // Generate VBO
    glBindBuffer(GL_ARRAY_BUFFER, planeVbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVerts), planeVerts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU
    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Cylinders
    glGenVertexArrays(1, &cylinderVao);
    glGenBuffers(1, &cylinderVbo);
    glBindVertexArray(cylinderVao);
    glBindBuffer(GL_ARRAY_BUFFER, cylinderVbo);

    // Shed
    glGenVertexArrays(1, &shedVao);
    glBindVertexArray(shedVao);
    glGenBuffers(1, &shedVbo);
    glBindBuffer(GL_ARRAY_BUFFER, shedVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(shedVerts), shedVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Roof
    glGenVertexArrays(1, &roofVao);
    glBindVertexArray(roofVao);
    glGenBuffers(1, &roofVbo);
    glBindBuffer(GL_ARRAY_BUFFER, roofVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(roofVerts), roofVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Pyramid
    glGenVertexArrays(1, &pyramidVao);
    glBindVertexArray(pyramidVao);
    glGenBuffers(1, &pyramidVbo);
    glBindBuffer(GL_ARRAY_BUFFER, pyramidVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidVerts), pyramidVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Place and draw objects     
    // Ground
    // Camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Creates a perspective or orthographic projection depending on input
    glm::mat4 projection;
    if (orthographic)
    {
        float scale = 100;
        projection = glm::ortho(-(GLfloat)WINDOW_WIDTH / scale, (GLfloat)WINDOW_WIDTH / scale, -(GLfloat)WINDOW_HEIGHT / scale, (GLfloat)WINDOW_HEIGHT / scale, 0.1f, 100.0f);
    }
    else
    {
        projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }

    // Creates model transformation (translates, rotates, scales object)
    glm::mat4 model = glm::mat4(1.0f); // Initialize with identity matrix before applying transformations
    model = glm::translate(glm::vec3(0.0f, 0.0f, 2.0f)) * glm::rotate(glm::radians(90.0f), (glm::vec3(1.0f, 0.0f, 0.0f))) * glm::scale(glm::vec3(10.0f, 12.0f, 0.0f));

    // Set the shader to be used
    glUseProgram(objectShaderId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(objectShaderId, "model");
    GLint viewLoc = glGetUniformLocation(objectShaderId, "view");
    GLint projLoc = glGetUniformLocation(objectShaderId, "projection");
    GLint UVScaleLoc = glGetUniformLocation(objectShaderId, "uvScale");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(glm::vec2(3.0f, 3.0f))); // Scale texture

    // Set up shader properties
    glUniform3f(glGetUniformLocation(objectShaderId, "light.position"), firePos.x, firePos.y, firePos.z);
    glUniform3f(glGetUniformLocation(objectShaderId, "viewPos"), gCamera.Position.x, gCamera.Position.y, gCamera.Position.z);
    // light properties for material
    glUniform3f(glGetUniformLocation(objectShaderId, "light.ambient"), 1.0f, 0.6f, 0.2f);
    glUniform3f(glGetUniformLocation(objectShaderId, "light.diffuse"), 1.0f, 0.6f, 0.2f);
    glUniform3f(glGetUniformLocation(objectShaderId, "light.specular"), 1.0f, 0.6f, 0.3f);
    glUniform1f(glGetUniformLocation(objectShaderId, "light.constant"), 1.0f);
    glUniform1f(glGetUniformLocation(objectShaderId, "light.linear"), 0.09f);
    glUniform1f(glGetUniformLocation(objectShaderId, "light.quadratic"), 0.032f);
    glUniform1f(glGetUniformLocation(objectShaderId, "material.shininess"), 24.0f);

    // Activate VBO
    glBindVertexArray(planeVao);
    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, grassTexture);
    // Draw the shape
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Modify properties and draw the rest of the objects
    // Door
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(glm::vec2(1.0f, 1.0f)));
    model = glm::mat4(1.0f);
    model = glm::translate(glm::vec3(0.0f, 1.5f, -0.29f)) * glm::scale(glm::vec3(0.75f, 1.5f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1f(glGetUniformLocation(objectShaderId, "material.shininess"), 28.0f);
    glBindTexture(GL_TEXTURE_2D, doorTexture);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Chairs
    // Blue back
    model = glm::mat4(1.0f);
    model = glm::translate(glm::vec3(3.0f, 1.625f, 2.5f)) * glm::rotate(glm::radians(90.0f), (glm::vec3(0.0f, 1.0f, 0.0f))) * glm::scale(glm::vec3(0.75f, 0.75f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1f(glGetUniformLocation(objectShaderId, "material.shininess"), 15.0f);
    glBindTexture(GL_TEXTURE_2D, blueTexture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    // Blue seat
    model = glm::mat4(1.0f);
    model = glm::translate(glm::vec3(2.5f, 0.875f, 2.5f)) * glm::rotate(glm::radians(90.0f), (glm::vec3(1.0f, 0.0f, 0.0f))) * glm::scale(glm::vec3(0.5f, 0.75f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 6);
    // Red Back
    model = glm::mat4(1.0f);
    model = glm::translate(glm::vec3(-3.0f, 1.625f, 2.5f)) * glm::rotate(glm::radians(90.0f), (glm::vec3(0.0f, 1.0f, 0.0f))) * glm::scale(glm::vec3(0.75f, 0.75f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glBindTexture(GL_TEXTURE_2D, redTexture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    // Red seat
    model = glm::mat4(1.0f);
    model = glm::translate(glm::vec3(-2.5f, 0.875f, 2.5f)) * glm::rotate(glm::radians(90.0f), (glm::vec3(1.0f, 0.0f, 0.0f))) * glm::scale(glm::vec3(0.5f, 0.75f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Chair legs
    model = glm::mat4(1.0f);
    model = glm::translate(glm::vec3(3.0f, 1.625f, 1.75f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(cylinderVao);
    glBindTexture(GL_TEXTURE_2D, blueTexture);
    static_meshes_3D::Cylinder blue(0.03, 10, 1.5, true, true, true);
    blue.render();
    model = glm::mat4(1.0f);
    model = glm::translate(glm::vec3(3.0f, 1.625f, 3.25f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    blue.render();
    glBindTexture(GL_TEXTURE_2D, redTexture);
    model = glm::mat4(1.0f);
    model = glm::translate(glm::vec3(-3.0f, 1.625f, 1.75f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    static_meshes_3D::Cylinder red(0.03, 10, 1.5, true, true, true);
    red.render();
    model = glm::mat4(1.0f);
    model = glm::translate(glm::vec3(-3.0f, 1.625f, 3.25f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    red.render();
    glBindTexture(GL_TEXTURE_2D, chairTexture);
    glUniform1f(glGetUniformLocation(objectShaderId, "material.shininess"), 35.0f);
    model = glm::mat4(1.0f);
    model = glm::translate(glm::vec3(3.0f, 0.5f, 3.25f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    static_meshes_3D::Cylinder leg(0.03, 10, 0.75, true, true, true);
    leg.render();
    model = glm::translate(glm::vec3(3.0f, 0.5f, 1.75f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    leg.render();
    model = glm::translate(glm::vec3(2.1f, 0.5f, 3.15f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    leg.render();
    model = glm::translate(glm::vec3(2.1f, 0.5f, 1.85f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    leg.render();
    model = glm::mat4(1.0f);
    model = glm::translate(glm::vec3(-3.0f, 0.5f, 3.25f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    leg.render();
    model = glm::translate(glm::vec3(-3.0f, 0.5f, 1.75f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    leg.render();
    model = glm::translate(glm::vec3(-2.1f, 0.5f, 3.15f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    leg.render();
    model = glm::translate(glm::vec3(-2.1f, 0.5f, 1.85f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    leg.render();

    // Fire pit
    glBindTexture(GL_TEXTURE_2D, firepitTexture);
    //Cylinder
    model = glm::mat4(1.0f);
    model = glm::translate(glm::vec3(0.0f, 0.0625f, 2.5f)) * glm::scale(glm::vec3(1.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    static_meshes_3D::Cylinder firepit(1, 10, 0.125, true, true, true);
    firepit.render();
    // Tube
    model = glm::mat4(1.0f);
    model = glm::translate(glm::vec3(0.0f, 0.125f, 2.5f)) * glm::scale(glm::vec3(1.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    static_meshes_3D::Tube T(1, 10, 0.25, true, true, true);
    T.render();

    // Doorknob
    model = glm::mat4(1.0f);
    model = glm::translate(glm::vec3(-0.5f, 1.525f, -0.25f)) * glm::scale(glm::vec3(1.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glBindTexture(GL_TEXTURE_2D, knobTexture);
    Sphere knob(0.1f, 10, 10);
    knob.Draw();

    //Tree trunks
    glBindTexture(GL_TEXTURE_2D, barkTexture);
    model = glm::mat4(1.0f);
    model = glm::translate(glm::vec3(-2.25f, 0.5f, 10.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    static_meshes_3D::Cylinder trunk(0.25, 10, 1.0, true, true, true);
    trunk.render();
    model = glm::mat4(1.0f);
    model = glm::translate(glm::vec3(2.25f, 0.5f, 10.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    trunk.render();

    // Shed
    model = glm::mat4(1.0f);
    model = glm::translate(glm::vec3(0.0f, 3.0f, -3.0f)) * glm::scale(glm::vec3(3.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1f(glGetUniformLocation(objectShaderId, "material.shininess"), 20.0f);
    glBindVertexArray(shedVao);
    glBindTexture(GL_TEXTURE_2D, shedTexture);
    glDrawArrays(GL_TRIANGLES, 0, 30);

    // Tree leaves
    glUniform1f(glGetUniformLocation(objectShaderId, "material.shininess"), 27.0f);
    glBindVertexArray(pyramidVao);
    glBindTexture(GL_TEXTURE_2D, pineTexture);
    model = glm::mat4(1.0f);
    model = glm::translate(glm::vec3(-2.25f, 4.0f, 10.0f)) * glm::rotate(glm::radians(45.0f), (glm::vec3(0.0f, 1.0f, 0.0f))) * glm::scale(glm::vec3(1.0f, 3.0f, 1.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 18);
    model = glm::mat4(1.0f);
    model = glm::translate(glm::vec3(2.25f, 4.0f, 10.0f)) * glm::rotate(glm::radians(45.0f), (glm::vec3(0.0f, 1.0f, 0.0f))) * glm::scale(glm::vec3(1.0f, 3.0f, 1.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 18);
    glUniform3f(glGetUniformLocation(objectShaderId, "light.position"), moonPos.x, moonPos.y, moonPos.z);
    glUniform3f(glGetUniformLocation(objectShaderId, "light.ambient"), 1.0f, 1.0f, 1.0f);
    glUniform3f(glGetUniformLocation(objectShaderId, "light.diffuse"), 1.0f, 1.0f, 1.0f);
    glUniform3f(glGetUniformLocation(objectShaderId, "light.specular"), 1.0f, 1.0f, 1.0f);
    model = glm::mat4(1.0f);
    model = glm::translate(glm::vec3(-2.25f, 4.0f, 10.0f)) * glm::scale(glm::vec3(1.0f, 3.0f, 1.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 18);
    model = glm::mat4(1.0f);
    model = glm::translate(glm::vec3(2.25f, 4.0f, 10.0f)) * glm::scale(glm::vec3(1.0f, 3.0f, 1.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 18);

    // Roof
    model = glm::mat4(1.0f);
    model = glm::translate(glm::vec3(0.0f, 3.0f, -3.0f)) * glm::scale(glm::vec3(3.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1f(glGetUniformLocation(objectShaderId, "material.shininess"), 18.0f);
    glBindVertexArray(roofVao);
    glBindTexture(GL_TEXTURE_2D, roofTexture);
    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, roofSpecular);
    glDrawArrays(GL_TRIANGLES, 0, 54);

    // Switch to light shader
    glUseProgram(lightShaderId);

    // Set up and render first light
    glUniform3f(glGetUniformLocation(lightShaderId, "light.color"), 1.0f, 1.0f, 1.0f);
    model = glm::mat4(1.0f);
    model = glm::translate(moonPos) * glm::scale(glm::vec3(1.0f));

    // Modifies, retrieves and passes transform matrices to the Shader program
    GLint modelLoc2 = glGetUniformLocation(lightShaderId, "model");
    GLint viewLoc2 = glGetUniformLocation(lightShaderId, "view");
    GLint projLoc2 = glGetUniformLocation(lightShaderId, "projection");
    glUniformMatrix4fv(modelLoc2, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc2, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc2, 1, GL_FALSE, glm::value_ptr(projection));
    // Draw the light
    glBindVertexArray(cylinderVao);
    Sphere moon(0.5, 10, 10);
    moon.Draw();

    // Set up and render second light
    glUniform3f(glGetUniformLocation(lightShaderId, "light.color"), 1.0f, 0.5f, 0.0f);
    model = glm::mat4(1.0f);
    model = glm::translate(firePos) * glm::scale(glm::vec3(0.5f));
    glUniformMatrix4fv(modelLoc2, 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(pyramidVao);
    glDrawArrays(GL_TRIANGLES, 0, 18);
    model = glm::mat4(1.0f);
    model = glm::translate(firePos) * glm::rotate(glm::radians(45.0f), (glm::vec3(0.0f, 1.0f, 0.0f))) * glm::scale(glm::vec3(0.5f));
    glUniformMatrix4fv(modelLoc2, 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 18);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}

// Generate and load the texture
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}

void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}


// Implements the UCreateShaders function

bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}
