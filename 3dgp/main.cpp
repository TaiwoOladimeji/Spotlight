#include <iostream>
#include <GL/glew.h>
#include <3dgl/3dgl.h>
#include <GL/glut.h>
#include <GL/freeglut_ext.h>

// Include GLM core features
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#pragma comment (lib, "glew32.lib")

using namespace std;
using namespace _3dgl;
using namespace glm;

// 3D models
C3dglModel table;
C3dglModel vase;
C3dglModel chicken;
C3dglModel lamp;
C3dglModel room;
unsigned nPyramidBuf = 0;

// texture ids
GLuint idTexWood;
GLuint idTexCloth;
GLuint idTexNone;

// GLSL Program
C3dglProgram program;

// The View Matrix
mat4 matrixView;

// Camera & navigation
float maxspeed = 4.f;	// camera max speed
float accel = 4.f;		// camera acceleration
vec3 _acc(0), _vel(0);	// camera acceleration and velocity vectors
float _fov = 60.f;		// field of view (zoom)

// light switches
float fAmbient = 1, fDir = 1, fPoint1 = 1, fPoint2 = 1;

bool init()
{
	// rendering states
	glEnable(GL_DEPTH_TEST);	// depth test is necessary for most 3D scenes
	glEnable(GL_NORMALIZE);		// normalization is needed by AssImp library models
	glShadeModel(GL_SMOOTH);	// smooth shading mode is the default one; try GL_FLAT here!
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	// this is the default one; try GL_LINE!

	// Initialise Shaders
	C3dglShader vertexShader;
	C3dglShader fragmentShader;

	if (!vertexShader.create(GL_VERTEX_SHADER)) return false;
	if (!vertexShader.loadFromFile("shaders/basic.vert")) return false;
	if (!vertexShader.compile()) return false;

	if (!fragmentShader.create(GL_FRAGMENT_SHADER)) return false;
	if (!fragmentShader.loadFromFile("shaders/basic.frag")) return false;
	if (!fragmentShader.compile()) return false;

	if (!program.create()) return false;
	if (!program.attach(vertexShader)) return false;
	if (!program.attach(fragmentShader)) return false;
	if (!program.link()) return false;
	if (!program.use(true)) return false;

	// glut additional setup
	glutSetVertexAttribCoord3(program.getAttribLocation("aVertex"));
	glutSetVertexAttribNormal(program.getAttribLocation("aNormal"));

	// load your 3D models here!
	if (!table.load("models\\table.obj")) return false;
	if (!vase.load("models\\vase.obj")) return false;
	if (!chicken.load("models\\chicken.obj")) return false;
	if (!lamp.load("models\\lamp.obj")) return false;
	if (!room.load("models\\LivingRoomObj\\LivingRoom.obj")) return false;
	room.loadMaterials("models\\LivingRoomObj\\");


	// create pyramid
	float vermals[] = {
	  -4, 0,-4, 0, 4,-7, 4, 0,-4, 0, 4,-7, 0, 7, 0, 0, 4,-7, 
	  -4, 0, 4, 0, 4, 7, 4, 0, 4, 0, 4, 7, 0, 7, 0, 0, 4, 7, 
	  -4, 0,-4,-7, 4, 0,-4, 0, 4,-7, 4, 0, 0, 7, 0,-7, 4, 0,
	   4, 0,-4, 7, 4, 0, 4, 0, 4, 7, 4, 0, 0, 7, 0, 7, 4, 0, 
	  -4, 0,-4, 0,-1, 0,-4, 0, 4, 0,-1, 0, 4, 0,-4, 0,-1, 0, 
	   4, 0, 4, 0,-1, 0,-4, 0, 4, 0,-1, 0, 4, 0,-4, 0,-1, 0 };

	// Generate 1 buffer name
	glGenBuffers(1, &nPyramidBuf);
	// Bind (activate) the buffer
	glBindBuffer(GL_ARRAY_BUFFER, nPyramidBuf);
	// Send data to the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(vermals), vermals, GL_STATIC_DRAW);

	// Setup Lights - note that diffuse and specular values are moved to onRender
	program.sendUniform("lightEmissive.color", vec3(0));
	program.sendUniform("lightDir.direction", vec3(1.0, 0.5, 1.0));
	program.sendUniform("lightPoint1.position", vec3(-2.95, 4.24, -1.0));
	program.sendUniform("lightPoint2.position", vec3(1.05, 4.24, 1.0));

	// Setup materials
	program.sendUniform("materialAmbient", vec3(1.0, 1.0, 1.0));
	program.sendUniform("materialDiffuse", vec3(1.0, 1.0, 1.0));
	program.sendUniform("materialSpecular", vec3(0.0, 0.0, 0.0));
	program.sendUniform("shininess", 10.0);

	// create & load textures
	C3dglBitmap bm;
	glActiveTexture(GL_TEXTURE0);
	
	// cloth texture
	bm.load("models/cloth.png", GL_RGBA);
	if (!bm.getBits()) return false;
	glGenTextures(1, &idTexCloth);
	glBindTexture(GL_TEXTURE_2D, idTexCloth);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), bm.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.getBits());

	// wood texture
	bm.load("models/oak.png", GL_RGBA);
	if (!bm.getBits()) return false;
	glGenTextures(1, &idTexWood);
	glBindTexture(GL_TEXTURE_2D, idTexWood);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), bm.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.getBits());

	// none (simple-white) texture
	glGenTextures(1, &idTexNone);
	glBindTexture(GL_TEXTURE_2D, idTexNone);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	BYTE bytes[] = { 255, 255, 255 };
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_BGR, GL_UNSIGNED_BYTE, &bytes);

	// Send the texture info to the shaders
	program.sendUniform("texture0", 0);

	// Initialise the View Matrix (initial position of the camera)
	matrixView = rotate(mat4(1), radians(12.f), vec3(1, 0, 0));
	matrixView *= lookAt(
		vec3(1.0, 5.0, 6.0),
		vec3(1.0, 5.0, 0.0),
		vec3(0.0, 1.0, 0.0));

	// setup the screen background colour
	glClearColor(0.18f, 0.25f, 0.22f, 1.0f);   // deep grey background

	cout << endl;
	cout << "Use:" << endl;
	cout << "  WASD or arrow key to navigate" << endl;
	cout << "  QE or PgUp/Dn to move the camera up and down" << endl;
	cout << "  Shift to speed up your movement" << endl;
	cout << "  Drag the mouse to look around" << endl;
	cout << endl;
	cout << "  1 to switch the lamp #1 on/off" << endl;
	cout << "  2 to switch the lamp #2 on/off" << endl;
	cout << "  9 to switch directional light on/off" << endl;
	cout << "  0 to switch ambient light on/off" << endl;
	cout << endl;
	
	return true;
}

void renderScene(mat4& matrixView, float time, float deltaTime)
{
	mat4 m;

	// setup lights
	program.sendUniform("lightAmbient.color", fAmbient * vec3(0.05, 0.05, 0.05));
	program.sendUniform("lightDir.diffuse", fDir * vec3(0.3, 0.3, 0.3));	  // dimmed white light
	program.sendUniform("lightPoint1.diffuse", fPoint1 * vec3(0.5, 0.5, 0.5));
	program.sendUniform("lightPoint1.specular", fPoint1 * vec3(1.0, 1.0, 1.0));
	program.sendUniform("lightPoint2.diffuse", fPoint2 * vec3(0.5, 0.5, 0.5));
	program.sendUniform("lightPoint2.specular", fPoint2 * vec3(1.0, 1.0, 1.0));

	// setup materials
	program.sendUniform("materialSpecular", vec3(0.0, 0.0, 0.0));

	// room
	program.sendUniform("materialDiffuse", vec3(1.0, 1.0, 1.0));
	program.sendUniform("materialAmbient", vec3(1.0, 1.0, 1.0));
	m = matrixView;
	m = scale(m, vec3(0.03f, 0.03f, 0.03f));
	room.render(m);

	// table & chairs
	program.sendUniform("materialDiffuse", vec3(1.0, 1.0, 1.0));
	program.sendUniform("materialAmbient", vec3(1.0, 1.0, 1.0));
	glBindTexture(GL_TEXTURE_2D, idTexWood);
	m = matrixView;
	m = scale(m, vec3(0.004f, 0.004f, 0.004f));
	table.render(1, m);
	glBindTexture(GL_TEXTURE_2D, idTexCloth);
	table.render(0, m);
	m = rotate(m, radians(180.f), vec3(0.f, 1.f, 0.f));
	table.render(0, m);
	m = translate(m, vec3(250, 0, 0));
	m = rotate(m, radians(90.f), vec3(0.f, 1.f, 0.f));
	table.render(0, m);
	m = translate(m, vec3(0, 0, -500));
	m = rotate(m, radians(180.f), vec3(0.f, 1.f, 0.f));
	table.render(0, m);

	// vase
	program.sendUniform("materialDiffuse", vec3(0.2, 0.4, 0.8));
	program.sendUniform("materialAmbient", vec3(0.2, 0.4, 0.8));
	program.sendUniform("materialSpecular", vec3(1.0, 1.0, 1.0));
	glBindTexture(GL_TEXTURE_2D, idTexNone);
	m = matrixView;
	m = translate(m, vec3(0.f, 3.f, 0.f));
	m = scale(m, vec3(0.12f, 0.12f, 0.12f));
	vase.render(m);

	// teapot
	program.sendUniform("materialDiffuse", vec3(0.1, 0.8, 0.3));
	program.sendUniform("materialAmbient", vec3(0.1, 0.8, 0.3));
	program.sendUniform("materialSpecular", vec3(1.0, 1.0, 1.0));
	m = matrixView;
	m = translate(m, vec3(1.8f, 3.4f, 0.0f));
	program.sendUniform("matrixModelView", m);
	glutSolidTeapot(0.5);

	// pyramid
	program.sendUniform("materialDiffuse", vec3(1.0, 0.2, 0.2));
	program.sendUniform("materialAmbient", vec3(1.0, 0.2, 0.2));
	program.sendUniform("materialSpecular", vec3(0.0, 0.0, 0.0));
	m = matrixView;
	m = translate(m, vec3(-1.5f, 3.7f, 0.5f));
	m = rotate(m, radians(180.f), vec3(1, 0, 0));
	m = rotate(m, radians(-40 * time), vec3(0, 1, 0));
	m = scale(m, vec3(0.1f, 0.1f, 0.1f));
	program.sendUniform("matrixModelView", m);

	GLuint attribVertex = program.getAttribLocation("aVertex");
	GLuint attribNormal = program.getAttribLocation("aNormal");
	glBindBuffer(GL_ARRAY_BUFFER, nPyramidBuf);
	glEnableVertexAttribArray(attribVertex);
	glEnableVertexAttribArray(attribNormal);
	glVertexAttribPointer(attribVertex, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
	glVertexAttribPointer(attribNormal, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glDrawArrays(GL_TRIANGLES, 0, 18);
	glDisableVertexAttribArray(GL_VERTEX_ARRAY);
	glDisableVertexAttribArray(GL_NORMAL_ARRAY);

	// chicken
	program.sendUniform("materialDiffuse", vec3(0.8, 0.8, 0.2));
	program.sendUniform("materialAmbient", vec3(0.8, 0.8, 0.2));
	program.sendUniform("materialSpecular", vec3(0.6, 0.6, 1.0));
	m = translate(m, vec3(0, -5, 0));
	m = scale(m, vec3(0.2f, 0.2f, 0.2f));
	m = rotate(m, radians(180.f), vec3(1, 0, 0));
	chicken.render(m);

	// lamp 1
	m = matrixView;
	m = translate(m, vec3(-2.2f, 3.075f, -1.0f));
	m = scale(m, vec3(0.02f, 0.02f, 0.02f));
	lamp.render(m);

	// lamp 2
	m = matrixView;
	m = translate(m, vec3(1.8f, 3.075f, 1.0f));
	m = scale(m, vec3(0.02f, 0.02f, 0.02f));
	lamp.render(m);

	// light bulb 1
	program.sendUniform("materialDiffuse", vec3(0.8, 0.8, 0.8));
	program.sendUniform("materialAmbient", vec3(1.0, 1.0, 1.0));
	program.sendUniform("lightEmissive.color", vec3(fPoint1));
	m = matrixView;
	m = translate(m, vec3(-2.95f, 4.24f, -1.0f));
	m = scale(m, vec3(0.1f, 0.1f, 0.1f));
	program.sendUniform("matrixModelView", m);
	glutSolidSphere(1, 32, 32);

	// light bulb 2
	program.sendUniform("materialDiffuse", vec3(0.8, 0.8, 0.8));
	program.sendUniform("materialAmbient", vec3(1.0, 1.0, 1.0));
	program.sendUniform("lightEmissive.color", vec3(fPoint2));
	m = matrixView;
	m = translate(m, vec3(1.05f, 4.24f, 1.0f));
	m = scale(m, vec3(0.1f, 0.1f, 0.1f));
	program.sendUniform("matrixModelView", m);
	glutSolidSphere(1, 32, 32);
	program.sendUniform("lightEmissive.color", vec3(0));
}

void onRender()
{
	// these variables control time & animation
	static float prev = 0;
	float time = glutGet(GLUT_ELAPSED_TIME) * 0.001f;	// time since start in seconds
	float deltaTime = time - prev;						// time since last frame
	prev = time;										// framerate is 1/deltaTime

	// clear screen and buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// setup the View Matrix (camera)
	_vel = clamp(_vel + _acc * deltaTime, -vec3(maxspeed), vec3(maxspeed));
	float pitch = getPitch(matrixView);
	matrixView = rotate(translate(rotate(mat4(1),
		pitch, vec3(1, 0, 0)),	// switch the pitch off
		_vel * deltaTime),		// animate camera motion (controlled by WASD keys)
		-pitch, vec3(1, 0, 0))	// switch the pitch on
		* matrixView;

	// setup View Matrix
	program.sendUniform("matrixView", matrixView);

	// render the scene objects
	renderScene(matrixView, time, deltaTime);

	// essential for double-buffering technique
	glutSwapBuffers();

	// proceed the animation
	glutPostRedisplay();
}

// called before window opened or resized - to setup the Projection Matrix
void onReshape(int w, int h)
{
	float ratio = w * 1.0f / h;      // we hope that h is not zero
	glViewport(0, 0, w, h);
	program.sendUniform("matrixProjection", perspective(radians(_fov), ratio, 0.02f, 1000.f));
}

// Handle WASDQE keys
void onKeyDown(unsigned char key, int x, int y)
{
	switch (tolower(key))
	{
	case 'w': _acc.z = accel; break;
	case 's': _acc.z = -accel; break;
	case 'a': _acc.x = accel; break;
	case 'd': _acc.x = -accel; break;
	case 'e': _acc.y = accel; break;
	case 'q': _acc.y = -accel; break;
	case '1': fPoint1 = 1 - fPoint1; break;
	case '2': fPoint2 = 1 - fPoint2; break;
	case '9': fDir = 1 - fDir; break;
	case '0': fAmbient = 1 - fAmbient; break;
	}
}

// Handle WASDQE keys (key up)
void onKeyUp(unsigned char key, int x, int y)
{
	switch (tolower(key))
	{
	case 'w':
	case 's': _acc.z = _vel.z = 0; break;
	case 'a':
	case 'd': _acc.x = _vel.x = 0; break;
	case 'q':
	case 'e': _acc.y = _vel.y = 0; break;
	}
}

// Handle arrow keys and Alt+F4
void onSpecDown(int key, int x, int y)
{
	maxspeed = glutGetModifiers() & GLUT_ACTIVE_SHIFT ? 20.f : 4.f;
	switch (key)
	{
	case GLUT_KEY_F4:		if ((glutGetModifiers() & GLUT_ACTIVE_ALT) != 0) exit(0); break;
	case GLUT_KEY_UP:		onKeyDown('w', x, y); break;
	case GLUT_KEY_DOWN:		onKeyDown('s', x, y); break;
	case GLUT_KEY_LEFT:		onKeyDown('a', x, y); break;
	case GLUT_KEY_RIGHT:	onKeyDown('d', x, y); break;
	case GLUT_KEY_PAGE_UP:	onKeyDown('q', x, y); break;
	case GLUT_KEY_PAGE_DOWN:onKeyDown('e', x, y); break;
	case GLUT_KEY_F11:		glutFullScreenToggle();
	}
}

// Handle arrow keys (key up)
void onSpecUp(int key, int x, int y)
{
	maxspeed = glutGetModifiers() & GLUT_ACTIVE_SHIFT ? 20.f : 4.f;
	switch (key)
	{
	case GLUT_KEY_UP:		onKeyUp('w', x, y); break;
	case GLUT_KEY_DOWN:		onKeyUp('s', x, y); break;
	case GLUT_KEY_LEFT:		onKeyUp('a', x, y); break;
	case GLUT_KEY_RIGHT:	onKeyUp('d', x, y); break;
	case GLUT_KEY_PAGE_UP:	onKeyUp('q', x, y); break;
	case GLUT_KEY_PAGE_DOWN:onKeyUp('e', x, y); break;
	}
}

// Handle mouse click
void onMouse(int button, int state, int x, int y)
{
	glutSetCursor(state == GLUT_DOWN ? GLUT_CURSOR_CROSSHAIR : GLUT_CURSOR_INHERIT);
	glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);
	if (button == 1)
	{
		_fov = 60.0f;
		onReshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
	}
}

// handle mouse move
void onMotion(int x, int y)
{
	glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);

	// find delta (change to) pan & pitch
	float deltaYaw = 0.005f * (x - glutGet(GLUT_WINDOW_WIDTH) / 2);
	float deltaPitch = 0.005f * (y - glutGet(GLUT_WINDOW_HEIGHT) / 2);

	if (abs(deltaYaw) > 0.3f || abs(deltaPitch) > 0.3f)
		return;	// avoid warping side-effects

	// View = Pitch * DeltaPitch * DeltaYaw * Pitch^-1 * View;
	constexpr float maxPitch = radians(80.f);
	float pitch = getPitch(matrixView);
	float newPitch = glm::clamp(pitch + deltaPitch, -maxPitch, maxPitch);
	matrixView = rotate(rotate(rotate(mat4(1.f),
		newPitch, vec3(1.f, 0.f, 0.f)),
		deltaYaw, vec3(0.f, 1.f, 0.f)), 
		-pitch, vec3(1.f, 0.f, 0.f)) 
		* matrixView;
}

void onMouseWheel(int button, int dir, int x, int y)
{
	_fov = glm::clamp(_fov - dir * 5.f, 5.0f, 175.f);
	onReshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
}

int main(int argc, char **argv)
{
	// init GLUT and create Window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(1280, 720);
	glutCreateWindow("3DGL Scene: Textured Room");

	// init glew
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		C3dglLogger::log("GLEW Error {}", (const char*)glewGetErrorString(err));
		return 0;
	}
	C3dglLogger::log("Using GLEW {}", (const char*)glewGetString(GLEW_VERSION));

	// register callbacks
	glutDisplayFunc(onRender);
	glutReshapeFunc(onReshape);
	glutKeyboardFunc(onKeyDown);
	glutSpecialFunc(onSpecDown);
	glutKeyboardUpFunc(onKeyUp);
	glutSpecialUpFunc(onSpecUp);
	glutMouseFunc(onMouse);
	glutMotionFunc(onMotion);
	glutMouseWheelFunc(onMouseWheel);

	C3dglLogger::log("Vendor: {}", (const char *)glGetString(GL_VENDOR));
	C3dglLogger::log("Renderer: {}", (const char *)glGetString(GL_RENDERER));
	C3dglLogger::log("Version: {}", (const char*)glGetString(GL_VERSION));
	C3dglLogger::log("");

	// init light and everything – not a GLUT or callback function!
	if (!init())
	{
		C3dglLogger::log("Application failed to initialise\r\n");
		return 0;
	}

	// enter GLUT event processing cycle
	glutMainLoop();

	return 1;
}

