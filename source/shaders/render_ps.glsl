#pragma once

#define GLSL(src) "#version 330\n" #src

const char* SHADER_RENDER_PS = GLSL(
	uniform sampler2D render_texture;
	in vec2 pixel_texcoord;
	out vec4 pixel_color;

	void main(void) {
		pixel_color = vec4(0.1f, 0.1f, 0.1f, 1.0f); 
	}
);
