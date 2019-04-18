/*
	This is where GLUT/GLEW is initialized and the Windows section of the project connects to the core
*/
#include "WindowsApp.h"

#include <iostream>
#include <sstream>

/*
	The current directory for this file is [...] RSM/projects/Windows/RSM
*/

// 1600 x 900
// 800 x 450

#define WND_W 1600
#define WND_H 900

using namespace rsm;

void rsm::init(int argc, char* argv[]) {
	glApp = new OpenGLApplication(WND_W, WND_H);

	width = WND_W;
	height = WND_H;
	title = "Mobile Reflective Shadow Maps";

	// Setup glut
	glutInit(&argc, argv);
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutInitWindowSize(width, height);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
	windowHandle = glutCreateWindow(title.c_str());

	if (windowHandle < 1) {
		std::cerr << "ERROR: Could not create a rendering window." << std::endl;
		exit(EXIT_FAILURE);
	}

	// Setup glew
	glewExperimental = GL_TRUE;
	GLenum result = glewInit();
	if (result != GLEW_OK) {
		std::cerr << "ERROR glewInit: " << glewGetString(result) << std::endl;
		exit(EXIT_FAILURE);
	}
	GLenum err_code = glGetError();

	// Print system info
	const GLubyte *renderer = glGetString(GL_RENDERER);
	const GLubyte *vendor = glGetString(GL_VENDOR);
	const GLubyte *version = glGetString(GL_VERSION);
	const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
	std::cerr << "OpenGL Renderer: " << renderer << " (" << vendor << ")" << std::endl;
	std::cerr << "OpenGL version " << version << std::endl;
	std::cerr << "GLSL version " << glslVersion << std::endl;

	/* ===================================================================================
				Loading and Setup
	=====================================================================================*/
	glApp->init();
	RM.init();

	/* ===================================================================================
				Textures
	=====================================================================================*/
	/**/

	/**/
	Image cube_diff;
	cube_diff.loadFromFile("../../../assets/textures/cube/diffuse.png", IMG_2D);
	Image cube_spec;
	cube_diff.loadFromFile("../../../assets/textures/cube/glossiness.png", IMG_2D);
	sref<Texture> cube_diffTex = make_sref<Texture>(cube_diff);
	sref<Texture> cube_specTex = make_sref<Texture>(cube_spec);
	/**/


	/**/

	/* ===================================================================================
				Shaders and Materials
	=====================================================================================*/
	/**/

	ShaderSource vShader = ShaderSource(VERTEX_SHADER, "../../../src/Shaders/BlinnPhong.vs");
	ShaderSource fShader = ShaderSource(FRAGMENT_SHADER, "../../../src/Shaders/BlinnPhong.fs");
	vShader.inject("#version 330 core\n");
	fShader.inject("#version 330 core\n");
	vShader.compile();
	fShader.compile();

	sref<Shader> program = make_sref<Shader>("MainProgram");
	program->addShader(vShader);
	program->addShader(fShader);

	program->link();

	RM.addShader("MainProgram", program);

	glApp->addProgram(program->id());

	/**/
	sref<BlinnPhongMaterial> bp_test = make_sref<BlinnPhongMaterial>();
	/** /
	bp_test->setDiffuse(glm::vec3(1.0f, 0.5f, 0.2f));
	bp_test->setSpecular(glm::vec3(1.0f));
	/**/
	bp_test->setDiffuseTex(cube_diffTex->id());
	bp_test->setSpecularTex(cube_specTex->id());
	bp_test->setShininess(32.0f);

	bp_test->setProgram(program->id());
	/**/

	/**/
	/* ===================================================================================
				Meshes and Models
	=====================================================================================*/
	/**/

	/**/
	sref<Mesh> cube_mesh = make_sref<Mesh>("../../../assets/models/cube.obj");
	sref<Model> test_cube = make_sref<Model>(cube_mesh);
	test_cube->setMaterial(bp_test);
	RM.addMesh("cube_mesh", cube_mesh);
	RM.addModel("test_cube", test_cube);
	/**/

	/** /
	sref<Mesh> bunny_mesh = make_sref<Mesh>("../../../assets/models/bunny.obj");
	sref<Model> bunny = make_sref<Model>(bunny_mesh);
	bunny->setMaterial(bp_test);
	RM.addMesh("bunny_mesh", bunny_mesh);
	RM.addModel("bunny", bunny);
	/**/

	/** /
	sref<Mesh> sponza_mesh = make_sref<Mesh>("../../../assets/models/sponza.obj");
	sref<Model> sponza = make_sref<Model>(sponza_mesh);
	sponza->setMaterial(bp_test);
	RM.addMesh("sponza_mesh", sponza_mesh);
	RM.addModel("sponza", sponza);
	/**/

	/**/

	checkOpenGLError("Error during loading and setup!");

	glApp->prepare();
}

void rsm::update() {
	int timeSinceStart = glutGet(GLUT_ELAPSED_TIME);
	int deltaTime = timeSinceStart - oldTimeSinceStart;
	oldTimeSinceStart = timeSinceStart;

	float dt = (float)deltaTime / 1000.0f;
	// Limit the delta time to avoid large intervals
	if (dt > 0.25f)
		dt = 0.25f;

	glApp->update(dt);
}

void rsm::updateFPS(int value) {
	std::ostringstream oss;
	oss << title << ": " << frameCount << " FPS @ (" << width << "x" << height << ")";
	std::string s = oss.str();
	glutSetWindow(windowHandle);
	glutSetWindowTitle(s.c_str());
	frameCount = 0;
}

void rsm::setTitle(const std::string& new_title) {
	title = new_title;
}

void rsm::render() {
	update();
	glApp->render();
	frameCount++;

	glutSwapBuffers();
}

void rsm::reshape(int w, int h) {
	glApp->reshape(w, h);
}

void rsm::idle() {
	glutPostRedisplay();
}

void rsm::setupCallbacks() {
	glutDisplayFunc(render);
	glutReshapeFunc(reshape);
	glutIdleFunc(idle);
	glutCloseFunc(cleanup);
	glutTimerFunc(1000, updateFPS, 0);
}

void rsm::loop() {
	glutMainLoop();
}

void rsm::cleanup() {
	glApp->cleanup();
	delete glApp;
}

int main(int argc, char* argv[]) {
	rsm::init(argc, argv);
	rsm::setupCallbacks();

	rsm::loop();

	exit(EXIT_SUCCESS);
}