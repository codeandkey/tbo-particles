#pragma once

#define GLSL(src) "#version 330\n" #src

const char* SHADER_ADVANCE_VS = GLSL(
	uniform samplerBuffer particle_buffer;

	out vec4 out_particle_data;
	uniform vec4 camera_bounds;
	uniform vec3 mouse_data;

	void main(void) {
		vec4 particle_data = texelFetch(particle_buffer, gl_InstanceID);

		float bounce_decay = 1.5f;
		float gravitation = 0.0001f;
		float speed_decay = 1.01;

		particle_data.w -= 0.0001f;

		if (particle_data.x <= camera_bounds.x) {
			particle_data.x = camera_bounds.x;
			particle_data.z = -particle_data.z / bounce_decay;
		}

		if (particle_data.x >= camera_bounds.y) {
			particle_data.x = camera_bounds.y;
			particle_data.z = -particle_data.z / bounce_decay;
		}

		if (particle_data.y <= camera_bounds.z) {
			particle_data.y = camera_bounds.z;
			particle_data.w = -particle_data.w / bounce_decay;
		}

		if (particle_data.y >= camera_bounds.w) {
			particle_data.y = camera_bounds.w;
			particle_data.w = -particle_data.w / bounce_decay;
		}

		if (mouse_data[2] == 1.0f) {
			float dist = sqrt(pow(mouse_data[0] - particle_data.x, 2) + pow(mouse_data[1] - particle_data.y, 2));

			float angle = atan(mouse_data[1] - particle_data.y, mouse_data[0] - particle_data.x);

			particle_data.z += (1.0f / (pow(dist, 2) + 0.01f)) * cos(angle) * gravitation;
			particle_data.w += (1.0f / (pow(dist, 2) + 0.01f)) * sin(angle) * gravitation;
		}

		particle_data.z /= speed_decay;
		particle_data.w /= speed_decay;

		particle_data.x += particle_data.z;
		particle_data.y += particle_data.w;
		out_particle_data = particle_data;
	}
);
