//  6-Assn-BumpyMesh.cpp, Saxton Van Dalsen, Assn6: Bézier Curve, 2026-05-18

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

int winWidth = 800, winHeight = 800;
Camera camera(0, 0, winWidth, winHeight, vec3(15, -30, 0), vec3(0, 0, -5), 30);

// seven Bézier control points
vec3 controlPoints[] = {
	{-1.5f,  0.5f, 0},
	{-1.0f,  1.5f, 0},
	{-0.5f, -1.5f, 0},
	{ 0.0f,  0.0f, 0},
	{ 0.5f,  1.5f, 0},
	{ 1.0f, -1.5f, 0},
	{ 1.5f,  0.5f, 0}
};

// currently selected control point
void *selectedPoint = NULL;

// animation timing for moving dot
float startTime = (float) glfwGetTime();
float duration = 4;

// compute point on one cubic Bézier curve
vec3 Bezier(vec3 *pts, float t) {
	float u = 1-t;

	return
		pts[0]*u*u*u +
		pts[1]*3*u*u*t +
		pts[2]*3*u*t*t +
		pts[3]*t*t*t;
}

void Resize(int width, int height) {
	glViewport(0, 0, width, height);
	camera.Resize(width, height);
}

// draw control points, curve, and animated point
void Display() {
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	// use Draw.h shader instead of old texture shader
	UseDrawShader(camera.fullview);

	mat4 view = camera.modelview;
	mat4 persp = camera.persp;

	// draw control polygon
	for (int i = 0; i < 6; i++)
		LineDash(controlPoints[i], controlPoints[i+1], 2, vec3(0, 0, 1), vec3(0, 0, 1));

	// draw control points
	for (int i = 0; i < 7; i++)
		Disk(controlPoints[i], 10, vec3(1, 0, 0), *view, persp);

	// draw first cubic curve using points 0-3
	vec3 prev = Bezier(&controlPoints[0], 0);
	for (int i = 1; i <= 100; i++) {
		float t = i/100.f;
		vec3 next = Bezier(&controlPoints[0], t);
		Line(prev, next, 3, vec3(0, 0, 0));
		prev = next;
	}

	// draw second cubic curve using points 3-6
	prev = Bezier(&controlPoints[3], 0);
	for (int i = 1; i <= 100; i++) {
		float t = i/100.f;
		vec3 next = Bezier(&controlPoints[3], t);
		Line(prev, next, 3, vec3(0, 0, 0));
		prev = next;
	}

	// animate point moving along curve
	float elapsed = (float) glfwGetTime() - startTime;
	float t = elapsed/duration;
	float alpha = (float) (sin(2*3.1415f*t - 3.1415f/2)+1)/2;

	// animate across both connected curves
	if (alpha < .5f)
		Disk(Bezier(&controlPoints[0], alpha*2), 14, vec3(1, 0, 0));
	else
		Disk(Bezier(&controlPoints[3], (alpha-.5f)*2), 14, vec3(1, 0, 0));

	glFlush();
}

// keep middle point between handles for C1 continuity
void EnforceContinuity() {
	controlPoints[3] = (controlPoints[2]+controlPoints[4])/2;
}

// move selected point or rotate camera
void MouseMove(float x, float y, bool leftDown, bool rightDown) {

	// move selected control point
	if (selectedPoint) {
		vec3 p1, p2;
		ScreenLine(x, y, camera.fullview, p1, p2);
		*(vec3 *) selectedPoint = p1;
		EnforceContinuity();
	}

	// otherwise rotate camera
	else if (leftDown)
		camera.Drag(x, y);
}

// select and move control points
void MouseButton(float x, float y, bool left, bool down) {
	if (left && down) {

		// try selecting a control point
		selectedPoint = NULL;

		for (int i = 0; i < 7; i++) {
			if (MouseOver(x, y, controlPoints[i], camera.fullview, 12)) {
				selectedPoint = &controlPoints[i];
				break;
			}
		}

		// rotate camera if no point selected
		if (!selectedPoint)
			camera.Down(x, y, Shift(), Control());
	}

	if (!down) {
		selectedPoint = NULL;
		camera.Up();
	}
}

int main() {
	// updated window
	GLFWwindow *w = InitGLFW(100, 100, winWidth, winHeight, "6-Assn-BumpyMesh");
	// load texture image into GPU
	RegisterMouseMove(MouseMove);
	RegisterMouseButton(MouseButton);
	RegisterResize(Resize);
	// event loop
	while (!glfwWindowShouldClose(w)) {
		Display();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
	glfwDestroyWindow(w);
	glfwTerminate();
}
