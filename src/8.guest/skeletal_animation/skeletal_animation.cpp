#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/animator.h>
#include <learnopengl/model_animation.h>

#include <iostream>


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 800;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Flags for desired animation state (mutually exclusive)
bool requestDance = false;
bool requestMoonwalk = false;


enum AnimState {
	IDLE = 1,
	IDLE_DANCE,
	DANCE_IDLE,
	IDLE_MOONWALK,
	MOONWALK_IDLE,
	DANCE,
	MOONWALK
};

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

	// tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
	stbi_set_flip_vertically_on_load(true);

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	// build and compile shaders
	// -------------------------
	Shader ourShader("anim_model.vs", "anim_model.fs");


	// load models
	// -----------
	Model ourModel(FileSystem::getPath("resources/objects/skelly/skelly.dae"));
	Animation idleAnimation(FileSystem::getPath("resources/objects/skelly/Idle.dae"), &ourModel);
	Animation danceAnimation(FileSystem::getPath("resources/objects/skelly/Breakdance_1990.dae"), &ourModel);
	Animation moonwalkAnimation(FileSystem::getPath("resources/objects/skelly/Moonwalk.dae"), &ourModel);
	Animator animator(&idleAnimation);
	enum AnimState charState = IDLE;
	float blendAmount = 0.0f;
	float blendRate = 2.0f; // Adjusted blend rate for faster blending (e.g., blend in 0.5s at 60fps)

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Input processing for camera and window close
		processInput(window);

		// --- Animation Input Handling ---
		// Determine desired animation state based on current key presses.
		// These flags dictate the desired state for the *next* state machine evaluation.
		// Ensure mutual exclusivity for 'continuous' animation requests.
		bool currentFrameRequestDance = false;
		bool currentFrameRequestMoonwalk = false;

		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			currentFrameRequestDance = true;
		}
		else if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			currentFrameRequestMoonwalk = true;
		}

		if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
			animator.PlayAnimation(&idleAnimation, NULL, 0.0f, 0.0f, 0.0f);
			charState = IDLE;
			blendAmount = 0.0f;
			requestDance = false;
			requestMoonwalk = false;
		}
		else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
			animator.PlayAnimation(&danceAnimation, NULL, 0.0f, 0.0f, 0.0f);
			charState = DANCE;
			blendAmount = 0.0f;
			requestDance = true;
			requestMoonwalk = false;
		}
		else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
			animator.PlayAnimation(&moonwalkAnimation, NULL, 0.0f, 0.0f, 0.0f);
			charState = MOONWALK;
			blendAmount = 0.0f;
			requestDance = false;
			requestMoonwalk = true;
		}
		else {
			// If no direct 1, 2, 3 key was pressed, then use the continuous input from LEFT/RIGHT
			requestDance = currentFrameRequestDance;
			requestMoonwalk = currentFrameRequestMoonwalk;
		}

		// Update the animator *before* processing input that changes state, but after setting request flags.
		animator.UpdateAnimation(deltaTime);

		// State machine logic
		switch (charState) {
		case IDLE:
			// Ensure we are playing the idle animation if nothing else is blending
			if (animator.m_CurrentAnimation != &idleAnimation || animator.m_CurrentAnimation2 != NULL) {
				animator.PlayAnimation(&idleAnimation, NULL, animator.m_CurrentTime, 0.0f, 0.0f);
			}

			if (requestDance) {
				blendAmount = 0.0f;
				animator.PlayAnimation(&idleAnimation, &danceAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
				charState = IDLE_DANCE;
			}
			else if (requestMoonwalk) { // This will only be reached if requestDance is false
				blendAmount = 0.0f;
				animator.PlayAnimation(&idleAnimation, &moonwalkAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
				charState = IDLE_MOONWALK;
			}
			break;

		case IDLE_DANCE:
			blendAmount += blendRate * deltaTime;
			if (blendAmount >= 1.0f) {
				blendAmount = 1.0f;
				animator.PlayAnimation(&danceAnimation, NULL, animator.m_CurrentTime2, 0.0f, 0.0f);
				charState = DANCE;
			}
			else {
				animator.PlayAnimation(&idleAnimation, &danceAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			}
			// If we stop requesting dance during blend-in, blend back to idle immediately
			if (!requestDance && animator.m_CurrentAnimation2 == &danceAnimation) {
				blendAmount = 1.0f - blendAmount; // Reverse the blend amount to start blending out
				animator.PlayAnimation(&danceAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				charState = DANCE_IDLE;
			}
			// If we request moonwalk during blend-in to dance, transition to blend out of dance and into moonwalk
			else if (requestMoonwalk) {
				blendAmount = 0.0f;
				animator.PlayAnimation(&idleAnimation, &moonwalkAnimation, animator.m_CurrentTime, 0.0f, blendAmount); // Blend from current idle time
				charState = IDLE_MOONWALK;
			}
			break;

		case DANCE: // Fully dancing
			// Guard: Ensure DANCE animation is explicitly set without blending.
			if (animator.m_CurrentAnimation != &danceAnimation || animator.m_CurrentAnimation2 != NULL) {
				animator.PlayAnimation(&danceAnimation, NULL, animator.m_CurrentTime, 0.0f, 0.0f);
			}

			if (!requestDance) { // Blend out if key is released
				blendAmount = 0.0f;
				animator.PlayAnimation(&danceAnimation, &idleAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
				charState = DANCE_IDLE;
			}
			else if (requestMoonwalk) { // If moonwalk is requested while dancing
				blendAmount = 0.0f;
				animator.PlayAnimation(&danceAnimation, &moonwalkAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
				charState = MOONWALK; // Directly switch to MOONWALK for now, or you could introduce DANCE_MOONWALK state
			}
			break;

		case DANCE_IDLE: // Blending out of dance
			blendAmount += blendRate * deltaTime;
			if (blendAmount >= 1.0f) {
				blendAmount = 1.0f;
				animator.PlayAnimation(&idleAnimation, NULL, animator.m_CurrentTime2, 0.0f, 0.0f);
				charState = IDLE;
			}
			else {
				animator.PlayAnimation(&danceAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			}
			// If dance is requested again during blend-out
			if (requestDance && animator.m_CurrentAnimation2 == &idleAnimation) {
				blendAmount = 1.0f - blendAmount; // Reverse the blend amount to blend back into dance
				animator.PlayAnimation(&idleAnimation, &danceAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				charState = IDLE_DANCE;
			}
			else if (requestMoonwalk) { // If moonwalk is requested during blend-out of dance
				blendAmount = 0.0f;
				animator.PlayAnimation(&idleAnimation, &moonwalkAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
				charState = IDLE_MOONWALK;
			}
			break;

		case IDLE_MOONWALK:
			blendAmount += blendRate * deltaTime;
			if (blendAmount >= 1.0f) {
				blendAmount = 1.0f;
				animator.PlayAnimation(&moonwalkAnimation, NULL, animator.m_CurrentTime2, 0.0f, 0.0f);
				charState = MOONWALK;
			}
			else {
				animator.PlayAnimation(&idleAnimation, &moonwalkAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			}
			// If we stop requesting moonwalk during blend-in, blend back to idle immediately
			if (!requestMoonwalk && animator.m_CurrentAnimation2 == &moonwalkAnimation) {
				blendAmount = 1.0f - blendAmount; // Reverse the blend amount
				animator.PlayAnimation(&moonwalkAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				charState = MOONWALK_IDLE;
			}
			else if (requestDance) { // If dance is requested during blend-in to moonwalk
				blendAmount = 0.0f;
				animator.PlayAnimation(&idleAnimation, &danceAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
				charState = IDLE_DANCE;
			}
			break;

		case MOONWALK: // Fully moonwalking
			if (animator.m_CurrentAnimation != &moonwalkAnimation || animator.m_CurrentAnimation2 != NULL) {
				animator.PlayAnimation(&moonwalkAnimation, NULL, animator.m_CurrentTime, 0.0f, 0.0f);
			}

			if (!requestMoonwalk) { // Blend out if key is released
				blendAmount = 0.0f;
				animator.PlayAnimation(&moonwalkAnimation, &idleAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
				charState = MOONWALK_IDLE;
			}
			else if (requestDance) { // If dance is requested while moonwalking
				blendAmount = 0.0f;
				animator.PlayAnimation(&moonwalkAnimation, &idleAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
				charState = DANCE; // Directly switch to DANCE for now
			}
			break;

		case MOONWALK_IDLE: // Blending out of moonwalk
			blendAmount += blendRate * deltaTime;
			if (blendAmount >= 1.0f) {
				blendAmount = 1.0f;
				animator.PlayAnimation(&idleAnimation, NULL, animator.m_CurrentTime2, 0.0f, 0.0f);
				charState = IDLE;
			}
			else {
				animator.PlayAnimation(&moonwalkAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			}
			// If moonwalk is requested again during blend-out
			if (requestMoonwalk && animator.m_CurrentAnimation2 == &idleAnimation) {
				blendAmount = 1.0f - blendAmount; // Reverse the blend amount
				animator.PlayAnimation(&idleAnimation, &moonwalkAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				charState = IDLE_MOONWALK;
			}
			else if (requestDance) { // If dance is requested during blend-out of moonwalk
				blendAmount = 0.0f;
				animator.PlayAnimation(&idleAnimation, &danceAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
				charState = IDLE_DANCE;
			}
			break;
		}

		// For debugging state changes
		static AnimState lastCharState = IDLE;
		if (charState != lastCharState) {
			switch (charState) {
			case IDLE: printf("STATE: IDLE\n"); break;
			case IDLE_DANCE: printf("STATE: IDLE_DANCE (Blending in)\n"); break;
			case DANCE: printf("STATE: DANCE (Playing full animation)\n"); break;
			case DANCE_IDLE: printf("STATE: DANCE_IDLE (Blending out)\n"); break;
			case IDLE_MOONWALK: printf("STATE: IDLE_MOONWALK (Blending in)\n"); break;
			case MOONWALK: printf("STATE: MOONWALK (Playing full animation)\n"); break;
			case MOONWALK_IDLE: printf("STATE: MOONWALK_IDLE (Blending out)\n"); break;
			}
			lastCharState = charState;
		}

		// render
		// ------
		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ourShader.use();

		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		ourShader.setMat4("projection", projection);
		ourShader.setMat4("view", view);

		auto transforms = animator.GetFinalBoneMatrices();
		for (int i = 0; i < transforms.size(); ++i)
			ourShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);


		// render the loaded model
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, -0.4f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::scale(model, glm::vec3(.5f, .5f, .5f));	// it's a bit too big for our scene, so scale it down
		ourShader.setMat4("model", model);
		ourModel.Draw(ourShader);


		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

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
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}