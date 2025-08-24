///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

#include "GLFW/glfw3.h"

#include <glm/gtc/matrix_transform.hpp>


// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
	m_loadedTextures = 0;
    m_shadowFBO = 0;
    m_shadowDepthTexture = 0;
    m_shadowMapWidth = 2048;
    m_shadowMapHeight = 2048;
    m_pDepthShaderManager = nullptr;
    m_spotLightSpaceMatrix = glm::mat4(1.0f);
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
    if (m_shadowDepthTexture != 0)
    {
        GLuint tex = m_shadowDepthTexture;
        glDeleteTextures(1, &tex);
        m_shadowDepthTexture = 0;
    }
    if (m_shadowFBO != 0)
    {
        GLuint fbo = m_shadowFBO;
        glDeleteFramebuffers(1, &fbo);
        m_shadowFBO = 0;
    }
    if (m_pDepthShaderManager != nullptr)
    {
        delete m_pDepthShaderManager;
        m_pDepthShaderManager = nullptr;
    }
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
    // ensure default sampler points at unit 0
    if (NULL != m_pShaderManager)
    {
        m_pShaderManager->setSampler2DValue("objectTexture", 0);
    }
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		if (m_textureIDs[i].ID != 0)
		{
			GLuint id = m_textureIDs[i].ID;
			glDeleteTextures(1, &id);
			m_textureIDs[i].ID = 0;
		}
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

        int textureSlot = -1;
        textureSlot = FindTextureSlot(textureTag);
        if (textureSlot < 0)
        {
            textureSlot = 0;
        }
        // activate and bind selected texture before updating sampler
        glActiveTexture(GL_TEXTURE0 + textureSlot);
        int textureID = FindTextureID(textureTag);
        if (textureID > 0)
        {
            glBindTexture(GL_TEXTURE_2D, textureID);
        }
        m_pShaderManager->setSampler2DValue(g_TextureValueName, textureSlot);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	// load textures once and bind to texture units
	// NOTE: ensure the exact filename in the textures folder matches below
	CreateGLTexture("textures/stone.png", "stone");
	CreateGLTexture("textures/grass.png", "grass");
	BindGLTextures();

    // OPTIONAL: enable basic directional lighting and material defaults
	if (m_pShaderManager != NULL)
	{
		// turn on lighting path in shader
		m_pShaderManager->setIntValue("bUseLighting", true);

		// basic ceramic-like material
		m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(1.0f, 1.0f, 1.0f));
		m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.5f, 0.5f, 0.5f));
		m_pShaderManager->setFloatValue("material.shininess", 32.0f);

        // simple white directional light
		m_pShaderManager->setVec3Value("directionalLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
        m_pShaderManager->setVec3Value("directionalLight.ambient", glm::vec3(0.15f, 0.15f, 0.15f));
        m_pShaderManager->setVec3Value("directionalLight.diffuse", glm::vec3(0.3f, 0.3f, 0.3f));
		m_pShaderManager->setVec3Value("directionalLight.specular", glm::vec3(1.0f, 1.0f, 1.0f));
		m_pShaderManager->setIntValue("directionalLight.bActive", true);

        // soft fill point light to avoid fully dark regions
        m_pShaderManager->setVec3Value("pointLights[0].position", glm::vec3(2.0f, 6.0f, 2.0f));
        m_pShaderManager->setVec3Value("pointLights[0].ambient", glm::vec3(0.08f, 0.08f, 0.08f));
        m_pShaderManager->setVec3Value("pointLights[0].diffuse", glm::vec3(0.35f, 0.35f, 0.35f));
        m_pShaderManager->setVec3Value("pointLights[0].specular", glm::vec3(0.35f, 0.35f, 0.35f));
        m_pShaderManager->setIntValue("pointLights[0].bActive", true);

        // disable additional point lights
        m_pShaderManager->setIntValue("pointLights[1].bActive", false);
        m_pShaderManager->setIntValue("pointLights[2].bActive", false);
        m_pShaderManager->setIntValue("pointLights[3].bActive", false);
        m_pShaderManager->setIntValue("pointLights[4].bActive", false);

        // initialize a camera-tied spotlight (position/direction updated per-frame in ViewManager)
        // widen and brighten flashlight slightly
        m_pShaderManager->setFloatValue("spotLight.cutOff", glm::cos(glm::radians(18.0f)));
        m_pShaderManager->setFloatValue("spotLight.outerCutOff", glm::cos(glm::radians(26.0f)));
        m_pShaderManager->setFloatValue("spotLight.constant", 1.0f);
        m_pShaderManager->setFloatValue("spotLight.linear", 0.045f);
        m_pShaderManager->setFloatValue("spotLight.quadratic", 0.008f);
        m_pShaderManager->setVec3Value("spotLight.ambient", glm::vec3(0.02f));
        m_pShaderManager->setVec3Value("spotLight.diffuse", glm::vec3(1.1f));
        m_pShaderManager->setVec3Value("spotLight.specular", glm::vec3(1.1f));
        m_pShaderManager->setIntValue("spotLight.bActive", true);
	}

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadCylinderMesh();

    // initialize shadow map resources (spotlight shadows)
    if (m_shadowFBO == 0)
    {
        glGenFramebuffers(1, &m_shadowFBO);
        glGenTextures(1, &m_shadowDepthTexture);
        glBindTexture(GL_TEXTURE_2D, m_shadowDepthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_shadowMapWidth, m_shadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadowDepthTexture, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // depth-only shader program can reuse the same vertex shader with a minimalist fragment shader
    if (m_pDepthShaderManager == nullptr)
    {
        m_pDepthShaderManager = new ShaderManager();
        // Reuse existing vertex shader; create a tiny fragment shader at runtime for depth pass
        // We'll write the fragment shader to a temp file if needed, but here we embed a path to an included minimal shader
        // Provide minimal depth-only shaders in project shaders folder
        m_pDepthShaderManager->LoadShaders("shaders/vertexShader.glsl", "shaders/shadowDepthFragment.glsl");
    }
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

    // ensure shadow map and light-space matrix are bound before drawing
    if (m_shadowDepthTexture != 0)
    {
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, m_shadowDepthTexture);
        m_pShaderManager->setSampler2DValue("spotShadowMap", 7);
        m_pShaderManager->setMat4Value("spotLightSpaceMatrix", m_spotLightSpaceMatrix);
    }

	/******************************************************************/
	/*** RENDER THE DARK SURFACE PLANE                             ***/
	/******************************************************************/
	// set the XYZ scale for the plane (table surface)
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the plane
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the plane
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

    // ensure plane uses lighting and a balanced material
    m_pShaderManager->setIntValue("bUseLighting", true);
    m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(1.0f));
    m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.4f));
    m_pShaderManager->setFloatValue("material.shininess", 32.0f);

	// apply stone texture to the plane and increase tiling for larger scale detail
	SetShaderTexture("stone");
	SetTextureUVScale(16.0f, 16.0f);

	// draw the plane mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/******************************************************************/
	/*** RENDER A CERAMIC SAUCER/PLATE UNDER THE MUG               ***/
	/******************************************************************/
	// large thin cylinder as a saucer beneath the cup
	scaleXYZ = glm::vec3(2.6f, 0.02f, 2.6f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// place directly on the ground (center at half-height)
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	// glossy white ceramic
	m_pShaderManager->setIntValue("bUseLighting", true);
	// glass-like: lower diffuse, strong specular, high shininess
	m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.6f));
	m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(1.0f));
	m_pShaderManager->setFloatValue("material.shininess", 128.0f);
	// cream-tinted, opaque, reflective
	SetShaderColor(1.00f, 0.97f, 0.88f, 1.00f);
	SetTextureUVScale(1.0f, 1.0f);
	// ensure opaque saucer writes depth
	glDepthMask(GL_TRUE);
	m_basicMeshes->DrawCylinderMesh(true, true, true);

	// add a shallow raised rim (thin ring) to make it look like a plate
	scaleXYZ = glm::vec3(2.6f, 0.03f, 2.6f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.03f, 0.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	// slightly dimmer tint for rim but still glassy
	m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.55f));
	m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(1.0f));
	m_pShaderManager->setFloatValue("material.shininess", 128.0f);
	SetShaderColor(0.98f, 0.95f, 0.86f, 1.00f);
	m_basicMeshes->DrawCylinderMesh(false, true, true);
	// keep depth writes enabled for subsequent draws
	glDepthMask(GL_TRUE);

	/******************************************************************/
	/*** RENDER THE GREEN TAPERED MUG BODY (OUTER)                 ***/
	/******************************************************************/
	// set the XYZ scale for the outer mug body
	scaleXYZ = glm::vec3(1.5f, 2.0f, 1.5f);

	// set the XYZ rotation for the mug body (flip 180° to make wider at top)
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mug body (slightly above plane to avoid z-fighting)
	positionXYZ = glm::vec3(0.0f, 2.10f, 0.0f);

	// set the transformations into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

    // mug body material
    m_pShaderManager->setIntValue("bUseLighting", true);
    m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(1.0f));
    m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.25f));
    m_pShaderManager->setFloatValue("material.shininess", 24.0f);
    // texture the outer mug body with grass and set moderate tiling
	// texture the outer mug body with grass and set moderate tiling
	SetShaderTexture("grass");
	SetTextureUVScale(2.0f, 1.0f);

	// draw the tapered cylinder mesh (outer mug body - with top, no bottom)
	m_basicMeshes->DrawTaperedCylinderMesh(true, false, true);



	/******************************************************************/
	/*** RENDER THE WHITE TAPERED MUG INTERIOR                     ***/
	/******************************************************************/
	// set the XYZ scale for the inner mug (tucked slightly inside outer to fuse rim)
	scaleXYZ = glm::vec3(1.46f, 1.96f, 1.46f);

	// set the XYZ rotation for the inner mug (flip 180° to match outer mug)
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the inner mug so its rim meets the outer top (align heights)
	positionXYZ = glm::vec3(0.0f, 2.11f, 0.0f);

	// set the transformations into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

    // inner surface material
    m_pShaderManager->setIntValue("bUseLighting", true);
    m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(1.0f));
    m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.25f));
    m_pShaderManager->setFloatValue("material.shininess", 24.0f);
	// texture the inner mug cavity with grass and 1:1 tiling
	SetShaderTexture("grass");
	SetTextureUVScale(1.0f, 1.0f);

	// draw the tapered cylinder mesh (inner mug - no top, no bottom)
	m_basicMeshes->DrawTaperedCylinderMesh(false, false, true);

	/******************************************************************/
	/*** RENDER A STRAW INSIDE THE CUP (before water for visibility) ***/
	/******************************************************************/
	// thin cylinder leaning and resting near inner rim
	scaleXYZ = glm::vec3(0.08f, 2.96f, 0.08f);
	XrotationDegrees = -32.25f;
	YrotationDegrees = 150.0f;
	ZrotationDegrees = 0.0f;
	// move inside cup near inner wall; center height so bottom is within liquid
	positionXYZ = glm::vec3(-0.45f, 0.25f, -0.12f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	// off-white plastic straw
	m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(1.0f));
	m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.35f));
	m_pShaderManager->setFloatValue("material.shininess", 32.0f);
	SetShaderColor(0.96f, 0.96f, 0.92f, 1.0f);
	m_basicMeshes->DrawCylinderMesh(true, true, true);

	/******************************************************************/
	/*** RENDER LIQUID SURFACE INSIDE CUP (RIPPLE)                  ***/
	/******************************************************************/
	// set scale for a thin disk slightly smaller than inner mug radius
	scaleXYZ = glm::vec3(1.30f, 0.01f, 1.30f);

	// rotation: slight tilt to catch lighting highlights
	XrotationDegrees = 0.5f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.3f;

	// position: slightly below the inner mug rim height (raised with cup)
	positionXYZ = glm::vec3(0.0f, 1.78f, 0.0f);

	// set the transformations into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

    // re-enable lighting for liquid to catch highlights on ripples
    m_pShaderManager->setIntValue("bUseLighting", true);
    // semi-transparent liquid tint (cool blue)
	SetShaderColor(0.2f, 0.45f, 0.9f, 0.7f);

	// enable ripple effect uniforms and switch to radial ripple behavior
	m_pShaderManager->setIntValue("bIsLiquidSurface", true);
	// freeze time-based drift to avoid lateral flow look; use slower time for subtle oscillation
	m_pShaderManager->setFloatValue("timeSeconds", (float)glfwGetTime() * 0.5f);
	// reinterpret rippleParams as (speed, radial frequency) in shader
	m_pShaderManager->setVec2Value("rippleParams", glm::vec2(3.0f, 22.0f));

    // glossy liquid material settings for strong specular
	m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(1.0f, 0.95f, 0.8f));
	m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(1.0f, 1.0f, 1.0f));
	m_pShaderManager->setFloatValue("material.shininess", 96.0f);

	// ripple amplitude will be driven at runtime (U/I keys) from View updates;
	// keep a reasonable fallback value if not set yet
	m_pShaderManager->setFloatValue("rippleAmplitude", 0.10f);

	// draw only the top cap of a cylinder to represent the liquid surface
	m_basicMeshes->DrawCylinderMesh(true, false, false);

	// restore depth writes after translucent draw
	glDepthMask(GL_TRUE);

    // disable liquid flag for subsequent draws
	m_pShaderManager->setIntValue("bIsLiquidSurface", false);





	/******************************************************************/
	/*** RENDER THE GREEN MUG BOTTOM                               ***/
	/******************************************************************/
	// set the XYZ scale for the mug bottom (flat cylinder)
	scaleXYZ = glm::vec3(1.0f, 0.1f, 1.0f);

	// set the XYZ rotation for the bottom
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the bottom (raised slightly to avoid z-fighting with plane)
	positionXYZ = glm::vec3(0.0f, 0.06f, 0.0f);

	// set the transformations into memory
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

    // bottom: lit as well for continuity
    m_pShaderManager->setIntValue("bUseLighting", true);
	// texture the mug bottom with grass and 1:1 tiling
	SetShaderTexture("grass");
	SetTextureUVScale(1.0f, 1.0f);

    // draw the cylinder mesh (mug bottom)
	m_basicMeshes->DrawCylinderMesh(true, true, false);

	// restore lighting for any subsequent draws
	m_pShaderManager->setIntValue("bUseLighting", true);

    // after geometry draw, bind spotlight shadow texture and light-space matrix for main shader
    // Note: These uniforms are consumed by the fragment shader to compute shadowing for the spotlight
    if (m_shadowDepthTexture != 0)
    {
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, m_shadowDepthTexture);
        m_pShaderManager->setSampler2DValue("spotShadowMap", 7);
        m_pShaderManager->setMat4Value("spotLightSpaceMatrix", m_spotLightSpaceMatrix);
    }
	/****************************************************************/
}

void SceneManager::RenderShadowMap(const glm::vec3& lightPosition, const glm::vec3& lightDirection)
{
    if (m_shadowFBO == 0 || m_pDepthShaderManager == nullptr)
        return;

    // define light-space transform for spotlight (perspective projection)
    float nearPlane = 0.05f;
    float farPlane = 80.0f;
    float fov = 48.0f; // slightly larger than widened spot outer cone to avoid clipping
    glm::mat4 lightProjection = glm::perspective(glm::radians(fov), (float)m_shadowMapWidth / (float)m_shadowMapHeight, nearPlane, farPlane);
    glm::mat4 lightView = glm::lookAt(lightPosition, lightPosition + glm::normalize(lightDirection), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;
    m_spotLightSpaceMatrix = lightSpaceMatrix;

    // save current viewport and switch to shadow map viewport
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);
    glViewport(0, 0, m_shadowMapWidth, m_shadowMapHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // bind depth-only program
    m_pDepthShaderManager->use();
    m_pDepthShaderManager->setMat4Value("view", lightView);
    m_pDepthShaderManager->setMat4Value("projection", lightProjection);

    auto setDepthModel = [&](const glm::vec3& scaleXYZ,
                             float Xdeg, float Ydeg, float Zdeg,
                             const glm::vec3& posXYZ)
    {
        glm::mat4 scale = glm::scale(scaleXYZ);
        glm::mat4 rotX = glm::rotate(glm::radians(Xdeg), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 rotY = glm::rotate(glm::radians(Ydeg), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 rotZ = glm::rotate(glm::radians(Zdeg), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 trans = glm::translate(posXYZ);
        glm::mat4 model = trans * rotZ * rotY * rotX * scale;
        m_pDepthShaderManager->setMat4Value("model", model);
    };

    // render depth for plane
    setDepthModel(glm::vec3(20.0f, 1.0f, 10.0f), 0.0f, 0.0f, 0.0f, glm::vec3(0.0f));
    m_basicMeshes->DrawPlaneMesh();

    // saucer/plate
    setDepthModel(glm::vec3(2.6f, 0.02f, 2.6f), 0.0f, 0.0f, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
    m_basicMeshes->DrawCylinderMesh(true, true, true);
    // saucer rim
    setDepthModel(glm::vec3(2.6f, 0.03f, 2.6f), 0.0f, 0.0f, 0.0f, glm::vec3(0.0f, 0.03f, 0.0f));
    m_basicMeshes->DrawCylinderMesh(false, true, true);

    // outer mug
    setDepthModel(glm::vec3(1.5f, 2.0f, 1.5f), 180.0f, 0.0f, 0.0f, glm::vec3(0.0f, 2.10f, 0.0f));
    m_basicMeshes->DrawTaperedCylinderMesh(true, false, true);

    // inner mug
    setDepthModel(glm::vec3(1.46f, 1.96f, 1.46f), 180.0f, 0.0f, 0.0f, glm::vec3(0.0f, 2.11f, 0.0f));
    m_basicMeshes->DrawTaperedCylinderMesh(false, false, true);

    // liquid surface
    setDepthModel(glm::vec3(1.30f, 0.01f, 1.30f), 0.5f, 0.0f, 0.3f, glm::vec3(0.0f, 1.78f, 0.0f));
    m_basicMeshes->DrawCylinderMesh(true, false, false);

    // mug bottom
    setDepthModel(glm::vec3(1.0f, 0.1f, 1.0f), 0.0f, 0.0f, 0.0f, glm::vec3(0.0f, 0.06f, 0.0f));
    m_basicMeshes->DrawCylinderMesh(true, true, false);

    // straw
    setDepthModel(glm::vec3(0.08f, 2.96f, 0.08f), -24.0f, -18.0f, 0.0f, glm::vec3(-0.45f, 2.60f, -0.12f));
    m_basicMeshes->DrawCylinderMesh(true, true, true);

    // unbind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // restore viewport to previous
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
}
