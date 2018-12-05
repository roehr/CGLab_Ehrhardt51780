#version 330 core
out vec4 out_Color;

in vec3 pass_tex;

uniform samplerCube CubeMap;
void main()
{    //just look up our texture

   out_Color = texture(CubeMap,pass_tex);
   
}