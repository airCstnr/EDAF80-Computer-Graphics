#version 410

/*
	EDAF 80 Fall 2019
	Raphael Castanier
	Niklas Karlsson
*/

// Water Shader for Assignment 4

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

uniform vec3 camera_position;	// defined in world space

in VS_OUT {
	vec3 vertex;				// from RAST/VS
	vec3 normal;				// from RAST/VS
} fs_in;

out vec4 frag_color;

void main()
{

	vec3 V 		= camera_position - fs_in.vertex;			// compute view/camera vector

	// Normal computation
	vec3 n				= vec3(0.0, 1.0, 0.0);				// compute normals from shape of wavw

	//Water color 
	vec4 color_deep		= vec4(0.0, 0.0, 0.1, 1.0);			// deep color
	vec4 color_shallow	= vec4(0.0, 0.5, 0.5, 1.0);			// shallow color
	float facing		= 1 - max(dot(V, n),0.0);			// compute facing component

	frag_color = mix(color_deep, color_shallow, color_shallow);	// pixel color is a mix of deep, shallow and facing

}
