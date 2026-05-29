// 8-Assn-BumpyMesh.cpp, Saxton Van Dalsen, Assn8: Animate, 2026-05-29

#include <glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "Camera.h"
#include "Draw.h"
#include "GLXtras.h"
#include "IO.h"

// Display
int			winW = 900, winH = 900;
Camera		camera(0, 0, winW, winH, {10, 0, 0}, {0, 0, -4});
vec3		red(1, 0, 0), blu(0, 0, 1), grn(.1f, .5f, .1f), mag(1, 0, 1);

// Lights
vec3		lights[] = { {1.3f, -.4f, .45f}, {-.4f, .6f, 1.f}, {.02f, .01f, .85f} };
const int	nLights = sizeof(lights)/sizeof(vec3);

// Interaction
Mover		mover;
void	   *picked = NULL;

// Mesh Class and Airplane

struct MeshS {
	vector<vec3> points, normals;
	vector<int3> triangles;
	mat4		 toWorld;
	GLuint		 program = 0;
	GLuint		 VAO = 0, VBO = 0, EBO = 0;		// vertex array object, vertex buffer, element buffer
	void Read(const char *filename) {
		if (!ReadAsciiObj(filename, points, triangles, &normals))
			printf("can't read %s\n", filename);
		else {
			// create vertex array object and buffer object
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			// allocate, load memory for points and normals
			int sPoints = points.size()*sizeof(vec3), sNormals = normals.size()*sizeof(vec3);
			glBufferData(GL_ARRAY_BUFFER, sPoints+sNormals, NULL, GL_STATIC_DRAW);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sPoints, points.data());
			glBufferSubData(GL_ARRAY_BUFFER, sPoints, sNormals, normals.data());
			// copy triangle data to GPU element buffer
			glGenBuffers(1, &EBO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangles.size()*sizeof(int3), triangles.data(), GL_STATIC_DRAW);
		}
	}
	void Render(vec3 color) {
		const char *vertexShader = R"(
			#version 410 core
			in vec3 point, normal;
			out vec3 vPoint, vNormal;
			uniform mat4 modelview, persp;
			void main() {
				vPoint = (modelview*vec4(point, 1)).xyz;
				vNormal = (modelview*vec4(normal, 0)).xyz;
				gl_Position = persp*vec4(vPoint, 1);
			}
		)";
		const char *pixelShader = R"(
			#version 410 core
			in vec3 vPoint, vNormal;
			uniform int nLights = 0;
			uniform vec3 lights[20];
			uniform float amb = .1, dif = .7, spc =.7;		// ambient, diffuse, specular
			uniform vec3 color = vec3(1, 1, 1);
			out vec4 pColor;
			void main() {
				float d = 0, s = 0;							// diffuse, specular terms
				vec3 N = normalize(vNormal);
				vec3 E = normalize(-vPoint);				// eye vector
				for (int i = 0; i < nLights; i++) {
					vec3 L = normalize(lights[i]-vPoint);	// light vector
					vec3 R = reflect(-L, N);				// highlight vector
					d += max(0, dot(N, L));					// one-sided diffuse
					float h = max(0, dot(R, E));			// highlight term
					s += pow(h, 100);						// specular term
				}
				float ads = clamp(amb+dif*d+spc*s, 0, 1);
				pColor = vec4(ads*color, 1);
			}
		)";
		if (!program)
			program = LinkProgramViaCode(&vertexShader, &pixelShader);
		glUseProgram(program);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		VertexAttribPointer(program, "point", 3, 0, (void *) 0);
		VertexAttribPointer(program, "normal", 3, 0, (void *) (points.size()*sizeof(vec3)));
		SetUniform(program, "modelview", camera.modelview*toWorld);
		SetUniform(program, "persp", camera.persp);
		SetUniform(program, "color", color);
		SetUniform(program, "nLights", nLights);
		SetUniform3v(program, "lights", nLights, (float *) lights, camera.modelview);
		glDrawElements(GL_TRIANGLES, 3*triangles.size(), GL_UNSIGNED_INT, 0);
	}
};

// Flight Path

// TODO: Bezier Class, Flight Path Control Points

// Animation

void Animate() {
	// TODO: compute elapsed time, compute frame on path
	// TODO: transform body to path, transform propeller to body
}

// Display

void Display() {
	// clear, depth test, blend
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	// TODO: display airplane, display flight path
	// lights
	UseDrawShader(camera.fullview);
	for (vec3 &l : lights)
		Star(l, 12, red, blu);
	glFlush();
}

// Mouse

void MouseButton(float x, float y, bool left, bool down) {
	picked = NULL;
	if (left && down) {
		// select light?
		for (vec3 &l : lights)
			if (MouseOver(x, y, l, camera.fullview)) {
				picked = lights;
				mover.Down(&l, (int) x, (int) y, camera.modelview, camera.persp);
			}
		if (!picked) {
			picked = &camera;
			camera.Down(x, y, Shift(), Control());
		}
	}
	if (!down)
		camera.Up();
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	if (leftDown) {
		if (picked == lights)
			mover.Drag((int) x, (int) y, camera.modelview, camera.persp);
		if (picked == &camera)
			camera.Drag(x, y);
	}
}

void MouseWheel(float spin) {
	camera.Wheel(spin);
}

// Application

void Resize(int width, int height) {
	glViewport(0, 0, width, height);
	camera.Resize(width, height);
}

int main() {
	// init app window and GL context
	GLFWwindow *w = InitGLFW(100, 100, winW, winH, "Animate");
	// TODO: read meshes
	// callbacks
	RegisterMouseButton(MouseButton);
	RegisterMouseMove(MouseMove);
	RegisterMouseWheel(MouseWheel);
	RegisterResize(Resize);
	// event loop
	while (!glfwWindowShouldClose(w)) {
		Animate();
		Display();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
}