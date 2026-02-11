#version 330 core

in vec4 channelCol; 
in vec2 channelTex;

out vec4 outCol;

uniform sampler2D uTex;
uniform bool useTex;
uniform bool transparent;
uniform float uAmb; 

void main()
{
	if (!useTex) {
		outCol = channelCol;
	}
	else {
		outCol = texture(uTex, channelTex) * channelCol;
		if (!transparent && outCol.a < 1.0) {
			outCol = vec4(outCol.rgb, 1.0); 
		}
	}
    outCol.rgb += uAmb; 
}