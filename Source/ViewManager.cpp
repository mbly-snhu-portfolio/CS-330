///////////////////////////////////////////////////////////////////////////////
// viewmanager.cpp
// ============
// manage the viewing of 3D objects within the viewport - camera, projection
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Additional includes for console output and OpenGL
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cmath>    

// declaration of the global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// camera object used for viewing and interacting with
	// the 3D scene
	Camera* g_pCamera = nullptr;

	// these variables are used for mouse movement processing
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// time between current frame and last frame
	float gDeltaTime = 0.0f; 
	float gLastFrame = 0.0f;

	// the following variable is false when orthographic projection
	// is off and true when it is on
	bool bOrthographicProjection = false;
}

/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(
	ShaderManager *pShaderManager)
{
	// initialize the member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;
	m_bPerspectiveProjection = true;  // start in perspective mode
	m_bPKeyPressed = false;
	m_bOKeyPressed = false;
	// initialize smooth movement variables
	m_currentVelocity = glm::vec3(0.0f);
	m_accelerationRate = 25.0f;  // smooth acceleration
	m_decelerationRate = 15.0f;  // smooth deceleration
	// initialize mouse and speed variables
	m_mouseSensitivity = 3.0f;  // increased from default for faster response
	m_movementSpeedMultiplier = 1.0f;

	// initialize liquid ripple control
	m_rippleAmplitude = 0.10f;
	m_rippleStep = 0.01f;
	
	g_pCamera = new Camera();
	// default camera view parameters
	g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
	g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80;
	g_pCamera->MovementSpeed = 20;
}

/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
	// free up allocated memory
	m_pShaderManager = NULL;
	m_pWindow = NULL;
	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
	}
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	// try to create the displayed OpenGL window
	window = glfwCreateWindow(
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		windowTitle,
		NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	// tell GLFW to capture all mouse events
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// this callback is used to receive mouse moving events
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);
	// this callback is used to receive mouse scroll events
	glfwSetScrollCallback(window, &ViewManager::Mouse_Scroll_Callback);

	// enable blending for supporting tranparent rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;

	return(window);
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	// when the first mouse move event is received, this needs to be recorded so that
	// all subsequent mouse moves can correctly calculate the X position offset and Y
	// position offset for proper operation
	if (gFirstMouse)
	{
		gLastX = xMousePos;
		gLastY = yMousePos;
		gFirstMouse = false;
	}

	// calculate the X offset and Y offset values for moving the 3D camera accordingly
	float xOffset = xMousePos - gLastX;
	float yOffset = gLastY - yMousePos; // reversed since y-coordinates go from bottom to top

	// set the current positions into the last position variables
	gLastX = xMousePos;
	gLastY = yMousePos;

	// move the 3D camera according to the calculated offsets with increased sensitivity
	g_pCamera->ProcessMouseMovement(xOffset * 3.0f, yOffset * 3.0f);
}

/***********************************************************
 *  Mouse_Scroll_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse scroll wheel is used within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset)
{
	// get the ViewManager instance (we'll need to store this globally)
	// for now, we'll access the camera directly through the global pointer
	if (g_pCamera != nullptr)
	{
		// adjust movement speed based on scroll direction
		float speedChange = yOffset * 2.0f;  // scroll sensitivity
		g_pCamera->MovementSpeed = glm::clamp(g_pCamera->MovementSpeed + speedChange, 1.0f, 100.0f);
	}
}

/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	// close the window if the escape key has been pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}

	// process smooth camera movement
	ProcessSmoothMovement();

	// process projection mode switching
	ProcessProjectionKeys();

	// process ripple amplitude controls (U/I)
	ProcessRippleControls();
}

/***********************************************************
 *  ProcessProjectionKeys()
 *
 *  This method processes the P and O keys for switching
 *  between perspective and orthographic projection modes
 ***********************************************************/
void ViewManager::ProcessProjectionKeys()
{
	// handle P key for perspective projection
	if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS)
	{
		if (!m_bPKeyPressed)
		{
			m_bPerspectiveProjection = true;
			m_bPKeyPressed = true;
		}
	}
	else
	{
		m_bPKeyPressed = false;
	}

	// handle O key for orthographic projection
	if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS)
	{
		if (!m_bOKeyPressed)
		{
			m_bPerspectiveProjection = false;
			m_bOKeyPressed = true;
		}
	}
	else
	{
		m_bOKeyPressed = false;
	}
}

/***********************************************************
 *  ProcessSmoothMovement()
 *
 *  This method processes smooth camera movement with 
 *  acceleration and deceleration for comfortable navigation
 ***********************************************************/
void ViewManager::ProcessSmoothMovement()
{
	glm::vec3 targetVelocity(0.0f);
	float baseSpeed = g_pCamera->MovementSpeed;

	// determine target velocity based on key presses
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
		targetVelocity += g_pCamera->Front;
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
		targetVelocity -= g_pCamera->Front;
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
		targetVelocity -= g_pCamera->Right;
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
		targetVelocity += g_pCamera->Right;
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS || 
		glfwGetKey(m_pWindow, GLFW_KEY_SPACE) == GLFW_PRESS)
		targetVelocity += g_pCamera->Up;
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS || 
		glfwGetKey(m_pWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		targetVelocity -= g_pCamera->Up;

	// normalize target velocity if moving diagonally
	if (glm::length(targetVelocity) > 0.0f)
		targetVelocity = glm::normalize(targetVelocity) * baseSpeed;

	// smooth interpolation between current and target velocity
	float lerpFactor = (glm::length(targetVelocity) > 0.0f) ? 
		m_accelerationRate * gDeltaTime : m_decelerationRate * gDeltaTime;
	lerpFactor = glm::clamp(lerpFactor, 0.0f, 1.0f);
	
	m_currentVelocity = glm::mix(m_currentVelocity, targetVelocity, lerpFactor);

	// apply movement if velocity is significant
	if (glm::length(m_currentVelocity) > 0.01f)
	{
		g_pCamera->Position += m_currentVelocity * gDeltaTime;
	}
}

/***********************************************************
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// per-frame timing
	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// process any keyboard events that may be waiting in the 
	// event queue
	ProcessKeyboardEvents();

	// get the current view matrix from the camera
	view = g_pCamera->GetViewMatrix();

	// define the current projection matrix based on mode
	if (m_bPerspectiveProjection)
	{
		// perspective projection (3D view)
		projection = glm::perspective(glm::radians(g_pCamera->Zoom), 
			(GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	}
	else
	{
		// orthographic projection (2D view)
		float orthoSize = 10.0f;
		projection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, 100.0f);
	}

	// if the shader manager object is valid
	if (NULL != m_pShaderManager)
	{
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ViewName, view);
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		// set the view position of the camera into the shader for proper rendering
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
		// push current ripple amplitude (U/I controls) every frame
		m_pShaderManager->setFloatValue("rippleAmplitude", m_rippleAmplitude);

        // update camera-tied spotlight each frame so it follows the camera like a flashlight
        m_pShaderManager->setVec3Value("spotLight.position", g_pCamera->Position);
        // ensure Front is normalized for direction
        glm::vec3 camDir = glm::normalize(g_pCamera->Front);
        m_pShaderManager->setVec3Value("spotLight.direction", camDir);
	}

	// render camera information on screen
	RenderCameraInfo();
}

glm::vec3 ViewManager::GetCameraPosition() const
{
    return (g_pCamera != nullptr) ? g_pCamera->Position : glm::vec3(0.0f);
}

glm::vec3 ViewManager::GetCameraFront() const
{
    return (g_pCamera != nullptr) ? g_pCamera->Front : glm::vec3(0.0f, 0.0f, -1.0f);
}

/***********************************************************
 *  RenderCameraInfo()
 *
 *  Minimal stub to keep build intact; HUD was removed.
 ***********************************************************/
void ViewManager::RenderCameraInfo()
{
	// No-op: HUD disabled. Keep function to satisfy references.
}

/***********************************************************
 *  ProcessRippleControls()
 *
 *  Adjust rippleAmplitude using U (decrease) and I (increase)
 ***********************************************************/
void ViewManager::ProcessRippleControls()
{
	if (!m_pWindow) return;

	// Increase ripple amplitude with 'I'
	if (glfwGetKey(m_pWindow, GLFW_KEY_I) == GLFW_PRESS)
	{
		m_rippleAmplitude = glm::clamp(m_rippleAmplitude + m_rippleStep, m_rippleMin, m_rippleMax);
	}

	// Decrease ripple amplitude with 'U'
	if (glfwGetKey(m_pWindow, GLFW_KEY_U) == GLFW_PRESS)
	{
		m_rippleAmplitude = glm::clamp(m_rippleAmplitude - m_rippleStep, m_rippleMin, m_rippleMax);
	}
}