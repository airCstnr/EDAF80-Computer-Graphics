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

// Uniform floats
uniform float time;	// current time elapsed

// Uniform samplerCubes
uniform samplerCube cube_map;

// Uniform sampler2D
uniform sampler2D wave_ripple_texture;

in VS_OUT {
	float xder;
	float zder;
	vec2 texcoord;
	vec2 normalCoord0;
	vec2 normalCoord1;
	vec2 normalCoord2;
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

	// Reflection

		// Define TBN matrix
		vec3 t = vec3(1, fs_in.xder, 0);
		vec3 b = vec3(0, fs_in.zder, 1);
		mat3 TBN = mat3(t, b, n);

		// Look up normals from cube map (range [0, 1])
		// Combination off skybox.frag and normal_mapping.frag, not sure if this is correct?
		vec3 n_cube = texture(cube_map, fs_in.vertex).rgb;

		// transform vector to range [-1, 1]
		n_cube = normalize(n_cube * 2.0 -1.0);

		// transform looked up normals to world space
		vec3 N = TBN * n_cube;

		// Compute reflection term
		vec3 R = normalize(reflect(-V, N));					// Formula from slides 

	// Animated Normal mapping

		// look up and remap from the three normalCoord calculated in vertex shader
		vec3 n0			= texture(wave_ripple_texture, fs_in.normalCoord0).rgb*2 -1; 
		vec3 n1			= texture(wave_ripple_texture, fs_in.normalCoord1).rgb*2 -1; 
		vec3 n2			= texture(wave_ripple_texture, fs_in.normalCoord2).rgb*2 -1; 

		// Superposition
		vec3 n_ripple	= normalize(n0 + n1 + n2); // looked up normals are in tangent space

		// transform to world space
		n_ripple		= TBN * n_ripple;

		// TODO n_ripple needs to be added to the pixel color computation

	//Water color 
	vec4 color_deep		= vec4(0.0, 0.0, 0.1, 1.0);			// deep color
	vec4 color_shallow	= vec4(0.0, 0.5, 0.5, 1.0);			// shallow color
	float facing		= 1 - max(dot(V, n), 0.0);			// compute facing component

	// Output pixel color
	frag_color = mix(color_deep, color_shallow, facing);
	// Output with the added relection below does not work correctly 
	//frag_color = mix(color_deep, color_shallow, facing) + vec4(R,1.0);

}
