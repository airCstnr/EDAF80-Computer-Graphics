#version 410

/*
	EDAF 80 Fall 2019
	Raphael Castanier
	Niklas Karlsson
*/

// uniform vectors
uniform vec3 light_position;	// defined in world space
uniform vec3 camera_position;	// defined in world space
uniform vec3 ambient;			// material ambient 
uniform vec3 diffuse;			// material diffuse
uniform vec3 specular;			// matieral specular

// uniform floats
uniform float shininess;		// material shininess 

in VS_OUT {
	vec3 normal;				// from RAST/VS
	vec3 light;					// from RAST/VS
	vec3 view;					// from RAST/VS
} fs_in;

out vec4 frag_color;			// pixel color

void main()
{
		//Normalize the vecotors
		vec3 N = normalize(fs_in.normal);
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
