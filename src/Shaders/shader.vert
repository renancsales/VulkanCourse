#version 450 

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(binding = 0) uniform uboViewProjection {
	mat4 projection;
	mat4 view;
} viewProjectionMtx;

// not in use, left for reference
layout(binding = 1) uniform uboModel {
	mat4 model;
} modelMtx;

layout(push_constant) uniform PushModel {
	mat4 model;
} pushModel;

layout(location = 1) out vec3 out_color;

void main()
{
	gl_Position = viewProjectionMtx.projection * viewProjectionMtx.view * modelMtx.model *vec4(position, 1.0);
	out_color = color;
}