//  5-Assn-SmoothMesh.cpp, Saxton Van Dalsen, Assn5: Smooth Shading, 2026-05-04

#include <glad.h>
#include <glfw3.h>
#include "GLXtras.h"
#include "VecMat.h"
#include "IO.h"
#include "Camera.h"
#include "Draw.h"
#include "Text.h"
#include "Widgets.h"
#include <vector>

// GPU identifiers
GLuint VAO = 0, VBO = 0, EBO = 0;	// vertex array, vertex buffer, element buffer
GLuint program = 0;					// shader program ID
int winWidth = 800, winHeight = 800;
Camera camera(0, 0, winWidth, winHeight, vec3(15, -30, 0), vec3(0, 0, -5), 30);

vector<vec3> points;
vector<vec3> normals;
vector<vec2> uvs;
vector<int3> triangles;

// file path for the object and texture image
const char *objFilename = "/Users/saxtonvandalsen/Graphics/Apps/Handbag.obj";
const char *textureFilename = "/Users/saxtonvandalsen/Graphics/Apps/brown.png";
GLuint textureName = 0; // id for texture image
int textureUnit = 0; // id for GPU image buffer

// two light positions for the scene
vec3 lights[] = { {.5f, 0, 1}, {1, 1, 0} };

// number of lights being used
const int nLights = sizeof(lights)/sizeof(vec3);

const char *vertexShader = R"(
	#version 410 core
	in vec2 uv;
	in vec3 point;
	in vec3 normal;
	out vec2 vUv;
	out vec3 vPoint;
	out vec3 vNormal;
	uniform mat4 modelview, persp;

	void main() {
		vPoint = (modelview*vec4(point, 1)).xyz;
		vNormal = (modelview*vec4(normal, 0)).xyz;
		gl_Position = persp*vec4(vPoint, 1);
		vUv = uv;
	}
)";

const char *pixelShader = R"(
	#version 410 core
	in vec2 vUv;
	in vec3 vPoint;

	out vec4 pColor;

	uniform int nLights = 0;
	uniform vec3 lights[20];
	uniform float amb = .1, dif = .8, spc = .7;
	uniform sampler2D textureImage;

	void main() {
		vec3 dx = dFdx(vPoint), dy = dFdy(vPoint);
		vec3 N = normalize(cross(dx, dy));

		float d = 0, s = 0;

		for (int i = 0; i < nLights; i++) {
		    vec3 L = normalize(lights[i] - vPoint);
		    d += abs(dot(N, L));

		    vec3 E = normalize(vPoint);
		    vec3 R = reflect(L, N);

		    float h = max(0, dot(R, E));
		    s += pow(h, 100);
		}

		// combine lighting terms
		float intensity = min(1.0, amb + dif*d) + spc*s;

		vec3 col = texture(textureImage, vUv).rgb;
		pColor = vec4(intensity * col, 1);
	}
)";

void Resize(int width, int height) {
	glViewport(0, 0, width, height);
	camera.Resize(width, height);
}

void Display() {
	// clear background
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	// run shader program, enable GPU vertex buffer
	glUseProgram(program);
	// create rotation matrix from mouse position and send to shader
	SetUniform(program, "modelview", camera.modelview);
	SetUniform(program, "persp", camera.persp);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	// connect GPU point and color buffers to shader inputs
	VertexAttribPointer(program, "point", 3, 0, (void *) 0);
	// connect uv buffer data to shader input
	VertexAttribPointer(program, "uv", 2, 0, (void *) sizeof(points));
	// connect texture image to shader
	SetUniform(program, "textureImage", textureUnit);
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(GL_TEXTURE_2D, textureName);
	// send lights to the shader
	SetUniform(program, "nLights", nLights);
	// transform lights into camera/view space
	SetUniform3v(program, "lights", nLights, (float *) lights, camera.modelview);
	// draw all triangle indices from EBO
	glDrawElements(GL_TRIANGLES, 3*triangles.size(), GL_UNSIGNED_INT, 0);
	// adding arcball draw
	glDisable(GL_DEPTH_TEST);
	if (camera.down)
		camera.Draw();
	glFlush();
}

void BufferVertices() {
	// make GPU buffer for points and colors, make it active
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// size of points and uvs
	size_t sPoints = points.size()*sizeof(vec3);
	size_t sNormals = normals.size()*sizeof(vec3);
	size_t sUvs = uvs.size()*sizeof(vec2);
	// allocate GPU memory for points + uvs
	glBufferData(GL_ARRAY_BUFFER, sPoints + sNormals + sUvs, NULL, GL_STATIC_DRAW);
	// copy vertex data to GPU
	glBufferSubData(GL_ARRAY_BUFFER, 0, sPoints, points.data());
	glBufferSubData(GL_ARRAY_BUFFER, sPoints, sNormals, normals.data());
	// copy uv data right after points
	glBufferSubData(GL_ARRAY_BUFFER, sPoints + sNormals, sUvs, uvs.data());
	// copy triangle index data to GPU
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangles.size()*sizeof(int3), triangles.data(), GL_STATIC_DRAW);
}

// update mouse position while left button is held down
void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	if (leftDown)
		camera.Drag(x, y);
}

// adding mouse button ability with camera
void MouseButton(float x, float y, bool left, bool down) {
	if (left && down)
		camera.Down(x, y, Shift(), Control());
	else
		camera.Up();
}

int main() {
	// updated window
	GLFWwindow *w = InitGLFW(100, 100, winWidth, winHeight, "5-Assn-SmoothMesh");
	program = LinkProgramViaCode(&vertexShader, &pixelShader);
	if (!program) {
		printf("can't init shader program\n");
		getchar();
		return 0;
	}
	// load texture image into GPU
	ReadTexture(textureFilename, &textureName);
	if (!ReadAsciiObj((char *) objFilename, points, triangles, &normals, &uvs)) {
		printf("can't read %s\n", objFilename);
		return 1;
	}
	if (!normals.size())
		SetVertexNormals(points, triangles, normals);
	// fit letter into OpenGL view
	Standardize(points.data(), points.size(), .8f);
	// allocate GPU vertex memory
	BufferVertices();
	RegisterMouseMove(MouseMove);
	RegisterMouseButton(MouseButton);
	RegisterResize(Resize);
	// event loop
	while (!glfwWindowShouldClose(w)) {
		Display();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
	// unbind vertex buffer, free GPU memory
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &VBO);
	glfwDestroyWindow(w);
	glfwTerminate();
}
