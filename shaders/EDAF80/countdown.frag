#version 410

uniform sampler2D number_three_texture;
uniform sampler2D number_two_texture;
uniform sampler2D number_one_texture;
uniform sampler2D go_texture;

uniform float time;

in VS_OUT {
	vec2 texcoord;
} fs_in;

out vec4 frag_color;

void main()
{
	// Time is the uniform for updating waves, therefore strange values
	// Would look better is changed to ellapsed time
	float alpha = 1.0f;
	if (time < .55) {	
		alpha = texture(number_three_texture, vec2(1.0f - fs_in.texcoord.x, fs_in.texcoord.y)).r;
		if (alpha == 0.0f)
			discard;
		if (alpha != 0.0f)
			frag_color = texture(number_three_texture, fs_in.texcoord);
	}
	else if (0.55 < time && time < 1.1) {
			alpha = texture(number_two_texture, vec2(1.0f - fs_in.texcoord.x, fs_in.texcoord.y)).r;
		if (alpha == 0.0f)
			discard;
		if (alpha != 0.0f)
			frag_color = texture(number_two_texture, fs_in.texcoord);
	}
	else {
		alpha = texture(number_one_texture, vec2(1.0f - fs_in.texcoord.x, fs_in.texcoord.y)).r;
		if (alpha == 0.0f)
			discard;
		if (alpha != 0.0f)
			frag_color = texture(number_one_texture, fs_in.texcoord);
	}

}
