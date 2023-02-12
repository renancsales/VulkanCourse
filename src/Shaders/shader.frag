#version 450

layout(location = 0) out vec4 outColor; // output color

layout(location = 0) in vec3 o_color;
layout(location = 1) in vec2 fragTex;

// Different descriptor set 
layout(set = 1, binding = 0) uniform sampler2D textureSampler;

void main()
{
	outColor = texture(textureSampler,fragTex);
}