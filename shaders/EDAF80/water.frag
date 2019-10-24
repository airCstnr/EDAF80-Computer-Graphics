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

// Uniforms for computing wave derivatives
uniform float amplitude[2] = {2.0, 0.5};
uniform float frequency[2] = {0.2, 0.4};
uniform float phase[2]     = {0.5, 1.3};
uniform float sharpness[2] = {2.0, 2.0};
uniform vec2  direction[2] = {vec2(-1.0, 0.2), vec2(-0.7, 0.7)};

// Uniform samplerCube
uniform samplerCube cube_map;

// Uniform sampler2D
uniform sampler2D wave_ripple_texture;

// Function to compute wave derivative in x direction
float wave_der_x(float x, float z, float t, int wave) {
	// formula from course
	float ret = direction[wave][0] * x + direction[wave][1] * z;
	ret *= frequency[wave];
	ret += t * phase[wave];
	float sin_part = sin(ret) * 0.5 + 0.5;
	float cos_part = cos(ret);
	sin_part = pow(sin_part, sharpness[wave]-1);
	float ans = 0.5 * sharpness[wave] * frequency[wave] * amplitude[wave] * sin_part * cos_part * direction[wave][0];
	return ans;
}

// Function to compute wave derivative in z direction
float wave_der_z(float x, float z, float t, int wave) {
	// formula from course
	float ret = direction[wave][0] * x + direction[wave][1] * z;
	ret *= frequency[wave];
	ret += t * phase[wave];
	float sin_part = sin(ret) * 0.5 + 0.5;
	float cos_part = cos(ret);
	sin_part = pow(sin_part, sharpness[wave]-1);
	float ans = 0.5 * sharpness[wave] * frequency[wave] * amplitude[wave] * sin_part * cos_part * direction[wave][1];
	return ans;
} 

in VS_OUT {
	vec2 normalCoord0;			// from RAST/VS
	vec2 normalCoord1;			// from RAST/VS
	vec2 normalCoord2;			// from RAST/VS
	vec3 vertex;				// from RAST/VS
} fs_in;

out vec4 frag_color;

void main()
{

	// Compute parital derivatives of the wave
	float xder = wave_der_x(fs_in.vertex.x, fs_in.vertex.z, time, 0) + wave_der_x(fs_in.vertex.x, fs_in.vertex.z, time, 1);
	float zder = wave_der_z(fs_in.vertex.x, fs_in.vertex.z, time, 0) + wave_der_z(fs_in.vertex.x, fs_in.vertex.z, time, 1);

	// Compute view vector
	vec3 V = normalize(camera_position - fs_in.vertex);		
	
	// Calculate normals using the parital derivatives of the wave
	vec3 n = vec3(-xder, 1.0, -zder);
	n = normalize(n);
	
	// Animated Normal mapping

		// Define TBN matrix
		vec3 t = vec3(1, xder, 0);
		vec3 b = vec3(0, zder, 1);
		mat3 TBN = mat3(t, b, n);

		// look up and remap (to [-1,-1]) from the three normalCoord calculated in vertex shader
		vec3 n0			= texture(wave_ripple_texture, fs_in.normalCoord0).rgb*2 -1; 
		vec3 n1			= texture(wave_ripple_texture, fs_in.normalCoord1).rgb*2 -1; 
		vec3 n2			= texture(wave_ripple_texture, fs_in.normalCoord2).rgb*2 -1; 

		// Superposition
		vec3 n_ripple	= normalize(n0 + n1 + n2); // looked up normals are in tangent space

		// transform to object space
		n_ripple		= TBN * n_ripple;

		// Use the new normals for the rest of computations
		n = normalize(vec3(normal_model_to_world * vec4(n_ripple, 0.0)));

		// Are we looking from under och above the water?

			// Refraction index (material constant, how fast light travels trough the material)

			// if we are above water
			float refraction_index = 1.0/1.33;				// air to water
			float R0 = 0.02037;								// air to water

			// if we are under water
			if (!gl_FrontFacing){
				n = -n;										// if we look from under water, flip the nomral vector
				refraction_index = 1.33;					// water to air
				float n1 = 1, n2 = 1.33;
				R0 = ((n2-n1)/(n1+n2))*((n2-n1)/(n1+n2));	// water to air
			}

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
		
			// Compute the refraction vector
			vec3 refraction_vector = refract(-V, n, refraction_index);

			// Find out the refraction color 
			vec4 refraction_color = texture(cube_map, refraction_vector);

	// Fresnel terms (reflection and refraction)
	// How much light reflacts at a glancing angle

		//float R0 = 0.02037; // air to water
		float fastFresnel = R0 + (1-R0)*pow((1 - dot(V,n)),5.0);
		

		// Update reflection  and refraction color with fresnel term
		reflection_color = reflection_color;// * fastFresnel;
		refraction_color = refraction_color * (1-fastFresnel);

	//Water color

		vec4 color_deep		= vec4(0.0, 0.0, 0.1, 1.0);			// deep color
		vec4 color_shallow	= vec4(0.0, 0.5, 0.5, 1.0);			// shallow color
		float facing		= 1 - max(dot(V, n), 0.0);			// compute facing component

	// Output pixel color

		frag_color = mix(color_deep, color_shallow, facing) +  reflection_color + refraction_color;


}
