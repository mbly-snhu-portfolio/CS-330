///////////////////////////////////////////////////////////////////////////////
// viewmanager.h
// ============
// manage the viewing of 3D objects within the viewport - camera, projection
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ShaderManager.h"
#include "camera.h"

// GLFW library
#include "GLFW/glfw3.h" 

class ViewManager
{
public:
	// constructor
	ViewManager(
		ShaderManager* pShaderManager);
	// destructor
	~ViewManager();

	// mouse position callback for mouse interaction with the 3D scene
	static void Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos);
	// mouse scroll callback for speed adjustment
	static void Mouse_Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset);

private:
	// pointer to shader manager object
	ShaderManager* m_pShaderManager;
	// active OpenGL display window
	GLFWwindow* m_pWindow;
	// projection mode flag (true = perspective, false = orthographic)
	bool m_bPerspectiveProjection;
	// key press tracking for projection switching
	bool m_bPKeyPressed;
	bool m_bOKeyPressed;
	// smooth movement variables
	glm::vec3 m_currentVelocity;
	float m_accelerationRate;
	float m_decelerationRate;
	// mouse and speed control variables
	float m_mouseSensitivity;
	float m_movementSpeedMultiplier;

	// ripple control state (for liquid surface)
	float m_rippleAmplitude;     // current amplitude
	float m_rippleStep;          // step change per key press
	float m_rippleMin = 0.0f;    // lower clamp
	float m_rippleMax = 0.2f;    // upper clamp

	// process keyboard events for interaction with the 3D scene
	void ProcessKeyboardEvents();
	// process projection mode switching keys
	void ProcessProjectionKeys();
	// process smooth camera movement
	void ProcessSmoothMovement();
	// render on-screen display for camera info
	void RenderCameraInfo();
	// handle ripple amplitude controls (U/I)
	void ProcessRippleControls();

public:
	// create the initial OpenGL display window
	GLFWwindow* CreateDisplayWindow(const char* windowTitle);
	
	// prepare the conversion from 3D object display to 2D scene display
	void PrepareSceneView();

    // camera accessors for systems that need light-aligned data (e.g., shadows)
    glm::vec3 GetCameraPosition() const;
    glm::vec3 GetCameraFront() const;
};