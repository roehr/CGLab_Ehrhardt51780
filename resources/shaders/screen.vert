#version 330 core
layout (location = 0) in vec2 in_Pos;
layout (location = 1) in vec2 in_uv;

out vec2 pass_uv;

void main()
{
//nothing special here, but we can ignore the z component
    pass_uv = in_uv;
    gl_Position = vec4(in_Pos.x, in_Pos.y, 0.0, 1.0); 
}  