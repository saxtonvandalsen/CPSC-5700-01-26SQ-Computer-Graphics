// ColorfulTriangle - draw RGB triangle using glDrawElements
// 2-Assn-RotateLetter.cpp, Saxton van Dalsen, 2 Rotate 2D Letter, 2026-04-12

#include <glad.h>
#include <glfw3.h>
#include "GLXtras.h"
#include "VecMat.h"

// GPU identifiers
GLuint VAO = 0, VBO = 0, EBO = 0;	// vertex array, vertex buffer, element buffer
GLuint program = 0;					// shader program ID
vec2 mouseNow;  // current mouse position

// a triangle (3 2D locations, 3 RGB colors)
vec2 points[] = {
    {50, 350},   // v0 top-left
    {200, 350},  // v1 top-right curve
    {240, 280},  // v2 upper-right
    {240, 120},  // v3 lower-right
    {200, 50},   // v4 bottom-right curve
    {50, 50},    // v5 bottom-left
    {125, 200}   // v6 center point
};

vec3 colors[] = {
	{1, 0, 0},   // v0 red
	{0, 1, 0},   // v1 green
	{0, 0, 1},   // v2 blue
	{1, 1, 0},   // v3 yellow
	{1, 0, 1},   // v4 magenta
	{0, 1, 1},   // v5 cyan
	{0.5, 0.5, 0.5} // v6 gray for center
};

// number of vertices in points array
const int nPoints = sizeof(points)/sizeof(vec2);

int triangles[][3] = {
	{0, 1, 6},   // top triangle
	{1, 2, 6},   // upper-right
	{2, 3, 6},   // right side
	{3, 4, 6},   // lower-right
	{4, 5, 6},   // bottom
	{5, 0, 6},   // left side
	{0, 6, 5}    // extra fill for coverage
};

// number of triangle indices (used for drawing)
const int nTriangles = sizeof(triangles)/sizeof(triangles[0]);

const char *vertexShader = R"(
	#version 410 core				// 130 for Windows
	in vec2 point;
	in vec3 color;
	out vec3 vColor;
	uniform mat4 view;

	void main() {
		gl_Position = view*vec4(point, 0, 1);
		vColor = color;
	}
)";

const char *pixelShader = R"(
	#version 410 core				// 130 for Windows
	in vec3 vColor;
	out vec4 pColor;
	void main() {
		pColor = vec4(vColor, 1);	// 1: fully opaque
	}
)";

void Display() {
	// clear background
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	// run shader program, enable GPU vertex buffer
	glUseProgram(program);
	// create rotation matrix from mouse position and send to shader
	mat4 view = RotateY(mouseNow.x)*RotateX(mouseNow.y);
	SetUniform(program, "view", view);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	// connect GPU point and color buffers to shader inputs
	VertexAttribPointer(program, "point", 2, 0, (void *) 0);
	VertexAttribPointer(program, "color", 3, 0, (void *) sizeof(points));
	// draw all triangle indices from EBO
	glDrawElements(GL_TRIANGLES, 3*nTriangles, GL_UNSIGNED_INT, 0);
	glFlush();
}

void BufferVertices() {
	// make GPU buffer for points and colors, make it active
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// allocate buffer memory for vertex locations and colors
	size_t sPoints = sizeof(points), sColors = sizeof(colors);
	// allocate GPU memory for points and colors
	glBufferData(GL_ARRAY_BUFFER, sPoints+sColors, NULL, GL_STATIC_DRAW);
	// copy vertex data to GPU
	glBufferSubData(GL_ARRAY_BUFFER, 0, sPoints, points);
		// start at beginning of buffer, for length of points array
	glBufferSubData(GL_ARRAY_BUFFER, sPoints, sColors, colors);
	// copy triangle index data to GPU
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3*nTriangles*sizeof(int), triangles, GL_STATIC_DRAW);
}

// scale and center points so letter fits in window
void StandardizePoints(float s = .8f) {
	vec2 min, max;
	float range = Bounds(points, nPoints, min, max);
	float scale = 2*s/range;
	vec2 center = (min+max)/2;
	for (int i = 0; i < nPoints; i++)
		points[i] = scale*(points[i]-center);
}

// update mouse position while left button is held down
void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	if (leftDown)
		mouseNow = vec2(x, y);
}

int main() {
	// initialize window
	GLFWwindow *w = InitGLFW(100, 100, 800, 800, "Colorful Triangle");
	program = LinkProgramViaCode(&vertexShader, &pixelShader);
	if (!program) {
		printf("can't init shader program\n");
		getchar();
		return 0;
	}
	// fit letter into OpenGL view
	StandardizePoints(.8f);
	// allocate GPU vertex memory
	BufferVertices();
	RegisterMouseMove(MouseMove);
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
