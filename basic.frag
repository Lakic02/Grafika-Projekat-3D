#version 330 core

in vec4 channelCol; // Boja iz C++ (gde je alpha 0.4)
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
		// Množimo boju teksture sa bojom kanala da bismo dobili providnost
		outCol = texture(uTex, channelTex) * channelCol;
		
		// Ako nije ukljucena transparentnost, forsiraj punu boju (tvoja stara logika)
		if (!transparent && outCol.a < 1.0) {
			outCol = vec4(outCol.rgb, 1.0); 
		}
	}
}