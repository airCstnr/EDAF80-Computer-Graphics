#version 410

/*
	EDAF 80 Fall 2019
	Raphael Castanier
	Niklas Karlsson
*/

// Water Shader for Assignment 4


layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

// current time elapsed
uniform float time;

out VS_OUT {
	vec3 vertex;
	vec3 normal;
} vs_out;

// create new wave structure, containing parameters
//struct wave {
//	float amplitude;
//	float direction_x, direction_z;
//	float freq, phase;
//	float sharpness;
//} w1, w2;

//uniform wave w1 = {01, 0.2, 0.1};
uniform float amplitude[2] = {1.0, 0.5}; // one other way to do it
uniform float frequency[2] = {0.2, 0.4}; // one other way to do it
uniform float phase[2] = {0.5, 1.3}; // one other way to do it
uniform float sharpness[2] = {2.0, 2.0}; // one other way to do it
uniform vec2 direction[2] = {vec2(-1.0, 0.2), vec2(-0.7, 0.7)}; // one other way to do it

// Function for computing vertex altitude from position, time and wave parameters
float y_wave(float x, float z, float t, int wave) {
	// formula from course
	float ret = direction[wave][0] * x + direction[wave][1] * z;
	ret *= frequency[wave];
	ret += t * phase[wave];
	ret = sin(ret) * 0.5 + 0.5;
	ret = pow(ret, sharpness[wave]);
	ret *= amplitude[wave];
	return ret;
}


void main()
{

	vec3 vert = vertex;

	// change y vertex value using waves
	vert.y += y_wave(vert.x, vert.z, time, 0);
	vert.y += y_wave(vert.x, vert.z, time, 1);

	vs_out.vertex = vec3(vertex_model_to_world * vec4(vert, 1.0));
	vs_out.normal = vec3(normal_model_to_world * vec4(normal, 0.0));

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vert, 1.0);
}



