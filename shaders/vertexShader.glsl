#version 330 core
layout (location = 0) in vec3 inVertexPosition;
layout (location = 1) in vec3 inVertexNormal;
layout (location = 2) in vec2 inTextureCoordinate;

out vec3 fragmentPosition;
out vec3 fragmentVertexNormal;
out vec2 fragmentTextureCoordinate;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
// depth pass toggle: render worldPos only, other varyings not needed
uniform bool bDepthOnly = false;

void main()
{
   vec4 worldPos = model * vec4(inVertexPosition, 1.0);
   fragmentPosition = vec3(worldPos);

   // Transform normals with inverse-transpose of the model matrix
   mat3 normalMatrix = mat3(transpose(inverse(model)));
   fragmentVertexNormal = normalize(normalMatrix * inVertexNormal);

   gl_Position = projection * view * worldPos;
   fragmentTextureCoordinate = inTextureCoordinate;
}
