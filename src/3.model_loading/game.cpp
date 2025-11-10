#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>
#include <vector>
#include <limits> // For numeric_limits

// Forward declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

// camera
Camera camera(glm::vec3(0.0f, 5.0f, 10.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Custom camera rotation (relative to the boat's default look direction)
float cameraYawOffset = 0.0f;
float cameraPitchOffset = 0.0f;
const float CAMERA_SENSITIVITY = 0.1f;
const float MAX_PITCH_OFFSET = 89.0f;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// === Game Objects ===
class GameObject {
public:
    Model model;
    glm::vec3 position;
    glm::vec3 scale;
    glm::quat rotation;
    bool hasCollision;

    Model* collisionModel; // Holds the custom collision mesh
    bool useCustomCollisionMesh;

    GameObject(const char* path, glm::vec3 pos = glm::vec3(0.0f), glm::vec3 s = glm::vec3(1.0f), glm::quat rot = glm::identity<glm::quat>(), bool collision = false, const char* collisionPath = nullptr)
        : model(FileSystem::getPath(path)), position(pos), scale(s), rotation(rot), hasCollision(collision), collisionModel(nullptr), useCustomCollisionMesh(false) {

        if (collisionPath && strlen(collisionPath) > 0) { // Check if path is valid
            try {
                collisionModel = new Model(FileSystem::getPath(collisionPath));
                useCustomCollisionMesh = true;
            }
            catch (const std::exception& e) {
                std::cerr << "Error loading collision model " << collisionPath << ": " << e.what() << std::endl;
                collisionModel = nullptr; // Ensure it's null if loading fails
                useCustomCollisionMesh = false;
            }
        }
    }

    ~GameObject() {
        if (collisionModel) {
            delete collisionModel;
            collisionModel = nullptr;
        }
    }

    void Draw(Shader& shader) {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, position);
        modelMatrix = modelMatrix * glm::mat4_cast(rotation);
        modelMatrix = glm::scale(modelMatrix, scale);
        shader.setMat4("model", modelMatrix);
        model.Draw(shader);
    }

    struct BoundingBox {
        glm::vec3 min;
        glm::vec3 max;
    };

    // This method now calculates the AABB for the (transformed) collision model
    BoundingBox GetBoundingBox() const {
        const Model* currentModel = nullptr;
        if (useCustomCollisionMesh && collisionModel) {
            currentModel = collisionModel;
        }
        else {
            currentModel = &model;
        }

        if (currentModel && !currentModel->meshes.empty()) {
            glm::vec3 minWorld = glm::vec3(std::numeric_limits<float>::max());
            glm::vec3 maxWorld = glm::vec3(std::numeric_limits<float>::lowest());

            // The model matrix for the object (position, rotation, scale)
            glm::mat4 modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, position);
            modelMatrix = modelMatrix * glm::mat4_cast(rotation);
            modelMatrix = glm::scale(modelMatrix, scale);

            for (const auto& mesh : currentModel->meshes) {
                for (const auto& vertex : mesh.vertices) {
                    // Transform the vertex by the object's full model matrix
                    glm::vec4 transformedVertex = modelMatrix * glm::vec4(vertex.Position, 1.0f);

                    minWorld.x = glm::min(minWorld.x, transformedVertex.x);
                    minWorld.y = glm::min(minWorld.y, transformedVertex.y);
                    minWorld.z = glm::min(minWorld.z, transformedVertex.z);
                    maxWorld.x = glm::max(maxWorld.x, transformedVertex.x);
                    maxWorld.y = glm::max(maxWorld.y, transformedVertex.y);
                    maxWorld.z = glm::max(maxWorld.z, transformedVertex.z);
                }
            }
            return { minWorld, maxWorld };
        }
        else {
            // Fallback to a simple AABB if no custom collision mesh or it failed to load
            glm::vec3 halfScale = scale * 0.5f;
            return { position - halfScale, position + halfScale };
        }
    }
};

class Player : public GameObject {
public:
    Player(const char* path, glm::vec3 pos = glm::vec3(0.0f), glm::vec3 s = glm::vec3(1.0f), glm::quat rot = glm::identity<glm::quat>(), const char* collisionPath = nullptr)
        : GameObject(path, pos, s, rot, true, collisionPath) {
    }

    void ProcessKeyboard(Camera_Movement direction, float dt) {
        float velocity = 5.0f * dt;
        glm::vec3 forward = rotation * glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 right = rotation * glm::vec3(1.0f, 0.0f, 0.0f);

        if (direction == FORWARD)
            position += forward * velocity;
        if (direction == BACKWARD)
            position -= forward * velocity;

        float turnSpeed = 100.0f * dt;
        if (direction == LEFT)
            rotation = glm::angleAxis(glm::radians(turnSpeed), glm::vec3(0.0f, 1.0f, 0.0f)) * rotation;
        if (direction == RIGHT)
            rotation = glm::angleAxis(glm::radians(-turnSpeed), glm::vec3(0.0f, 1.0f, 0.0f)) * rotation;
    }
};

// Collision detection function (AABB vs AABB)
bool CheckCollision(const GameObject::BoundingBox& a, const GameObject::BoundingBox& b) {
    bool collisionX = a.max.x >= b.min.x && b.max.x >= a.min.x;
    bool collisionY = a.max.y >= b.min.y && b.max.y >= a.min.y;
    bool collisionZ = a.max.z >= b.min.z && b.max.z >= a.min.z;
    return collisionX && collisionY && collisionZ;
}

Player* playerBoat;
std::vector<GameObject> sceneObjects;

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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Boat Game", NULL, NULL);
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

    // build and compile shaders
    // -------------------------
    Shader ourShader("game.vs", "game.fs");

    // === Game Initialization ===
    // Load player boat and its collision mesh
    playerBoat = new Player("resources/objects/boat/boat.obj", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.05f), glm::angleAxis(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), "resources/objects/boat/boat_collision.obj");

    // Load tower/island with its custom collision mesh
    // Make sure 'tower_collision.obj' exists and is correctly placed.
    // Adjust scale for the tower collision model if needed.
    sceneObjects.emplace_back("resources/objects/tower/tower.obj", glm::vec3(2.0f, 0.0f, -3.0f), glm::vec3(0.5f), glm::identity<glm::quat>(), true, "resources/objects/tower/tower_collision.obj");


    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Collision detection for playerBoat
        glm::vec3 originalPlayerPosition = playerBoat->position;
        glm::quat originalPlayerRotation = playerBoat->rotation; // Store original rotation too, if rotation should be affected by collision

        // Process player movement (this updates playerBoat->position and rotation)
        processInput(window);

        // Calculate the player's bounding box after intended movement
        GameObject::BoundingBox playerBB = playerBoat->GetBoundingBox();

        bool collided = false;
        for (auto& obj : sceneObjects) {
            if (obj.hasCollision) {
                GameObject::BoundingBox objBB = obj.GetBoundingBox();
                if (CheckCollision(playerBB, objBB)) {
                    std::cout << "Collision detected with scene object!" << std::endl;
                    collided = true;
                    break; // Exit loop on first collision
                }
            }
        }

        if (collided) {
            playerBoat->position = originalPlayerPosition; // Revert position
            playerBoat->rotation = originalPlayerRotation; // Revert rotation
            // You could also add logic here to stop future movement for a moment
            // or apply a 'bounce' effect, but reverting is the simplest "can't go through"
        }

        // === Camera Logic === (This part remains the same)
        glm::vec3 cameraLocalOffset = glm::vec3(0.0f, 4.0f, 7.0f);
        glm::vec3 cameraLookAtOffset = glm::vec3(0.0f, 1.0f, 0.0f);

        glm::vec3 rotatedCameraOffset = playerBoat->rotation * cameraLocalOffset;
        glm::vec3 rotatedLookAtOffset = playerBoat->rotation * cameraLookAtOffset;

        glm::vec3 baseCameraPos = playerBoat->position + rotatedCameraOffset;
        glm::vec3 baseLookAtTarget = playerBoat->position + rotatedLookAtOffset;

        glm::mat4 rotationMatrix = glm::mat4(1.0f);
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(cameraYawOffset), glm::vec3(0.0f, 1.0f, 0.0f));
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(cameraPitchOffset), glm::vec3(1.0f, 0.0f, 0.0f));

        glm::vec3 cameraToTarget = baseCameraPos - baseLookAtTarget;
        glm::vec3 rotatedCameraToTarget = glm::vec3(rotationMatrix * glm::vec4(cameraToTarget, 0.0f));
        glm::vec3 finalCameraPos = baseLookAtTarget + rotatedCameraToTarget;

        glm::vec3 finalCameraFront = glm::normalize(baseLookAtTarget - finalCameraPos);
        glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

        camera.Position = finalCameraPos;
        camera.Front = finalCameraFront;

        // render
        // ------
        glClearColor(0.05f, 0.05f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        glm::mat4 view = glm::lookAt(camera.Position, camera.Position + camera.Front, cameraUp);
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // Render player boat
        playerBoat->Draw(ourShader);

        // Render scene objects
        for (auto& obj : sceneObjects) {
            obj.Draw(ourShader);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // cleanup
    delete playerBoat;

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        playerBoat->ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        playerBoat->ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        playerBoat->ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        playerBoat->ProcessKeyboard(RIGHT, deltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

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
    float yoffset = ypos - lastY;

    lastX = xpos;
    lastY = ypos;

    cameraYawOffset += -xoffset * CAMERA_SENSITIVITY;
    cameraPitchOffset += yoffset * CAMERA_SENSITIVITY;

    if (cameraPitchOffset > MAX_PITCH_OFFSET)
        cameraPitchOffset = MAX_PITCH_OFFSET;
    if (cameraPitchOffset < -MAX_PITCH_OFFSET)
        cameraPitchOffset = -MAX_PITCH_OFFSET;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}