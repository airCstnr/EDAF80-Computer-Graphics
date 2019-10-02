#version 410

/*
	EDAF 80 Fall 2019
	Raphael Castanier
	Niklas Karlsson
*/

// uniform matrices 
uniform mat4 normal_model_to_world;			// Model -> World (normal)

// uniform vectors
uniform vec3 light_position;				//defined in world space
uniform vec3 camera_position;				//defined in world space

// uniform integers
uniform int has_textures;

// unifrom sampler2D
uniform sampler2D fieldstone_bump_texture;
uniform sampler2D opacity_texture;


in VS_OUT {
	vec3 normal;				// from RAST/VS
	vec3 texcoord;				// from RAST/VS
	vec3 tangent;				// from RAST/VS
	vec3 binormal;				// from RAST/VS
} fs_in;

out vec4 frag_color;			// pixel color

void main()
{

	// Normal mapping = obtain normal from normal-map instead of interpolation from vertices 

	// look up r,g,b from texture
	// map range [0,1] to [-1, 1] 
	// multiply looked up normals with TBN and model_to_world to get into the correct space

	vec3 n;
	if (has_textures != 0) {
		// obtain normal/bump map in range [0,1]
		//n = texture(fieldstone_bump_texture, fs_in.texcoord).rgb;
		// transform vector to range [-1, 1]
		n = normalize(n * 2.0 -1.0);
	}

	mat3 TBN			= mat3(fs_in.tangent, fs_in.binormal ,fs_in.normal);	// define the TBN matrix than transforms from tangent space to model space
	vec3 N;
	// multiply looked up normals with TBN and model_to_world to get into the correct space
	// N = normal_model_to_world * TBN * n;

	frag_color = vec4((normalize(fs_in.tangent) + 1.0) / 2.0, 1.0);
}
