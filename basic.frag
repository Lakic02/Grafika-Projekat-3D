#version 330 core

in vec4 channelCol;
in vec2 channelTex;

out vec4 outCol;

uniform sampler2D uTex;
uniform bool useTex;
uniform bool transparent;

void main()
{
	if (!useTex) {
		outCol = channelCol;
	}
	else {
		outCol = texture(uTex, channelTex);
		if (!transparent && outCol.a < 1) {
			outCol = vec4(1.0, 1.0, 1.0, 1.0);
		}
	}
}