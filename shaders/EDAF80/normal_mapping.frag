#version 410

/*
	EDAF 80 Fall 2019
	Raphael Castanier
	Niklas Karlsson
*/

// uniform vectors
uniform vec3 light_position;				//defined in world space
uniform vec3 camera_position;				//defined in world space
uniform vec3 ambient;						// material ambient 
uniform vec3 diffuse;						// material diffuse
uniform vec3 specular;						// matieral specular

// uniform floats
uniform float shininess;					// material shininess 

// uniform integers
uniform int has_textures;

// unifrom sampler2D
uniform sampler2D fieldstone_bump_texture;

in VS_OUT {
	vec3 normal;				// from RAST/VS
	vec2 texcoord;				// from RAST/VS
	vec3 tangent;				// from RAST/VS
	vec3 binormal;				// from RAST/VS
	vec3 light;					// from RAST/VS
	vec3 view;					// from RAST/VS
} fs_in;

out vec4 frag_color;			// pixel color

void main()
{

	// Normal mapping = obtain normal from normal-map instead of interpolation from vertices 

	// look up r,g,b from texture
	// map range [0,1] to [-1, 1] 
	// multiply looked up normals with TBN and model_to_world to get into the correct space

	vec3 n = normalize(fs_in.normal);
	vec3 N = normalize(fs_in.normal);

	// obtain normal/bump map in range [0,1]
	n = texture(fieldstone_bump_texture, fs_in.texcoord).rgb;
	// transform vector to range [-1, 1]
	n = normalize(n * 2.0 -1.0);
	// define the TBN matrix than transforms from tangent space to model space
	mat3 TBN = mat3(fs_in.tangent, fs_in.binormal ,fs_in.normal);
	// multiply looked up normals with TBN to get into world space
	// tangents, binormals and normals are already in worldspace when passed from vertex shader
	N = TBN * n;

	// Continue with light calculations from phong shader (but with new normals)
	//Normalize the vecotors
	vec3 L = normalize(fs_in.light);
	vec3 V = normalize(fs_in.view);
	vec3 R = normalize(reflect(-L,N));

	// Calculate the diffuse and specular term
	vec3 D = diffuse*max(dot(N,L),0.0);
	vec3 S = specular*pow(max(dot(R,V),0.0), shininess);

	// Combine the ambient, diffuse and specular to get final color
	frag_color.xyz = ambient + D + S;
	frag_color.w = 1.0;

}
