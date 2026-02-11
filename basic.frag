#version 330 core

in vec4 channelCol; 
in vec2 channelTex;

out vec4 outCol;

uniform sampler2D uTex;
uniform bool useTex;
uniform bool transparent;
uniform float uAmb; 
uniform vec4 uTint; // NOVA UNIFORMA: Za bojenje rezervisanih sedista

void main()
{
	vec4 resCol;
	if (!useTex) {
		resCol = channelCol;
	}
	else {
		resCol = texture(uTex, channelTex) * channelCol;
		if (!transparent && resCol.a < 1.0) {
			resCol = vec4(resCol.rgb, 1.0); 
		}
	}
    
    // Mnozimo boju sa tintom (ako je uTint plav, sediste ce poplaveti)
    outCol = resCol * uTint;
    outCol.rgb += uAmb; 
}