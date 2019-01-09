#version 150
#extension GL_ARB_explicit_attrib_location : require

// glVertexAttribPointer mapped positions to first
layout(location = 0) in vec3 in_Position;
// glVertexAttribPointer mapped color  to second attribute 
layout(location = 1) in vec3 in_Color;

//layout for uniform block
layout (std140) uniform CameraBlock
{
    mat4 projection;
    mat4 view;
}blockCam;

//Matrix Uniforms uploaded with glUniform*
uniform mat4 ModelMatrix;


out vec3 pass_Color;

void main() {
	gl_Position = (blockCam.projection *blockCam.view *ModelMatrix)* vec4(in_Position, 1.0);
	pass_Color = in_Color;
}