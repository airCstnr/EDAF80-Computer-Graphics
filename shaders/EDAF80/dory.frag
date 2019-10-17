#version 410

uniform sampler2D dory_texture;

in VS_OUT {
	vec2 texcoord;
} fs_in;

out vec4 frag_color;

void main()
{
	frag_color = texture(dory_texture, fs_in.texcoord);
}
