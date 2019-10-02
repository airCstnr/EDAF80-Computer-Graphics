#version 410

/*
	EDAF 80 Fall 2019
	Raphael Castanier
	Niklas Karlsson
*/

layout (location = 0) in vec3 vertex;	// defined in model space
layout (location = 1) in vec3 normal;	// defined in model space

// uniform matrices, set by Node::redner()
uniform mat4 vertex_model_to_world;		// Model -> World (vertex)
uniform mat4 vertex_world_to_clip;		// World -> Clip  (vertex)
uniform mat4 normal_model_to_world;		// Model -> World (normal)

// uniform vectors
uniform vec3 light_position;			//defined in world space
uniform vec3 camera_position;			//defined in world space

out VS_OUT {
	vec3 normal;
	vec3 light;
	vec3 view;
} vs_out;

void main()
{
	vec3 world_position	= (vertex_model_to_world*vec4(vertex,1.0)).xyz;		// transform vertex to world space			
	vs_out.normal		= (normal_model_to_world*vec4(normal,1.0)).xyz;	    // transform normal to world space and pass onto RAST/FS
	vs_out.light		= light_position - world_position;					// compute light vector and pass onto RAST/FS
	vs_out.view 		= camera_position - world_position;					// compute view/camera vector and pass onto RAST/FS
	gl_Position			= vertex_world_to_clip*vec4(world_position, 1.0);	// transform vertex position to clip space
}
