/*
 * JT Stanley (github@metredigm) - OpenGL TBO particles program.
 * This program is part a of a 5-hour code rush I'm challenging myself with.
 * 
 * I would do the project in C, but there is no nice library for OpenGL matrices, and implementing this yourself takes _time_.
 * This is a single-source program (not enough time to structure the program much), and I'll try to keep it documented as much as I can.
 *
 * Absolutely requires GLSL version 330 support to run. If you don't have support, (indicated by shader error messages during launch), try upgrading to the proprietary driver set
 *	for your GPU.
 *
 * Unfortunately, Mesa does not have support for GLSL 330 yet.
 */

/* Library includes */

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <ctime>

#include <GLXW/glxw.h>
#include <GLFW/glfw3.h>

#include <IL/il.h>

#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

/* Shader includes */

#include "shaders/render_vs.glsl"
#include "shaders/render_gs.glsl"
#include "shaders/render_ps.glsl"
#include "shaders/advance_vs.glsl"

/* Config defines */

#define PARTICLE_COUNT 175000

#define WINDOW_WIDTH 1366
#define WINDOW_HEIGHT 768
#define WINDOW_VSYNC 0
#define WINDOW_FULLSCREEN 1

/* PARTICLE_TEXTURE has a use and is loaded, but I failed to debug the texture display in the 5-hour time frame. */
#define PARTICLE_TEXTURE "particle.png"

/* Global variable declarations */

static GLFWwindow* window_handle = NULL;
static glm::mat4 projection_matrix;
static float projection_camera_data[4] = {0.0f};

static unsigned int shader_render_vs = 0;
static unsigned int shader_render_gs = 0;
static unsigned int shader_render_ps = 0;
static unsigned int shader_render_program = 0;
static int shader_render_tbo_loc = 0;
static int shader_render_mvp_loc = 0;
static int shader_render_tex_loc = 0;
static int shader_render_color_loc = 0;

static unsigned int shader_advance_vs = 0;
static unsigned int shader_advance_program = 0;
static int shader_advance_tbo_loc = 0;
static int shader_advance_cam_loc = 0;
static int shader_advance_mouse_loc = 0;

static unsigned int particle_buffer_first = 0;
static unsigned int particle_buffer_second = 0;
static unsigned int particle_buffer_first_texture = 0;
static unsigned int particle_buffer_second_texture = 0;

static unsigned int render_texture = 0;

/* Global function declarations */

bool initialize_window(void);
bool update_window(void);
void swap_window(void);
void clear_window(void);

bool initialize_shaders(void);
bool initialize_buffers(void);
void initialize_camera(void);

/* Entry point function definition */

int main(int argc, char** argv) {
	if (!initialize_window()) {
		printf("[main] Failed to initialize window.\n");
		return 1;
	}

	initialize_camera(); // Camera bounds needed for initialize_shaders().

	if (!initialize_shaders()) {
		printf("[main] Failed to initialize shaders.\n");
		return 1;
	}

	if (!initialize_buffers()) {
		printf("[main] Failed to initialize buffers.\n");
		return 1;
	}

	while (update_window()) {
		clear_window();

		/* before we do anything, we update the mouse data. */
		float mouse_data[3] = {0.0f};

		if (glfwGetMouseButton(window_handle, 0) == GLFW_PRESS) {
			mouse_data[2] = 1.0f;
		} else {
			mouse_data[2] = 0.0f;
		}

		double mx, my;

		glfwGetCursorPos(window_handle, &mx, &my); // Conv. from double to float, should be fine
		/* we need to conv. abs mouse pos to world coordinates */
		mouse_data[0] = (((mx / (float) WINDOW_WIDTH) - 0.5f) * 2.0f) * projection_camera_data[1];
		mouse_data[1] = (((my / (float) WINDOW_HEIGHT) - 0.5f) * -2.0f) * projection_camera_data[3];
		
		/* first, we run the particle advance. */
		glUseProgram(shader_advance_program);
		glActiveTexture(GL_TEXTURE0);
		glUniform3f(shader_advance_mouse_loc, mouse_data[0], mouse_data[1], mouse_data[2]); // Can't trust fv anymore.

		glEnable(GL_RASTERIZER_DISCARD); // We're not drawing anything! Save performance.
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, particle_buffer_second);

		glBeginTransformFeedback(GL_POINTS);
		glBindTexture(GL_TEXTURE_BUFFER, particle_buffer_first_texture);
		glDrawArraysInstanced(GL_POINTS, 0, 1, PARTICLE_COUNT);
		glEndTransformFeedback();

		glDisable(GL_RASTERIZER_DISCARD);

		/* We then swap the first and second buffer so that our changes are reflected. */
		unsigned int temp = particle_buffer_first;
		particle_buffer_first = particle_buffer_second;
		particle_buffer_second = temp;

		temp = particle_buffer_first_texture; // Switch the textures too, for good measure.
		particle_buffer_first_texture = particle_buffer_second_texture;
		particle_buffer_second_texture = temp;

		/* Update the TBOs to match. */
		glBindTexture(GL_TEXTURE_BUFFER, particle_buffer_first_texture);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, particle_buffer_first);

		glBindTexture(GL_TEXTURE_BUFFER, particle_buffer_second_texture);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, particle_buffer_second);

		/* ^ Those steps may be unnecessary, if the TBO automatically updates with the new values. (that could mess stuff up though) */

		/* next, bind the render shader. */
		glUseProgram(shader_render_program);

		/* set the color uniform. */

		static float dx = 0.0f; dx += 0.001f;
		float r = sinf(dx);
		float g = cosf(dx); // Make some cool colors.
		float b = 1.0f;

		glUniform3f(shader_render_color_loc, r, g, b);

		/* bind the first TBO. */
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_BUFFER, particle_buffer_first_texture);

		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_BUFFER, render_texture);

		glBindBuffer(GL_ARRAY_BUFFER, 0); // We are using the texture buffer! No need to actually draw anything from the array buffer here.
		glDrawArraysInstanced(GL_POINTS, 0, 1, PARTICLE_COUNT);

		swap_window();
	}	

	glfwTerminate();
	return 0;
};

/* Other function definitions */

void initialize_camera(void) {
	float ratio = (float) WINDOW_WIDTH / (float) WINDOW_HEIGHT;

	projection_matrix = glm::ortho(-ratio / 2.0f, ratio / 2.0f, -0.5f, 0.5f);

	projection_camera_data[0] = -ratio / 2.0f;
	projection_camera_data[1] = ratio / 2.0f;
	projection_camera_data[2] = -0.5f;
	projection_camera_data[3] = 0.5f;
}

bool initialize_shaders(void) {
	shader_render_vs = glCreateShader(GL_VERTEX_SHADER);
	shader_render_gs = glCreateShader(GL_GEOMETRY_SHADER);
	shader_render_ps = glCreateShader(GL_FRAGMENT_SHADER);

	shader_advance_vs = glCreateShader(GL_VERTEX_SHADER);

	glShaderSource(shader_render_vs, 1, &SHADER_RENDER_VS, NULL);
	glShaderSource(shader_render_gs, 1, &SHADER_RENDER_GS, NULL);
	glShaderSource(shader_render_ps, 1, &SHADER_RENDER_PS, NULL);

	glCompileShader(shader_render_vs);
	glCompileShader(shader_render_gs);
	glCompileShader(shader_render_ps);

	int compile_status = 0;

	glGetShaderiv(shader_render_vs, GL_COMPILE_STATUS, &compile_status);

	if (!compile_status) {
		char log[1024] = {0};

		glGetShaderInfoLog(shader_render_vs, 1024, NULL, log);
		printf("[initialize_shaders] VS error : %s\n", log);

		printf("[initialize_shaders] render vertex shader compile fail\n");
		return false;
	}

	glGetShaderiv(shader_render_gs, GL_COMPILE_STATUS, &compile_status);

	if (!compile_status) {
		char log[1024] = {0};

		glGetShaderInfoLog(shader_render_gs, 1024, NULL, log);
		printf("[initialize_shaders] GS error : %s\n", log);

		printf("[initialize_shaders] render geometry shader compile fail\n");
		return false;
	}

	glGetShaderiv(shader_render_ps, GL_COMPILE_STATUS, &compile_status);

	if (!compile_status) {
		char log[1024] = {0};

		glGetShaderInfoLog(shader_render_ps, 1024, NULL, log);
		printf("[initialize_shaders] PS error : %s\n", log);

		printf("[initialize_shaders] render fragment shader compile fail\n");
		return false;
	}

	shader_render_program = glCreateProgram();

	glAttachShader(shader_render_program, shader_render_vs);
	glAttachShader(shader_render_program, shader_render_gs);
	glAttachShader(shader_render_program, shader_render_ps);

	glLinkProgram(shader_render_program);

	int link_status = 0;

	glGetProgramiv(shader_render_program, GL_LINK_STATUS, &link_status);

	if (!link_status) {
		char log[1024] = {0};
		
		glGetProgramInfoLog(shader_render_program, 1024, NULL, log);
		printf("[initialize_shaders] link error : %s\n", log);

		printf("[initialize_shaders] render program link fail\n");
		return false;
	}

	glUseProgram(shader_render_program);

	shader_render_tbo_loc = glGetUniformLocation(shader_render_program, "particle_buffer");
	shader_render_mvp_loc = glGetUniformLocation(shader_render_program, "mat_mvp");
	shader_render_tex_loc = glGetUniformLocation(shader_render_program, "render_texture");
	shader_render_color_loc = glGetUniformLocation(shader_render_program, "render_color");

	if (shader_render_tbo_loc != -1) {
		glUniform1i(shader_render_tbo_loc, 0);
	} else {
		printf("[initialize_shaders] could not locate uniform location for particle_buffer\n");
	}

	if (shader_render_mvp_loc != -1) {
		glUniformMatrix4fv(shader_render_mvp_loc, 1, GL_FALSE, glm::value_ptr(projection_matrix));
	} else {
		printf("[initialize_shaders] could not locate uniform location for mat_mvp\n");
	}

	if (shader_render_tex_loc != -1) {
		glUniform1i(shader_render_tex_loc, 1); // Use texture unit 1 for actual texture rendering.
	} else {
		printf("[initialize_shaders] could not locate uniform location for render_texture (render)\n");
	}

	if (shader_render_color_loc != -1) {
		glUniform3f(shader_render_color_loc, 1.0f, 1.0f, 1.0f);
	} else {
		printf("[initialize_shaders] could not locate uniform location for render_color\n");
	}

	/* TODO : shader loading code for advance shader! */

	shader_advance_vs = glCreateShader(GL_VERTEX_SHADER);

	glShaderSource(shader_advance_vs, 1, &SHADER_ADVANCE_VS, NULL);
	glCompileShader(shader_advance_vs);

	glGetShaderiv(shader_advance_vs, GL_COMPILE_STATUS, &compile_status);

	if (!compile_status) {
		char log[1024] = {0};

		glGetShaderInfoLog(shader_advance_vs, 1024, NULL, log);
		printf("[initialize_shaders] compilation for advance VS failed : %s\n", log);
		return false;
	}	

	shader_advance_program = glCreateProgram();

	glAttachShader(shader_advance_program, shader_advance_vs);

	/* Here we set up the transform feedback. */
	const char* varyings[1] = {"out_particle_data"};
	glTransformFeedbackVaryings(shader_advance_program, 1, varyings, GL_INTERLEAVED_ATTRIBS);
	
	glLinkProgram(shader_advance_program);
	glGetProgramiv(shader_advance_program, GL_LINK_STATUS, &link_status);

	if (!link_status) {
		char log[1024] = {0};

		glGetProgramInfoLog(shader_advance_program, 1024, NULL, log);
		printf("[initialize_shaders] advance program link fail : %s\n", log);

		return false;
	}

	glUseProgram(shader_advance_program);

	shader_advance_tbo_loc = glGetUniformLocation(shader_advance_program, "particle_buffer");
	shader_advance_cam_loc = glGetUniformLocation(shader_advance_program, "camera_bounds");
	shader_advance_mouse_loc = glGetUniformLocation(shader_advance_program, "mouse_data");

	if (shader_advance_tbo_loc != -1) {
		glUniform1i(shader_advance_tbo_loc, 0);
	} else {
		printf("[initialize_shaders] could not locate uniform location for advance particle_buffer\n");
	}

	if (shader_advance_cam_loc != -1) {
		// Uniform4fv is supposed to work, but doesn't
		glUniform4f(shader_advance_cam_loc, projection_camera_data[0], projection_camera_data[1], projection_camera_data[2], projection_camera_data[3]);
	} else {
		printf("[initialize_shaders] could not locate uniform location for advance camera_bounds\n");
	}

	if (shader_advance_mouse_loc != -1) {
		glUniform3f(shader_advance_mouse_loc, 0.0f, 0.0f, 0.0f);
	} else {
		printf("[initialize_shaders] could not locate uniform location for advance mouse_data\n");
	}

	/* We use this opportunity to load the particle render texture. */

	ilInit();

	unsigned int il_image;
	ilGenImages(1, &il_image);
	ilBindImage(il_image);

	if (!ilLoadImage(PARTICLE_TEXTURE)) {
		printf("[initialize_shaders] failed to load particle render texture\n");
		return false;
	}

	ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

	glActiveTexture(GL_TEXTURE0 + 1);
	glGenTextures(1, &render_texture);
	glBindTexture(GL_TEXTURE_2D, render_texture);

	glTexImage2D(render_texture, 0, GL_RGBA, ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT), 0, GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());
	glActiveTexture(GL_TEXTURE0);

	return true;
}

bool initialize_buffers(void) {
	srand(time(NULL));

	glGenBuffers(1, &particle_buffer_first);
	glGenBuffers(1, &particle_buffer_second);

	float* particle_buffer = new float[PARTICLE_COUNT * 4];
	memset(particle_buffer, 0, sizeof(float) * PARTICLE_COUNT * 4);

	for (unsigned int i = 0; i < PARTICLE_COUNT; i++) {
		float* current_particle = particle_buffer + 4 * i;

		current_particle[0] = ((float) rand() / ((float) INT_MAX / 2.0f) - 1.0f) / 1.02f;
		current_particle[1] = ((float) rand() / ((float) INT_MAX / 2.0f) - 1.0f) / 1.02f;
		current_particle[2] = current_particle[3] = 0.0f;
	}

	glBindBuffer(GL_ARRAY_BUFFER, particle_buffer_first);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * PARTICLE_COUNT * 4, particle_buffer, GL_DYNAMIC_COPY);

	glBindBuffer(GL_ARRAY_BUFFER, particle_buffer_second);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * PARTICLE_COUNT * 4, particle_buffer, GL_DYNAMIC_COPY);

	glGenTextures(1, &particle_buffer_first_texture);
	glGenTextures(1, &particle_buffer_second_texture);

	glBindTexture(GL_TEXTURE_BUFFER, particle_buffer_first_texture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, particle_buffer_first);

	glBindTexture(GL_TEXTURE_BUFFER, particle_buffer_second_texture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, particle_buffer_second);

	return true;
}

bool initialize_window(void) {
	glfwInit();

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);

	window_handle = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "particles", WINDOW_FULLSCREEN ? glfwGetPrimaryMonitor() : NULL, NULL);

	if (window_handle == NULL) {
		printf("[initialize_window] GLFW failure\n");
		return false;
	}

	glfwMakeContextCurrent(window_handle);

	if (glxwInit() != 0) {
		printf("[initialize_window] GLXW failure\n");
		return false;
	}

	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ZERO);
	glBlendEquation(GL_FUNC_ADD);

	return true;
}

bool update_window(void) {
	glfwPollEvents();

	return !glfwWindowShouldClose(window_handle);
}

void swap_window(void) {
	glfwSwapInterval(WINDOW_VSYNC);
	glfwSwapBuffers(window_handle);
}

void clear_window(void) {
	glClear(GL_COLOR_BUFFER_BIT);
}
