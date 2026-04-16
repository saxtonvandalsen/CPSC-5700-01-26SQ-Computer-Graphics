// ClearScreen.cpp: simple OpenGL program with shaders, copyright 2025 J. Bloomenthal, all rights reserved.
// 1-Assn-ClearScreen.cpp, Saxton Van Dalsen, 1 Setup and Pixels, 2024-06-01

#include <glad.h>														// OpenGL routines
#include <glfw3.h>														// OS negotiation 
#include "GLXtras.h"
#include <stdio.h>

int winWidth = 600, winHeight = 600;									// window size, in pixels

GLuint	VAO = 0, VBO = 0;												// vertex array ID, vertex buffer ID
GLuint	program = 0;
vec3 userColor(1, 0, 0); // default green


// vertex shader: operations before the rasterizer
const char *vertexShader = R"(
	#version 330														// minimum version for Mac (130 for Windows)
	in vec2 point;														// 2D input from GPU memory 
	void main() {
		gl_Position = vec4(point, 0, 1);								// promote 2D point to 4D, set built-in output
	}
)";

// pixel shader: operations after the rasterizer
const char *pixelShader = R"(
	#version 330														// minimum version
	out vec4 pColor;
	uniform vec3 userColor;													// 4D color output
	void main() {
		pColor = vec4(userColor, 1);										// set (r,g,b,a) pixel
	}
)";

void Display() {
	glUseProgram(program);
	SetUniform(program, "userColor", userColor);// run shader program
	glBindBuffer(GL_ARRAY_BUFFER, VBO);									// ensure correct buffer
	VertexAttribPointer(program, "point", 2, 0, (void *) 0);			// connect buffer to vertex shader
#ifdef __APPLE__
	glDrawArrays(GL_TRIANGLES, 0, 6);									// send 2 triangles to vertex shader
#else
	glDrawArrays(GL_QUADS, 0, 4);										// send 1 quad to vertex shader
#endif
	glFlush();															// flush OpenGL ops
}

void BufferVertices() {
	vec2 pts[] = { {-1,-1}, {1,-1}, {1,1}, {-1,1}, {-1,-1}, {1,1} };	// 2 triangles or 1 quad define app window
	glGenVertexArrays(1, &VAO);											// create vertex array
	glGenBuffers(1, &VBO);												// create vertex buffer on GPU
	glBindVertexArray(VAO);												// bind (make active) the vertex array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);									// bind vertex buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(pts), pts, GL_STATIC_DRAW);	// copy six vertices to GPU buffer
}

void Keyboard(int key, bool press, bool shift, bool control) {
	if (press && key == 'C') {
		vec3 c;
		printf("type r g b (range 0-1, no commas): ");
		if (fscanf(stdin, "%f%f%f", &c.x, &c.y, &c.z) == 3) {
			userColor = c;
			char title[500];
			sprintf(title, "Clear to (%g, %g, %g)", c.x, c.y, c.z);
			glfwSetWindowTitle(glfwGetCurrentContext(), title);
		}
		rewind(stdin);
	}
}

int main() {															// application entry
	GLFWwindow *w = InitGLFW(150, 150, winWidth, winHeight, "Clear");	// create titled window, must call before GL calls
	program = LinkProgramViaCode(&vertexShader, &pixelShader);			// build shader program
	BufferVertices();
	RegisterKeyboard(Keyboard);// store points in GPU memory
	while (!glfwWindowShouldClose(w)) {									// event loop, check for quit
		Display();                                     
		glfwSwapBuffers(w);												// swap front and back buffers
		glfwPollEvents();												// test for keyboard or mouse event
	}
	glfwDestroyWindow(w);
	glfwTerminate();
}
