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

// Animation timing
float startTime = (float) glfwGetTime();
float duration = 3;

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

MeshS body, prop;

// Flight Path
vec3 path[] = {
	{2/3.f,0,2/3.f},    {1,0,1/3.f},    {1,.1f,-1/3.f},
	{2/3.f,.1f,-2/3.f}, {1/3.f,.1f,-1}, {-1/3.f,.4f,-1},
	{-2/3.f,.4f,-2/3.f}, {-1,.4f,-1/3.f}, {-1,0,1/3.f},
	{-2/3.f,0,2/3.f},   {-1/3.f,0,1},   {1/3.f,0,1},
	{2/3.f,0,2/3.f}
};

class Bezier {
public:
	vec3 *pts = NULL; // *pts is beginning of 4 points defining the curve

	Bezier(vec3 *pts) : pts(pts) { }

	vec3 Position(float t) {
		float s = 1-t;
		return s*s*s*pts[0]+3*s*s*t*pts[1]+3*s*t*t*pts[2]+t*t*t*pts[3];
	}

	vec3 Velocity(float t) {
		float t2 = t*t;
		return (-3*t2+6*t-3)*pts[0]+(9*t2-12*t+3)*pts[1]+(6*t-9*t2)*pts[2]+3*t2*pts[3];
	}

	mat4 Frame(float t) {
		vec3 p = Position(t);
		vec3 v = normalize(Velocity(t));
		vec3 n = normalize(cross(v, vec3(0, 1, 0)));
		vec3 b = normalize(cross(n, v));

		mat4 frame {
				{n[0], b[0], -v[0], p[0]},
				{n[1], b[1], -v[1], p[1]},
				{n[2], b[2], -v[2], p[2]},
				{0,    0,    0,     1}
		};

		return frame;
	}

	void Draw(int res = 50, float lineWidth = 3) {
		glLineWidth(lineWidth);
		for (int i = 0; i < res; i++) {
			float t1 = (float) i/res;
			float t2 = (float) (i+1)/res;
			Line(Position(t1), Position(t2), lineWidth, mag);
		}
	}
};

Bezier bezier[] = {
	Bezier(&path[0]),
	Bezier(&path[3]),
	Bezier(&path[6]),
	Bezier(&path[9])
};

int nBezier = sizeof(bezier)/sizeof(Bezier);

// Animation
void Animate() {
	float elapsed = (float) glfwGetTime()-startTime;
	float alpha = nBezier*elapsed/duration;

	float beta = fmod(alpha, (float) nBezier);
	float t = beta-floor(beta);
	int i = (int) floor(beta);

	mat4 f = bezier[i].Frame(t);

	body.toWorld = f*Scale(.35f)*RotateY(-90);
	prop.toWorld = body.toWorld*Translate(-.6f,0,0)*RotateY(-90)*Scale(.25f)*RotateZ(1500*elapsed);
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
	body.Render(vec3(.55f, .25f, .12f)); // brown airplane body
	prop.Render(vec3(0, .7f, 0));        // green propeller
	for (int i = 0; i < nBezier; i++)
		bezier[i].Draw();
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
	// read meshes
	body.Read("../Airplane-Body.obj");
	prop.Read("../Airplane-Propeller.obj");
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