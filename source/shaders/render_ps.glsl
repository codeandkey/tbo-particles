#pragma once

#define GLSL(src) "#version 330\n" #src

const char* SHADER_RENDER_PS = GLSL(
	uniform sampler2D render_texture;
	uniform vec3 render_color;
	in vec2 pixel_texcoord;
	out vec4 pixel_color;

	void main(void) {
		vec4 offset = vec4(0.1f, 0.1f, 0.1f, 0.0f);

		pixel_color = (vec4(render_color, 1.0f) / 10.0f); 
	}
);
