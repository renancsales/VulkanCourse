#version 450 

vec3 positions[3] = vec3[](
	vec3(0.0, -0.4, 0.0),
	vec3(0.4, 0.4, 0.0),
	vec3(-0.4, 0.4, 0.0)
);

vec3 colors[3] = vec3[](
	vec3(1.0,0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.4, 1.0)
);


layout(location = 0) out vec3 o_color;


void main()
{
	gl_Position = vec4(positions[gl_VertexIndex],0.0);
	o_color = colors[gl_VertexIndex];
}