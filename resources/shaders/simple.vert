#version 150
#extension GL_ARB_explicit_attrib_location : require


// vertex attributes of VAO
layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec2 in_Tex;
layout (location = 3) in vec3 in_Tangent;
layout (location = 4) in vec3 in_Bitangent;
//layout for uniform block - use instead of our "old" uniforms
layout (std140) uniform CameraBlock
{
    mat4 projection;
    mat4 view;
}blockCam;


//Matrix Uniforms as specified with glUniformMatrix4fv
uniform mat4 ModelMatrix;
uniform mat4 NormalMatrix;
out vec3 pass_Normal;
out vec3 pass_Pos;
out vec3 pass_Vert;
out vec2 pass_UV;
out mat3 pass_TBN;
out vec3 eyepos;

void main(void)
{
    //positions for fragmentshader - same as before just with blockcam
	gl_Position = (blockCam.projection *blockCam.view *ModelMatrix)* vec4(in_Position, 1.0);
	pass_Normal = (NormalMatrix * vec4(in_Normal, 0.0)).xyz;
	pass_Pos= vec3(ModelMatrix * vec4(in_Position, 1.0));
	pass_UV=in_Tex;
	vec3 T   = normalize(mat3(ModelMatrix) * in_Tangent);
    vec3 B   = normalize(mat3(ModelMatrix) * in_Bitangent);
    vec3 N   = normalize(mat3(ModelMatrix) * in_Normal);
    pass_TBN = transpose(mat3(T, B, N));
	vec3 eyepos=vec3(ModelMatrix *blockCam.view* vec4(in_Position, 1.0));
}
