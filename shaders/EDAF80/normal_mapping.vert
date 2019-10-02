#version 410

/*
	EDAF 80 Fall 2019
	Raphael Castanier
	Niklas Karlsson
*/

layout (location = 0) in vec3 vertex;		// defined in model space
layout (location = 1) in vec3 normal;		// defined in model space
layout (location = 2) in vec3 texcoord;
layout (location = 3) in vec3 tangent;		// defined in model space 
layout (location = 4) in vec3 binormal;		// defined in model space

// uniform matrices, set by Node::redner()
uniform mat4 vertex_model_to_world;			// Model -> World (vertex)
uniform mat4 vertex_world_to_clip;			// World -> Clip  (vertex)
uniform mat4 normal_model_to_world;			// Model -> World (normal)

// uniform vectors
uniform vec3 light_position;				//defined in world space
uniform vec3 camera_position;				//defined in world space

out VS_OUT {
	vec3 normal;
	vec2 texcoord;
	vec3 tangent;
	vec3 binormal;
	vec3 light;
	vec3 view;
} vs_out;

void main()
{
	// For texture mapping
	vs_out.normal		= normalize(vec3(normal_model_to_world * vec4(normal, 1.0)));		// transform normal from model to world space
	vs_out.texcoord		= texcoord.xy; 														// pass textcoord to RAST/FS
	vs_out.tangent		= normalize(vec3(normal_model_to_world * vec4(tangent, 1.0)));		// transform tagnent from model to world space
	vs_out.binormal		= normalize(vec3(normal_model_to_world * vec4(binormal, 1.0)));		// transform bitangent from model to world space

	// For phong shading
	vec3 world_position	= (vertex_model_to_world*vec4(vertex,1.0)).xyz;						// transform vertex to world space			
	vs_out.light		= light_position - world_position;									// compute light vector and pass onto RAST/FS
	vs_out.view 		= camera_position - world_position;									// compute view/camera vector and pass onto RAST/FS

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);			// transform vertex position to world space and then clip space
}
