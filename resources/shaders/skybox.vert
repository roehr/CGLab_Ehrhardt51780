#version 150
#extension GL_ARB_explicit_attrib_location : require
// vertex attributes of VAO
layout(location = 0) in vec3 in_Position;

out vec3 pass_tex;
uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
void main(void)
{
    //positions for fragmentshader, nothing fency here
	vec4 pos=(ProjectionMatrix  * ViewMatrix * ModelMatrix) * vec4(in_Position, 1.0);
	gl_Position = pos;
	pass_tex=in_Position;

}