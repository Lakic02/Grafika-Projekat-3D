#version 330 core

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inCol;
layout(location = 2) in vec2 inTex;
layout(location = 3) in vec3 inNormal; // Ovo su ona tri broja 0,0,1 iz tvog koda

uniform mat4 uM; 
uniform mat4 uV; 
uniform mat4 uP; 

out vec4 channelCol;
out vec2 channelTex;
out vec3 chWorldPos; // Pozicija fragmenta u svetu
out vec3 chNormal;   // Normala fragmenta

void main()
{
	chWorldPos = vec3(uM * vec4(inPos, 1.0));
	chNormal = mat3(transpose(inverse(uM))) * inNormal; // Transformacija normale
	
	channelCol = inCol;
	channelTex = inTex;
	gl_Position = uP * uV * uM * vec4(inPos, 1.0);
}