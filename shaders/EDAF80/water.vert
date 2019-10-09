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
float time;

out VS_OUT {
	vec3 vertex;
	vec3 normal;
} vs_out;

// create new wave structure, containing parameters
struct wave {
	float amplitude;
	float direction_x, direction_z;
	float freq, phase;
	float sharpness;
} w1, w2;

// Function for computing vertex altitude from position, time and wave parameters
float y_wave(float x, float z, float t, wave w) {
	// formula from course
	float ret = w.direction_x * x + w.direction_z * z;
	ret *= w.freq;
	ret += t * w.phase;
	ret = sin(ret) * 0.5 + 0.5;
	ret = pow(ret, w.sharpness);
	ret *= w.amplitude;
	return ret;
}


void main()
{

	vs_out.vertex = vec3(vertex_model_to_world * vec4(vertex, 1.0));
	vs_out.normal = vec3(normal_model_to_world * vec4(normal, 0.0));

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);

	// update y position using wave function on the 2 waves
	gl_Position.y += y_wave(gl_Position.x, gl_Position.z, time, w1);
	gl_Position.y += y_wave(gl_Position.x, gl_Position.z, time, w2);
}



