#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp> // For easier rotation of vectors

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>

#include <iostream>
#include <functional>
#include <cstdlib>
#include <ctime>
#include <string>
#include <map>
#include <stack> // For L-system turtle graphics
#include <vector>

void framebuffer_size_callback(GLFWwindow * window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* path);

// settings
const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 1000;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float currentTime = 0.0f;

// L-system parameters
std::string lSystemAxiom = "F"; // Axiom is 'F' for this rule
std::map<char, std::string> lSystemRules;
int lSystemIterations = 8;
float lSystemBranchAngle = 30.0f; 
float lSystemBranchScale = 0.7f;

// L-system animation progress
float lSystemAnimationProgress = 0.0f; // 0.0 to 1.0, controls how much of the tree is 'grown'
float lSystemGrowthSpeed = 0.04f; // Speed at which the tree grows

// Global variable to store the generated L-system string
std::string lSystemString;

// Helper struct for turtle graphics state
struct TurtleState {
    glm::vec3 position;
    glm::vec3 direction; // This is the 'forward' vector
    glm::vec3 up;        // Local 'up' vector
    glm::vec3 right;     // Local 'right' vector
    float length;
    float thickness;
};

struct Firefly {
    glm::vec3 position;
    glm::vec3 color;
    float orbitRadius;
    float orbitSpeed;
    float orbitAngle;
};

// Global variables
std::vector<Firefly> fireflies;

// Function declarations
void updateFireflies(float deltaTime);
void generateFireflies();
std::string generateLSystem(const std::string& axiom, const std::map<char, std::string>& rules, int iterations);
void renderLSystemTree(const std::string& lSystemStr, Shader& shader, unsigned int VAO,
    TurtleState initialTurtleState, // Changed to take an initial TurtleState
    float angle, float scaleFactor, float currentTime, float animationProgress);


int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile our shader zprogram
    // ------------------------------------
    Shader lightingShader("6.multiple_lights.vs", "6.multiple_lights.fs");
    Shader lightCubeShader("6.light_cube.vs", "6.light_cube.fs");

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };

    // Initialize random seed
    srand(static_cast<unsigned int>(time(nullptr)));

    // L-system rules definition for a binary tree
    lSystemRules['F'] = "F[+F][-F]"; // Simple binary branching
    //lSystemRules['X'] = "F[+X][-X]"; // Example for a more complex axiom starting with 'X'

    // Generate the L-system string
    lSystemString = generateLSystem(lSystemAxiom, lSystemRules, lSystemIterations);
    std::cout << "Generated L-system string (truncated): " << lSystemString.substr(0, glm::min((int)lSystemString.length(), 500)) << (lSystemString.length() > 500 ? "..." : "") << std::endl;


    // first, configure the cube's VAO (and VBO)
    unsigned int VBO, cubeVAO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(cubeVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // second, configure the light's VAO (VBO stays the same; the vertices are the same for the light object which is also a 3D cube)
    unsigned int lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // note that we update the lamp's position attribute's stride to reflect the updated buffer data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // load textures (we now use a utility function to keep the code more organized)
    // -----------------------------------------------------------------------------
    // Wood texture
    unsigned int diffuseMap = loadTexture(FileSystem::getPath("resources/textures/Wood047_1K-JPG_Color.jpg").c_str());
    unsigned int specularMap = loadTexture(FileSystem::getPath("resources/textures/container2_specular.png").c_str());

    // shader configuration
    // --------------------
    lightingShader.use();
    lightingShader.setInt("material.diffuse", 0);
    lightingShader.setInt("material.specular", 1);

    // Define initial TurtleState for the tree
    TurtleState initialTurtleState;
    initialTurtleState.position = glm::vec3(0.0f, -2.0f, 0.0f); // Base of the tree
    initialTurtleState.direction = glm::vec3(0.0f, 1.0f, 0.0f); // Start pointing straight up

    // Calculate initial right and up vectors for the turtle's local frame
    glm::vec3 defaultNonParallelAxis = glm::vec3(0.0f, 0.0f, 1.0f); // Z-axis as a reference

    // Calculate 'right' vector. Cross initial direction with a non-parallel axis.
    initialTurtleState.right = glm::normalize(glm::cross(initialTurtleState.direction, defaultNonParallelAxis));
    // If direction is parallel to defaultNonParallelAxis, pick another default
    if (glm::length(initialTurtleState.right) < 0.001f) {
        initialTurtleState.right = glm::normalize(glm::cross(initialTurtleState.direction, glm::vec3(1.0f, 0.0f, 0.0f)));
    }

    // Calculate 'up' vector, orthogonal to 'direction' and 'right'
    initialTurtleState.up = glm::normalize(glm::cross(initialTurtleState.right, initialTurtleState.direction));

    // Ensure all are normalized (might be redundant after cross/normalize but good practice)
    initialTurtleState.direction = glm::normalize(initialTurtleState.direction);
    initialTurtleState.right = glm::normalize(initialTurtleState.right);
    initialTurtleState.up = glm::normalize(initialTurtleState.up);

    initialTurtleState.length = 2.0f;
    initialTurtleState.thickness = 0.3f;


    // Generate fireflies
    generateFireflies();

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        currentTime = currentFrame;

        // Update L-system animation progress
        if (lSystemAnimationProgress < 1.0f) {
            lSystemAnimationProgress += lSystemGrowthSpeed * deltaTime;
            lSystemAnimationProgress = glm::min(lSystemAnimationProgress, 1.0f); // Cap at 1.0
        }
        else {
            lSystemAnimationProgress = 0.0f; // Resets for continuous animation
        }
        // input
        // -----
        processInput(window);

        // update fireflies
        updateFireflies(deltaTime);

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // be sure to activate shader when setting uniforms/drawing objects
        lightingShader.use();
        lightingShader.setVec3("viewPos", camera.Position);
        lightingShader.setFloat("material.shininess", 32.0f);

        // directional light
        lightingShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        lightingShader.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
        lightingShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
        lightingShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);
        // firefly lights (use up to NR_POINT_LIGHTS fireflies - ensure shader has enough)
        for (int i = 0; i < fireflies.size(); i++) {
            std::string prefix = "pointLights[" + std::to_string(i) + "]";
            lightingShader.setVec3((prefix + ".position").c_str(), fireflies[i].position);
            lightingShader.setVec3((prefix + ".ambient").c_str(), fireflies[i].color * 0.05f);
            lightingShader.setVec3((prefix + ".diffuse").c_str(), fireflies[i].color);
            lightingShader.setVec3((prefix + ".specular").c_str(), fireflies[i].color);
            // Softer attenuation so light spreads further
            lightingShader.setFloat((prefix + ".constant").c_str(), 1.0f);
            lightingShader.setFloat((prefix + ".linear").c_str(), 0.07f);
            lightingShader.setFloat((prefix + ".quadratic").c_str(), 0.017f);
        }
        // spotLight
        lightingShader.setVec3("spotLight.position", camera.Position);
        lightingShader.setVec3("spotLight.direction", camera.Front);
        lightingShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
        lightingShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
        lightingShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
        lightingShader.setFloat("spotLight.constant", 1.0f);
        lightingShader.setFloat("spotLight.linear", 0.09f);
        lightingShader.setFloat("spotLight.quadratic", 0.032f);
        lightingShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
        lightingShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        // world transformation
        glm::mat4 model = glm::mat4(1.0f);
        lightingShader.setMat4("model", model); // Set identity model for the scene itself

        // bind diffuse map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        // bind specular map
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);

        // render L-system fractal tree
        glBindVertexArray(cubeVAO);
        // Corrected call to renderLSystemTree
        renderLSystemTree(lSystemString, lightingShader, cubeVAO,
            initialTurtleState, // Pass the pre-initialized TurtleState
            lSystemBranchAngle, lSystemBranchScale,
            currentTime, lSystemAnimationProgress);

        // also draw the lamp object(s)
        lightCubeShader.use();
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);

        // render fireflies as small glowing cubes
        glBindVertexArray(lightCubeVAO);
        for (size_t i = 0; i < fireflies.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, fireflies[i].position);
            model = glm::scale(model, glm::vec3(0.1f)); // Make them very small
            lightCubeShader.setMat4("model", model);
            lightCubeShader.setVec3("lightColor", fireflies[i].color);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteBuffers(1, &VBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// Function to generate the L-system string
std::string generateLSystem(const std::string& axiom, const std::map<char, std::string>& rules, int iterations) {
    std::string current = axiom;
    for (int i = 0; i < iterations; ++i) {
        std::string next = "";
        for (char c : current) {
            if (rules.count(c)) {
                next += rules.at(c);
            }
            else {
                next += c;
            }
        }
        current = next;
    }
    return current;
}

// Function to render the L-system tree using turtle graphics
void renderLSystemTree(const std::string& lSystemStr, Shader& shader, unsigned int VAO,
    TurtleState initialTurtleState,
    float angle, float scaleFactor, float currentTime, // currentTime is already there
    float animationProgress) // <--- NEW: Pass animationProgress here
{
    std::stack<TurtleState> stateStack;
    TurtleState currentState = initialTurtleState;

    glm::vec3 segmentDefaultUp = glm::vec3(0.0f, 1.0f, 0.0f);

    // Calculate how many characters of the L-system string to process
    // This will effectively grow the tree segment by segment.
    size_t charsToProcess = static_cast<size_t>(lSystemStr.length() * animationProgress);
    charsToProcess = glm::min(charsToProcess, lSystemStr.length()); // Ensure it doesn't exceed string length

    // The per-segment growth animation is now mostly handled by the overall progress
    // We can simplify the smoothProgress or remove it if we want distinct segments popping in.
    // For now, let's keep it simple and just draw segments if they are 'processed'.

    for (size_t i = 0; i < charsToProcess; ++i) { // <--- CHANGE: Loop only up to charsToProcess
        char c = lSystemStr[i];
        switch (c) {
        case 'F': // Draw a line segment and move forward
        {
            float currentSegmentLength = currentState.length; // No longer scaled by smoothProgress directly
            float currentSegmentThickness = currentState.thickness;

            if (currentSegmentLength > 0.001f && currentSegmentThickness > 0.001f) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, currentState.position);

                glm::vec3 rotationAxis = glm::cross(segmentDefaultUp, currentState.direction);
                float rotationAngle = glm::acos(glm::dot(segmentDefaultUp, currentState.direction));

                if (glm::length(rotationAxis) > 0.001f) {
                    model = glm::rotate(model, rotationAngle, glm::normalize(rotationAxis));
                }

                model = glm::scale(model, glm::vec3(currentSegmentThickness, currentSegmentLength, currentSegmentThickness));

                shader.setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            currentState.position += currentState.direction * currentSegmentLength;
        }
        break;
        // ... (rest of the cases +, -, &, ^, \, /, [, ]) ...
        case '+':
        {
            currentState.direction = glm::rotate(currentState.direction, glm::radians(angle), currentState.up);
            currentState.right = glm::rotate(currentState.right, glm::radians(angle), currentState.up);
            currentState.direction = glm::normalize(currentState.direction);
            currentState.right = glm::normalize(currentState.right);
        }
        break;
        case '-':
        {
            currentState.direction = glm::rotate(currentState.direction, glm::radians(-angle), currentState.up);
            currentState.right = glm::rotate(currentState.right, glm::radians(-angle), currentState.up);
            currentState.direction = glm::normalize(currentState.direction);
            currentState.right = glm::normalize(currentState.right);
        }
        break;
        case '&':
        {
            currentState.direction = glm::rotate(currentState.direction, glm::radians(angle), currentState.right);
            currentState.up = glm::rotate(currentState.up, glm::radians(angle), currentState.right);
            currentState.direction = glm::normalize(currentState.direction);
            currentState.up = glm::normalize(currentState.up);
        }
        break;
        case '^':
        {
            currentState.direction = glm::rotate(currentState.direction, glm::radians(-angle), currentState.right);
            currentState.up = glm::rotate(currentState.up, glm::radians(-angle), currentState.right);
            currentState.direction = glm::normalize(currentState.direction);
            currentState.up = glm::normalize(currentState.up);
        }
        break;
        case '\\':
        {
            currentState.up = glm::rotate(currentState.up, glm::radians(angle), currentState.direction);
            currentState.right = glm::rotate(currentState.right, glm::radians(angle), currentState.direction);
            currentState.up = glm::normalize(currentState.up);
            currentState.right = glm::normalize(currentState.right);
        }
        break;
        case '/':
        {
            currentState.up = glm::rotate(currentState.up, glm::radians(-angle), currentState.direction);
            currentState.right = glm::rotate(currentState.right, glm::radians(-angle), currentState.direction);
            currentState.up = glm::normalize(currentState.up);
            currentState.right = glm::normalize(currentState.right);
        }
        break;
        case '[':
            stateStack.push(currentState);
            currentState.length *= scaleFactor;
            currentState.thickness *= scaleFactor;
            break;
        case ']':
            currentState = stateStack.top();
            stateStack.pop();
            break;
        }
    }
}

// update firefly positions
void updateFireflies(float deltaTime) {
    // Define a fixed center for firefly orbits, near the base of the L-system tree
    glm::vec3 orbitCenter = glm::vec3(0.0f, -1.0f, 0.0f); // Roughly where the L-system tree starts to branch

    for (auto& firefly : fireflies) {
        firefly.orbitAngle += firefly.orbitSpeed * deltaTime;

        // Calculate orbit position around the fixed center
        float x = cos(firefly.orbitAngle) * firefly.orbitRadius;
        float z = sin(firefly.orbitAngle) * firefly.orbitRadius;

        // Add some vertical variation to the orbit
        float y = sin(firefly.orbitAngle * 0.5f) * 0.3f; // Gentle vertical movement

        firefly.position = orbitCenter + glm::vec3(x, y, z);
    }
}

// generate fireflies around the base of the tree
void generateFireflies() {
    // Create fireflies centered around a fixed point
    for (int i = 0; i < 12; i++) {
        Firefly firefly;
        // Structured orbits
        firefly.orbitRadius = 1.5f + (static_cast<float>(i) * 0.1f); // Radii spread out
        firefly.orbitSpeed = 0.5f + (static_cast<float>(i) * 0.05f); // Speeds vary slightly
        firefly.orbitAngle = (static_cast<float>(i) * 30.0f) * 3.14159f / 180.0f; // Staggered initial angles

        // Structured firefly colors (warm, slightly varying hues)
        float r = 0.8f + (static_cast<float>(rand()) / RAND_MAX * 0.2f);
        float g = 0.6f + (static_cast<float>(rand()) / RAND_MAX * 0.2f);
        float b = 0.2f + (static_cast<float>(rand()) / RAND_MAX * 0.1f);
        firefly.color = glm::vec3(r, g, b);

        fireflies.push_back(firefly);
    }
}