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
uniform vec3 camera_position;			// defined in world space
uniform vec3 light_position;			//defined in world space

// Uniform floats
uniform float time;						// current time elapsed

// Uniform samplerCube
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
	vec3 V = normalize(camera_position - normalize(fs_in.vertex));		

	// Compute ligth vector
	vec3 L = normalize(light_position - normalize(fs_in.vertex));
	
	// Calculate normals using the parital derivatives of the wave
	vec3 n = vec3(-fs_in.xder, 1.0, -fs_in.zder);
	n = normalize(n);

	// Animated Normal mapping

		// Define TBN matrix
		vec3 t = vec3(1, fs_in.xder, 0);
		vec3 b = vec3(0, fs_in.zder, 1);
		mat3 TBN = mat3(t, b, n);

		// look up and remap (to [-1,-1]) from the three normalCoord calculated in vertex shader
		vec3 n0			= texture(wave_ripple_texture, fs_in.normalCoord0).rgb*2 -1; 
		vec3 n1			= texture(wave_ripple_texture, fs_in.normalCoord1).rgb*2 -1; 
		vec3 n2			= texture(wave_ripple_texture, fs_in.normalCoord2).rgb*2 -1; 

		// Superposition
		vec3 n_ripple	= normalize(n0 + n1 + n2); // looked up normals are in tangent space

		// transform to world space
		n_ripple		= TBN * n_ripple;

		// Use the new normals for the rest of computations
		n = n_ripple;

	// Reflection

		/*
		Reflect a scene's sky box onto a surface
		Use GLSL reflect() function to get direction towards sky box
		Take the color from the points it hit and add to frag_color
		*/

		// Compute reflection vector
		vec3 reflection_vector = normalize(reflect(-V, n));			// Formula from slides

		// Find out the reflection color 
		vec4 reflection_color = texture(cube_map, reflection_vector);

	// Refraction

		/*
		To find out what is inside the water
		*/

		// Refraction index (material constant, how fast light travels trough the material)
		float refraction_index = 1.33;		// air to water
		//float refraction_index = 1.0/1.33;	// water to air

		// Compute the refraction vector
		vec3 refraction_vector = refract(L, n, refraction_index);

		// Find out the refraction color 
		vec4 refraction_color = texture(cube_map, refraction_vector);

	// Fresnel terms (reflection and refraction)
	// How much light reflacts at a glancing angle

		float R0 = 0.02037; // air to water
		float fastFresnel = R0 + (1-R0)*pow((1 - dot(V,n)),5.0);

		// Update reflection  and refraction color with fresnel term
		reflection_color = reflection_color * fastFresnel;
		refraction_color = refraction_color * (1-fastFresnel);

	//Water color

		vec4 color_deep		= vec4(0.0, 0.0, 0.1, 1.0);			// deep color
		vec4 color_shallow	= vec4(0.0, 0.5, 0.5, 1.0);			// shallow color
		float facing		= 1 - max(dot(V, n), 0.0);			// compute facing component

	// Output pixel color

		frag_color = mix(color_deep, color_shallow, facing) +  reflection_color + refraction_color;

}
