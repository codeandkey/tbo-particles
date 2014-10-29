#pragma once

#define GLSL(src) "#version 330\n" #src

const char* SHADER_RENDER_GS = GLSL(
	layout (points) in;
	layout (triangle_strip, max_vertices = 4) out;

	out vec2 pixel_texcoord;
	uniform mat4 mat_mvp;

	void main(void) {
		float particle_dim = 0.001f;

		pixel_texcoord = vec2(0.0f, 0.0f);
		gl_Position = mat_mvp * (gl_in[0].gl_Position + vec4(-particle_dim, -particle_dim, 0.0f, 0.0f));

		EmitVertex();

		pixel_texcoord = vec2(0.0f, 1.0f);
		gl_Position = mat_mvp * (gl_in[0].gl_Position + vec4(-particle_dim, particle_dim, 0.0f, 0.0f));

		EmitVertex();

		pixel_texcoord = vec2(1.0f, 0.0f);
		gl_Position = mat_mvp * (gl_in[0].gl_Position + vec4(particle_dim, -particle_dim, 0.0f, 0.0f));

		EmitVertex();

		pixel_texcoord = vec2(1.0f, 1.0f);
		gl_Position = mat_mvp * (gl_in[0].gl_Position + vec4(particle_dim, particle_dim, 0.0f, 0.0f));

		EmitVertex();
		EndPrimitive();
	}
);
