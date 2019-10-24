#version 410

/*
	EDAF 80 Fall 2019
	Raphael Castanier
	Niklas Karlsson
*/

// Water Shader for Assignment 4

layout (location = 0) in vec3 vertex;
layout (location = 2) in vec3 texcoord;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

// current time elapsed
uniform float time;

out VS_OUT {
	vec2 normalCoord0;
	vec2 normalCoord1;
	vec2 normalCoord2;
	vec3 vertex;
} vs_out;

// create new wave structure, containing parameters
//struct wave {
//	float amplitude;
//	float direction_x, direction_z;
//	float freq, phase;
//	float sharpness;
//} w1, w2;

// one other way to do it is fixing values in vertex shader
uniform float amplitude[2] = {2.0, 0.5};
uniform float frequency[2] = {0.2, 0.4};
uniform float phase[2]     = {0.5, 1.3};
uniform float sharpness[2] = {2.0, 2.0};
uniform vec2  direction[2] = {vec2(-1.0, 0.2), vec2(-0.7, 0.7)};

// Function for computing vertex altitude from position, time and wave parameters
float y_wave(float x, float z, int wave) {
	// formula from course
	float ret = direction[wave][0] * x + direction[wave][1] * z;
	ret *= frequency[wave];
	ret += time * phase[wave];
	ret = sin(ret) * 0.5 + 0.5;
	ret = pow(ret, sharpness[wave]);
	ret *= amplitude[wave];
	return ret;
}

void main()
{

	// Save vertices in a local copy (can't modify a layout)
	vec3 vert = vertex;

	// change y vertex value using waves
	vert.y += y_wave(vert.x, vert.z, 0);
	vert.y += y_wave(vert.x, vert.z, 1);

	// Animated Normal mapping: coordinates
	// Read three times from the normal map
	// All points in the same direction
	// Here the three coordinates are calculated
	vec2 texScale		= vec2(8,4);
	float normalTime	= mod(time, 100.0);
	vec2 normalSpeed	= vec2(-0.05, 0);

	// Set outputs from water vertex shader
	vs_out.normalCoord0 = vec2(texcoord.xy*texScale + normalTime*normalSpeed);
	vs_out.normalCoord1 = vec2(texcoord.xy*texScale*2 + normalTime*normalSpeed*4);
	vs_out.normalCoord2 = vec2(texcoord.xy*texScale*4 + normalTime*normalSpeed*8);
	vs_out.vertex	= vec3(vertex_model_to_world * vec4(vert, 1.0));

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vert, 1.0);
}



