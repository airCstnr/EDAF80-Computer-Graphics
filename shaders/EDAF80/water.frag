#version 410

/*
	EDAF 80 Fall 2019
	Raphael Castanier
	Niklas Karlsson
*/

// Water Shader for Assignment 4

// Uniform matrices
uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

// Uniform vectors
uniform vec3 camera_position;	// defined in world space

// Uniform samplerCubes
uniform samplerCube cube_map;

in VS_OUT {
	float xder;
	float zder;
	vec2 texcoord;
	vec3 vertex;				// from RAST/VS
	vec3 normal;				// from RAST/VS
} fs_in;

out vec4 frag_color;

void main()
{

	// Compute view vector
	vec3 V = camera_position - fs_in.vertex;			
	V = normalize(V);

	// Calculate normals using the parital derivatives of the wave
	vec3 n = vec3(-fs_in.xder, 1.0, -fs_in.zder);
	n = normalize(n);

	// Compute reflection
	// Not sure how to correctly calculate the reflection


	vec3 vert = normalize(fs_in.vertex);
	vec3 R = reflect(-V, n);							// formula from slide
	vec4 reflection = texture(cube_map, fs_in.vertex);  // we need to look up reflection from cube map?


	//Water color 
	vec4 color_deep		= vec4(0.0, 0.0, 0.1, 1.0);			// deep color
	vec4 color_shallow	= vec4(0.0, 0.5, 0.5, 1.0);			// shallow color
	float facing		= 1 - max(dot(V, n), 0.0);			// compute facing component

	// Output pixel color
	frag_color = mix(color_deep, color_shallow, facing) + vec4(R,1.0);

}
