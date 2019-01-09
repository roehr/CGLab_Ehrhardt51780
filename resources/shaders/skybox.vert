#version 150
#extension GL_ARB_explicit_attrib_location : require

// vertex attributes of VAO
layout(location = 0) in vec3 in_Position;


out vec3 pass_tex;
//layout for uniform block
layout (std140) uniform CameraBlock
{
    mat4 projection;
    mat4 view;
}blockCam;
uniform mat4 ModelMatrix;

void main(void)
{
    //positions for fragmentshader, nothing fency here
	vec4 pos=(blockCam.projection *blockCam.view *ModelMatrix)* vec4(in_Position, 1.0);
	
	gl_Position = pos;
	pass_tex=in_Position;

}