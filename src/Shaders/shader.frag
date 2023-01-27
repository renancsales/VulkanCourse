#version 450

layout(location = 0) out vec4 outColor;

layout(location = 1) in vec3 o_color;


void main()
{
	outColor = vec4(o_color, 1.0);
}