//  4-Assn-Texture3dLetter.cpp, Saxton Van Dalsen, 4 Texture 3D Letter, 2026-04-28

#include <glad.h>
#include <glfw3.h>
#include "GLXtras.h"
#include "VecMat.h"
#include "IO.h"
#include "Camera.h"
#include "Draw.h"
#include "Text.h"
#include "Widgets.h"

// GPU identifiers
GLuint VAO = 0, VBO = 0, EBO = 0;	// vertex array, vertex buffer, element buffer
GLuint program = 0;					// shader program ID
int winWidth = 800, winHeight = 800;
Camera camera(0, 0, winWidth, winHeight, vec3(15, -30, 0), vec3(0, 0, -5), 30);

// a triangle (3 2D locations, 3 RGB colors)
vec3 points[] = {
    {50, 350, 0},   // v0 top-left
    {200, 350, 0},  // v1 top-right curve
    {240, 280, 0},  // v2 upper-right
    {240, 120, 0},  // v3 lower-right
    {200, 50, 0},   // v4 bottom-right curve
    {50, 50, 0},    // v5 bottom-left
    {125, 200, 0},   // v6 center point

		// back face
	{50, 350, -50},   // v7
	{200, 350, -50},  // v8
	{240, 280, -50},  // v9
	{240, 120, -50},  // v10
	{200, 50, -50},   // v11
	{50, 50, -50},    // v12
	{125, 200, -50}   // v13
};

// number of vertices in points array
const int nPoints = sizeof(points)/sizeof(vec3);

// texture coordinates for each point
vec2 uvs[nPoints];

// file path for the star wars png file
const char *textureFilename = "/Users/saxtonvandalsen/Graphics/Apps/star-wars-prequels.png";
GLuint textureName = 0; // id for texture image
int textureUnit = 0; // id for GPU image buffer

// two light positions for the scene
vec3 lights[] = { {.5f, 0, 1}, {1, 1, 0} };

// number of lights being used
const int nLights = sizeof(lights)/sizeof(vec3);

int triangles[][3] = {
	{0, 1, 6},   // top triangle
	{1, 2, 6},   // upper-right
	{2, 3, 6},   // right side
	{3, 4, 6},   // lower-right
	{4, 5, 6},   // bottom
	{5, 0, 6},   // left side

		// back (reversed, offset by 7)
	{7, 13, 8},
	{8, 13, 9},
	{9, 13, 10},
	{10, 13, 11},
	{11, 13, 12},
	{12, 13, 7},

		// sides
		// edge 0-1
	{0, 1, 7},
	{7, 1, 8},

		// edge 1-2
	{1, 2, 8},
	{8, 2, 9},

		// edge 2-3
	{2, 3, 9},
	{9, 3, 10},

		// edge 3-4
	{3, 4, 10},
	{10, 4, 11},

		// edge 4-5
	{4, 5, 11},
	{11, 5, 12},

		// edge 5-0
	{5, 0, 12},
	{12, 0, 7}
};

// number of triangle indices (used for drawing)
const int nTriangles = sizeof(triangles)/sizeof(triangles[0]);

const char *vertexShader = R"(
	#version 410 core
	in vec2 uv;
	in vec3 point;
	out vec2 vUv;
	out vec3 vPoint;
	uniform mat4 modelview, persp;

	void main() {
		vPoint = (modelview*vec4(point, 1)).xyz;
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

// generate uv coords based on point positions
void SetUvs() {
	vec3 min, max;

	// find bounding box of the points
	Bounds(points, nPoints, min, max);

	// get width/height of the shape
	vec3 dif(max - min);

	// assign uv for each point
	for (int i = 0; i < nPoints; i++) {
		uvs[i] = vec2(
			(points[i].x - min.x) / dif.x,
			(points[i].y - min.y) / dif.y
		);
	}
}

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
	glDrawElements(GL_TRIANGLES, 3*nTriangles, GL_UNSIGNED_INT, 0);
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
	size_t sPoints = sizeof(points), sUvs = sizeof(uvs);
	// allocate GPU memory for points + uvs
	glBufferData(GL_ARRAY_BUFFER, sPoints + sUvs, NULL, GL_STATIC_DRAW);
	// copy vertex data to GPU
	glBufferSubData(GL_ARRAY_BUFFER, 0, sPoints, points);
	// copy uv data right after points
	glBufferSubData(GL_ARRAY_BUFFER, sPoints, sUvs, uvs);
	// copy triangle index data to GPU
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3*nTriangles*sizeof(int), triangles, GL_STATIC_DRAW);
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
	GLFWwindow *w = InitGLFW(100, 100, winWidth, winHeight, "4-Assn-Texture3DLetter");
	program = LinkProgramViaCode(&vertexShader, &pixelShader);
	if (!program) {
		printf("can't init shader program\n");
		getchar();
		return 0;
	}
	// load texture image into GPU
	ReadTexture(textureFilename, &textureName);
	// fit letter into OpenGL view
	Standardize(points, nPoints, .8f);
	// generate uv coordinates for texture mapping
	SetUvs();
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
