
#define PI 3.1415926538
#define TWO_PI 6.2831853076
#define HALF_PI 1.5707963269

in vec2 texCoords;

struct Light {
	vec3 position;
	vec3 direction;
	vec3 emission;
	float linear;
	float quadratic;
	int type;
	bool state;
	float cutoff;
};

uniform cameraBlock {
   	mat4 ViewMatrix;
	mat4 ProjMatrix;
	mat4 ViewProjMatrix;
	vec3 ViewPos;
};

uniform Light lights[NUM_LIGHTS];

// Shadow filtering
const float baseBias = 0.005f;

// RSM Variables
uniform vec2 VPLSamples[NUM_VPL];
uniform float rsmRMax;
uniform float rsmIntensity;

/* ==============================================================================
        Deferred Shading
 ============================================================================== */
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gDiffuse;
uniform sampler2D gSpecular;
uniform sampler2D gLightSpacePosition;

// assuming global value for shininess for now...
const float shininess = 16.0f;

/* ==============================================================================
        Directional / Spot Lights
 ============================================================================== */
uniform sampler2D depthMap;
uniform sampler2D positionMap;
uniform sampler2D normalMap;
uniform sampler2D fluxMap;

float shadowFactor(vec4 lightSpacePosition, vec3 N, vec3 L){
	// perform perspective divide: clip space-> normalized device coords (done automatically for gl_Position)
    vec3 projCoords = lightSpacePosition.xyz / lightSpacePosition.w;
    // bring from [-1,1] to [0,1]
    projCoords = projCoords * 0.5 + 0.5;

    if(projCoords.z > 1.0)
        return 1.0;

    float currentDepth = projCoords.z; 

    float bias = baseBias * tan(acos(dot(N,L)));
    bias = clamp(bias, 0.0, 0.0005f);

    //float shadow = currentDepth - bias > closestDepth  ? 0.0 : 1.0;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(depthMap, 0));
    for(int x = -1; x <= 1; x++){
    	for(int y = -1; y <= 1; y++){
    		float depth = texture(depthMap, projCoords.xy + vec2(x,y) * texelSize).r;
    		shadow += currentDepth - bias > depth ? 0.0 : 1.0;
    	}
    }

    shadow = shadow / 9.0;

    return shadow;
}

/* ==============================================================================
        Illumination
 ============================================================================== */

 uniform sampler2D lowResIndirect;

vec3 indirectIllumination(vec3 FragPos, vec4 LightSpacePos, vec3 Normal, vec3 Diffuse) {
	vec3 result = vec3(0.0);
	vec3 indirect = vec3(0.0);
	// perform perspective divide: clip space-> normalized device coords (done automatically for gl_Position)

    vec3 projCoords = LightSpacePos.xyz / LightSpacePos.w;

    // bring from [-1,1] to [0,1]
    projCoords = projCoords * 0.5 + 0.5;

    for(int i=0; i < NUM_VPL; i++){
    	vec2 rnd = VPLSamples[i];
    	vec2 coords = vec2(projCoords.x + rsmRMax*rnd.x*sin(TWO_PI*rnd.y), projCoords.y + rsmRMax*rnd.x*cos(TWO_PI*rnd.y));

    	vec3 vplP = texture(positionMap, coords.xy).xyz;
    	vec3 vplN = normalize(texture(normalMap, coords.xy)).xyz;
    	vec3 vplFlux = texture(fluxMap, coords.xy).rgb;

    	float dot1 = max(0.0, dot(vplN, FragPos - vplP));
    	float dot2 = max(0.0, dot(normalize(Normal), vplP - FragPos));

    	float dist = length(vplP - FragPos);

    	indirect += vplFlux * (dot1 * dot2) / (dist * dist * dist * dist);

    	float weight = rnd.x * rnd.x;

    	indirect = indirect * weight;
    	result += indirect;
    }

	return (result * Diffuse) * rsmIntensity;
}

vec3 directIllumination(vec3 FragPos, vec4 LightSpacePos, vec3 Normal, vec3 Diffuse, vec3 Specular) {
	vec3 V = normalize(ViewPos - FragPos);
	vec3 N = Normal;
	vec3 L;

	float shadow = 1.0;
	vec3 retColor = vec3(0.0);

	for(int i=0; i < NUM_LIGHTS; i++){

		// Get Light Vector
		if (lights[i].type == 0){
			L = normalize(-lights[i].direction);

			shadow = shadowFactor(LightSpacePos, N, L);
		}
		else {
			L = normalize(lights[i].position - FragPos);
		}

		if (lights[i].type == 2){
			float theta = dot(L, normalize(-lights[i].direction));
			if(theta <= lights[i].cutoff){
				continue;
			}
		}

		vec3 H = normalize(L + V);

		float NdotL = max(dot(N, L), 0.0);
		float NdotH = max(dot(N, H), 0.0);

		vec3 diff = vec3(0.0);
		vec3 spec = vec3(0.0);

		if (NdotL > 0.0){

			diff = lights[i].emission * ( Diffuse * NdotL);
			spec = lights[i].emission * ( Specular * pow(NdotH, shininess));

			// if not directional light
			if (lights[i].type != 0){
				float distance = length(lights[i].position - FragPos);
				float attenuation = 1.0 / (1.0 + lights[i].linear * distance + lights[i].quadratic * pow(distance, 2.0));

				diff *= attenuation;
				spec *= attenuation;
			}
		}

		retColor += (diff + spec) * shadow;
	}

	return retColor;
}

/* ==============================================================================
        Stage Outputs
 ============================================================================== */

out vec4 outColor;

const float maxDist = 2.0;
const float minDot = 0.8;

void main(void) {

	vec3 pos      = texture(gPosition, texCoords).rgb;
	vec3 N        = texture(gNormal, texCoords).rgb;
	vec3 diffuse  = texture(gDiffuse, texCoords).rgb;
	vec3 specular = texture(gSpecular, texCoords).rgb;
	vec4 lightSpacePos = texture(gLightSpacePosition, texCoords);

	//outColor = vec4(directIllumination(pos, lightSpacePos, N, diffuse, specular) + indirectIllumination(pos, lightSpacePos, N, diffuse), 1.0);
	vec3 direct = directIllumination(pos, lightSpacePos, N, diffuse, specular);
	//outColor = vec4(texture(lowResIndirect, texCoords).rgb, 1.0);

	//first check if interpolation is viable
	vec2 texelSize = 1.0 / vec2(textureSize(lowResIndirect, 0));

	vec4 sampleX;
	vec4 sampleY;
	vec4 viable = vec4(1.0);
	int viableSamples = 4;

	sampleX[0] = texCoords.x - texelSize.x ; sampleY[0] = texCoords.y;
	sampleX[1] = texCoords.x               ; sampleY[1] = texCoords.y + texelSize.y;
	sampleX[2] = texCoords.x               ; sampleY[2] = texCoords.y - texelSize.y;
	sampleX[3] = texCoords.x + texelSize.x ; sampleY[3] = texCoords.y;

	/** /
	vec2 left  = texCoords + vec2(-1.0, 0.0) * texelSize;
	vec2 up    = texCoords + vec2(0.0, 1.0)  * texelSize;
	vec2 down  = texCoords + vec2(0.0, -1.0) * texelSize;
	vec2 right = texCoords + vec2(1.0, 0.0)  * texelSize;
	/**/

	// must have close World pos... what does that mean?
	for(int i = 0; i < 4; i++){
		if( length(pos - texture(gPosition, vec2(sampleX[i], sampleY[i])).rgb) > maxDist ){
			viable[i] = 0.0;
			viableSamples -= 1;
		}
	}

	// must have similar normal... dot >= some value?
	for(int i = 0; i < 4; i++){
		if(viable[i] == 0.0){
			continue;
		}
		else{
			if(dot(N, texture(gNormal, vec2(sampleX[i], sampleY[i])).rgb) <= minDot){
				viable[i] = 0.0;
				viableSamples -= 1;
			}
		}
	}

	vec3 indirect = vec3(0.0);

	if(viableSamples >= 3){

		for(int i = 0; i < 4; i++){
			if(viable[i] == 1.0){
				indirect += texture(lowResIndirect, vec2(sampleX[i], sampleY[i])).rgb;
			}
		}

		// TEMPORARY "NORMALIZATION"
		indirect = indirect / float(viableSamples);
		outColor = vec4(direct + indirect, 1.0);
		//outColor = vec4(1.0);
	}
	//if not, do raw indirect illum call
	else{
		outColor = vec4(direct + indirectIllumination(pos, lightSpacePos, N, diffuse), 1.0);
	}

}