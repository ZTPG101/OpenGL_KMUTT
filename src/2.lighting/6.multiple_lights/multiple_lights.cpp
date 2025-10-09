#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>

#include <iostream>
#include <functional>
#include <cstdlib>
#include <ctime>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path);

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

// lighting
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

// fractal tree parameters
struct Branch {
    glm::vec3 position;
    glm::vec3 direction;
    float length;
    float thickness;
    int level;
    std::vector<Branch> children;
    
    // Growth animation properties
    float currentLength;
    float currentThickness;
    float growthSpeed;
    float maxLength;
    float maxThickness;
    bool isGrowing;
    float growthStartTime;
    float growthDelay;
};

struct Firefly {
    glm::vec3 position;
    glm::vec3 color;
    float orbitRadius;
    float orbitSpeed;
    float orbitAngle;
    int branchLevel;
};

// Global variables
std::vector<Firefly> fireflies;
std::vector<Branch> treeBranches;
Branch rootBranch;

// Function declarations
void generateFractalTree(Branch& branch, int maxLevel, float scaleFactor);
void renderBranch(const Branch& branch, Shader& shader, unsigned int VAO);
void updateFireflies(float deltaTime);
void generateFireflies();
void updateBranchGrowth(Branch& branch, float currentTime);
void initializeBranchGrowth(Branch& branch, float delay);

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
    // generate fractal tree
    rootBranch.position = glm::vec3(0.0f, -2.0f, 0.0f);
    rootBranch.direction = glm::vec3(0.0f, 1.0f, 0.0f);
    rootBranch.length = 2.0f;
    rootBranch.thickness = 0.3f;
    rootBranch.level = 0;
    
    // Initialize random seed
    srand(static_cast<unsigned int>(time(nullptr)));
    
    generateFractalTree(rootBranch, 6, 0.75f);
    initializeBranchGrowth(rootBranch, 0.0f);
    generateFireflies();
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

        // input
        // -----
        processInput(window);
        
        // update branch growth
        updateBranchGrowth(rootBranch, currentTime);
        
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

        /*
           Here we set all the uniforms for the 5/6 types of lights we have. We have to set them manually and index 
           the proper PointLight struct in the array to set each uniform variable. This can be done more code-friendly
           by defining light types as classes and set their values in there, or by using a more efficient uniform approach
           by using 'Uniform buffer objects', but that is something we'll discuss in the 'Advanced GLSL' tutorial.
        */
        // directional light
        lightingShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        lightingShader.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
        lightingShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
        lightingShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);
        // firefly lights (use up to 8 fireflies)
        for (int i = 0; i < 8 && i < static_cast<int>(fireflies.size()); i++) {
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
        lightingShader.setMat4("model", model);

        // bind diffuse map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        // bind specular map
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);

        // render fractal tree
        glBindVertexArray(cubeVAO);
        renderBranch(rootBranch, lightingShader, cubeVAO);

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
void processInput(GLFWwindow *window)
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
unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
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

// generate fractal tree recursively
void generateFractalTree(Branch& branch, int maxLevel, float scaleFactor) {
    if (branch.level >= maxLevel) return;
    
    // Create 3-4 child branches for more complex tree
    int numChildren = 3 + (branch.level % 2); // 3 or 4 children
    
    for (int i = 0; i < numChildren; i++) {
        Branch child;
        child.level = branch.level + 1;
        child.length = branch.length * scaleFactor;
        child.thickness = branch.thickness * scaleFactor;
        
        // Calculate new position at the end of current branch
        child.position = branch.position + branch.direction * branch.length;
        
        // Create 3D branching angles - spread in all directions including South
        float angleY = i * 90.0f; // 0, 90, 180, 270 degrees (N, E, S, W)
        float angleX = (i % 3) * 30.0f - 30.0f; // X rotation: -30, 0, 30 degrees for vertical spread
        float angleZ = (i % 2) * 20.0f; // Z rotation: 0, 20 degrees for slight twist
        
        // Create rotation matrices for 3D spreading
        glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), glm::radians(angleX), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 rotZ = glm::rotate(glm::mat4(1.0f), glm::radians(angleZ), glm::vec3(0.0f, 0.0f, 1.0f));
        
        // Apply rotations to direction (combine all rotations) - no randomness
        child.direction = glm::vec3(rotZ * rotX * rotY * glm::vec4(branch.direction, 0.0f));
        
        branch.children.push_back(child);
        generateFractalTree(branch.children.back(), maxLevel, scaleFactor);
    }
}

// render a branch and its children recursively
void renderBranch(const Branch& branch, Shader& shader, unsigned int VAO) {
    // Calculate transformation matrix for this branch
    glm::mat4 model = glm::mat4(1.0f);
    
    // Translate to branch position
    model = glm::translate(model, branch.position);
    
    // Calculate rotation to align with branch direction
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 axis = glm::cross(up, branch.direction);
    float angle = glm::acos(glm::dot(up, branch.direction));
    
    if (glm::length(axis) > 0.001f) {
        model = glm::rotate(model, angle, glm::normalize(axis));
    }
    
    // Scale to branch dimensions (make it cylindrical) - use current animated values
    model = glm::scale(model, glm::vec3(branch.currentThickness, branch.currentLength, branch.currentThickness));
    
    shader.setMat4("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    
    // Render children recursively
    for (const Branch& child : branch.children) {
        renderBranch(child, shader, VAO);
    }
}

// update firefly positions
void updateFireflies(float deltaTime) {
    for (auto& firefly : fireflies) {
        firefly.orbitAngle += firefly.orbitSpeed * deltaTime;
        
        // All fireflies orbit around the root branch
        glm::vec3 rootBranchEnd = rootBranch.position + rootBranch.direction * rootBranch.currentLength;
        
        // Calculate orbit position around the root branch
        float x = cos(firefly.orbitAngle) * firefly.orbitRadius;
        float z = sin(firefly.orbitAngle) * firefly.orbitRadius;
        
        // Add some vertical variation to the orbit
        float y = sin(firefly.orbitAngle * 0.5f) * 0.3f; // Gentle vertical movement
        
        firefly.position = rootBranchEnd + glm::vec3(x, y, z);
    }
}

// generate fireflies around the first branch (root branch)
void generateFireflies() {
    // Create fireflies centered around the root branch - no randomness
    for (int i = 0; i < 12; i++) {
        Firefly firefly;
        firefly.branchLevel = 0; // All fireflies orbit around root branch
        // Structured orbits around the main trunk
        firefly.orbitRadius = 1.5f + (i * 0.1f); // 1.5, 1.6, 1.7, 1.8, 1.9, 2.0, 2.1, 2.2, 2.3, 2.4, 2.5, 2.6
        firefly.orbitSpeed = 0.5f + (i * 0.05f); // 0.5, 0.55, 0.6, 0.65, 0.7, 0.75, 0.8, 0.85, 0.9, 0.95, 1.0, 1.05
        firefly.orbitAngle = (i * 30.0f) * 3.14159f / 180.0f; // 0, 30, 60, 90, 120, 150, 180, 210, 240, 270, 300, 330 degrees
        
        // Structured firefly colors (warm colors)
        float r = 0.8f + (i * 0.01f);
        float g = 0.6f + (i * 0.02f);
        float b = 0.2f + (i * 0.01f);
        firefly.color = glm::vec3(r, g, b);
        
        fireflies.push_back(firefly);
    }
}

// initialize branch growth properties
void initializeBranchGrowth(Branch& branch, float delay) {
    branch.currentLength = 0.0f;
    branch.currentThickness = 0.0f;
    branch.maxLength = branch.length;
    branch.maxThickness = branch.thickness;
    branch.growthSpeed = 0.3f; // Fixed growth speed for consistent timing
    branch.isGrowing = false;
    branch.growthStartTime = delay;
    branch.growthDelay = delay;
    
    // Initialize children with staggered delays - structured timing
    float childDelay = delay + 2.0f + (branch.level * 0.5f); // Structured delay based on level
    for (auto& child : branch.children) {
        initializeBranchGrowth(child, childDelay);
    }
}

// update branch growth animation
void updateBranchGrowth(Branch& branch, float currentTime) {
    // Check if it's time to start growing
    if (!branch.isGrowing && currentTime >= branch.growthStartTime) {
        branch.isGrowing = true;
    }
    
    if (branch.isGrowing) {
        float growthTime = currentTime - branch.growthStartTime;
        float growthDuration = 2.0f / branch.growthSpeed;
        float growthProgress = glm::min(growthTime / growthDuration, 1.0f);
        
        // Use smooth growth curves
        float smoothProgress = growthProgress * growthProgress * (3.0f - 2.0f * growthProgress); // Smooth step
        
        branch.currentLength = branch.maxLength * smoothProgress;
        branch.currentThickness = branch.maxThickness * smoothProgress;
        
        // Reset growth after 25 seconds for continuous animation
        if (growthTime > 25.0f) {
            branch.isGrowing = false;
            branch.growthStartTime = currentTime + 2.0f; // Restart after 2 second pause
        }
        
        // Update firefly positions based on current branch length
        for (auto& firefly : fireflies) {
            if (firefly.branchLevel == branch.level) {
                // Update firefly orbit based on current branch end
                glm::vec3 branchEnd = branch.position + branch.direction * branch.currentLength;
                float x = cos(firefly.orbitAngle) * firefly.orbitRadius;
                float z = sin(firefly.orbitAngle) * firefly.orbitRadius;
                firefly.position = branchEnd + glm::vec3(x, 0.0f, z);
            }
        }
    }
    
    // Recursively update children
    for (auto& child : branch.children) {
        updateBranchGrowth(child, currentTime);
    }
}
