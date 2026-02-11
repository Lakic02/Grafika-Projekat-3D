#version 330 core

in vec4 channelCol; 
in vec2 channelTex;
in vec3 chWorldPos;
in vec3 chNormal;

out vec4 outCol;

uniform sampler2D uTex;
uniform bool useTex;
uniform bool transparent;
uniform float uAmb; 
uniform vec4 uTint;

// Uniforme za svetlo koje saljemo iz C++
uniform vec3 uLightPos;
uniform vec3 uLightCol;
uniform float uLightIntensity;

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

    // --- OSVETLJENJE ---
    vec3 normal = normalize(chNormal);
    vec3 lightDir = normalize(uLightPos - chWorldPos);
    
    // Slabljenje svetla sa daljinom (attenuation)
    float dist = distance(uLightPos, chWorldPos);
    float attenuation = 1.0 / (1.0 + 0.05 * dist + 0.02 * (dist * dist));

    // Difuzno svetlo
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuseEffect = uLightCol * diff * uLightIntensity * attenuation;

    // Finalni proracun: Osnovna boja * (Svetlost + Ambijentalni nivo)
    outCol = resCol * uTint;
    outCol.rgb = outCol.rgb * (diffuseEffect + uAmb + 0.05); // 0.05 je minimalni mrak da se bar nesto vidi
}