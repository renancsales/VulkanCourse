#version 450

layout(input_attachment_index = 0, binding = 0) uniform subpassInput inputColor; // color output from subpass 1
layout(input_attachment_index = 1, binding = 1) uniform subpassInput inputDepth; // depth output from subpass 1

layout(location = 0) out vec4 outColor; // output color

void main()
{
	int xHalf = 500;
	if(gl_FragCoord.x > xHalf)
	{
		float lowerBound = 0.98;
		float upperBound = 1.0;
		
		float depth = subpassLoad(inputDepth).r;
		float depthColorScaled = 1.0f - ((depth-lowerBound)/(upperBound-lowerBound));
		
		if(depthColorScaled > 0.0f)
		{
			outColor = vec4(depthColorScaled, depthColorScaled, depthColorScaled, 1.0f);
		}
		else
		{
			outColor = vec4(0.05f, 0.05f, 0.90f, 1.0f);
		}
		
		//outColor = vec4(subpassLoad(inputColor).rgb * depthColorScaled, 1.0f);
	}
	else
	{
		outColor = subpassLoad(inputColor).rgba;
	}
}