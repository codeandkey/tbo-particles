#pragma once

#define GLSL(src) "#version 330\n" #src

const char* SHADER_RENDER_VS = GLSL(
	uniform samplerBuffer particle_buffer;
	uniform mat4 mat_mvp;

	void main(void) {
		vec4 particle_data;
		particle_data=texelFetch(particle_buffer, gl_InstanceID);
		gl_Position=vec4(particle_data.x, particle_data.y, 0.0f, 1.0f);
	}
);
