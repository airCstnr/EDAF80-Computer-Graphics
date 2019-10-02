#version 410

// cube map
uniform samplerCube cube_map;


in VS_OUT {
	vec3 vertex;
} fs_in;

out vec4 frag_color;

void main()
{
	// cube map texture
	frag_color = texture(cube_map, fs_in.vertex);
}
