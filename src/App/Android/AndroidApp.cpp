#include "App/OpenGLApplication.h"

#include <android/asset_manager_jni.h>

#include <time.h>
#include <sys/types.h>
#include <dirent.h>

using namespace rsm;

rsm::OpenGLApplication* glApp;
AAssetManager* assetManager;

long oldTimeSinceStart;

char* getAssetSource(const char* filepath);
size_t getAssetLength(const char* filepath);
long getCurrentTime();
bool endsWith (std::string const &fullString, std::string const &ending);
void loadTextures(std::string directory, std::string extension, std::string prefix);

/** ==================================================================================
            README™ - A Comprehensive Guide on how not to fuck up!
=====================================================================================*/

//FIXME: On Reshape/Screen orientation change (Landscape <-> Portrait) "init()" seems to be called everytime!

/**
 * IMPORTANT: SUPPOSEDLY reshape is always called at least once...
 * So I'm just using useless values: 1920x1080
 * UPDATE: So far it seems to be true!
 */

void init() {

    glApp = new OpenGLApplication(1920, 1080);

    LOG("Initializing AndroidApp...\n");

    /** ==================================================================================
				Loading and Setup
	=====================================================================================*/
    glApp->init();
    RM.init();

    checkOpenGLError("Error during engine initialization!");
    LOG("Init successful...\n");

    /** ==================================================================================
				Shaders
	=====================================================================================*/

    ShaderSource vBP = ShaderSource(VERTEX_SHADER, "vBP", getAssetSource("shaders/BlinnPhong.vs"));
    ShaderSource fBP = ShaderSource(FRAGMENT_SHADER, "fBP", getAssetSource("shaders/BlinnPhong.fs"));
    vBP.inject(std::string("#version 320 es\n") +
               std::string("#extension GL_EXT_shader_io_blocks : enable\n"));

    fBP.inject(std::string("#version 320 es\n") +
               std::string("#extension GL_EXT_shader_io_blocks : enable\n") +
               std::string("precision highp float;\n") +
               std::string("const int NUM_LIGHTS = ") + std::to_string(NUM_LIGHTS) + ";\n" +
               std::string("const int NUM_VPL = ") + std::to_string(NUM_VPL) + ";\n");
    vBP.compile();
    fBP.compile();

    ShaderSource vBPT = ShaderSource(VERTEX_SHADER, "vBPT", getAssetSource("shaders/BlinnPhongTex.vs"));
    ShaderSource fBPT = ShaderSource(FRAGMENT_SHADER, "fBPT", getAssetSource("shaders/BlinnPhongTex.fs"));
    vBPT.inject(std::string("#version 320 es\n") +
               std::string("#extension GL_EXT_shader_io_blocks : enable\n"));

    fBPT.inject(std::string("#version 320 es\n") +
               std::string("#extension GL_EXT_shader_io_blocks : enable\n") +
               std::string("precision highp float;\n") +
               std::string("const int NUM_LIGHTS = ") + std::to_string(NUM_LIGHTS) + ";\n" +
               std::string("const int NUM_VPL = ") + std::to_string(NUM_VPL) + ";\n");
    vBPT.compile();
    fBPT.compile();

    ShaderSource vDS = ShaderSource(VERTEX_SHADER, "vDS", getAssetSource("shaders/DeferredShading.vs"));
    ShaderSource fDS = ShaderSource(FRAGMENT_SHADER, "fDS", getAssetSource("shaders/DeferredShading.fs"));
    vDS.inject(std::string("#version 320 es\n") +
               std::string("#extension GL_EXT_shader_io_blocks : enable\n"));
    fDS.inject(std::string("#version 320 es\n") +
               std::string("#extension GL_EXT_shader_io_blocks : enable\n") +
               std::string("precision highp float;\n") +
               std::string("const int NUM_LIGHTS = ") + std::to_string(NUM_LIGHTS) + ";\n" +
               std::string("const int NUM_VPL = ") + std::to_string(NUM_VPL) + ";\n");
    vDS.compile();
    fDS.compile();

    ShaderSource vII = ShaderSource(VERTEX_SHADER, "vII", getAssetSource("shaders/IndirectIllumination.vs"));
    ShaderSource fII = ShaderSource(FRAGMENT_SHADER, "fII", getAssetSource("shaders/IndirectIllumination.fs"));
    vII.inject(std::string("#version 320 es\n") +
               std::string("#extension GL_EXT_shader_io_blocks : enable\n"));
    fII.inject(std::string("#version 320 es\n") +
               std::string("#extension GL_EXT_shader_io_blocks : enable\n") +
               std::string("precision highp float;\n") +
               std::string("const int NUM_LIGHTS = ") + std::to_string(NUM_LIGHTS) + ";\n" +
               std::string("const int NUM_VPL = ") + std::to_string(NUM_VPL) + ";\n");
    vII.compile();
    fII.compile();

    ShaderSource vGB = ShaderSource(VERTEX_SHADER, "vGB",getAssetSource("shaders/GBuffer.vs"));
    ShaderSource fGB = ShaderSource(FRAGMENT_SHADER, "fGB", getAssetSource("shaders/GBuffer.fs"));
    vGB.inject(std::string("#version 320 es\n") +
                std::string("#extension GL_EXT_shader_io_blocks : enable\n"));
    fGB.inject(std::string("#version 320 es\n") +
                std::string("#extension GL_EXT_shader_io_blocks : enable\n") +
                std::string("precision highp float;\n"));
    vGB.compile();
    fGB.compile();

    ShaderSource vRGB = ShaderSource(VERTEX_SHADER, "vRGB",getAssetSource("shaders/RSMGBuffer.vs"));
    ShaderSource fRGB = ShaderSource(FRAGMENT_SHADER, "fRGB", getAssetSource("shaders/RSMGBuffer.fs"));
    vRGB.inject(std::string("#version 320 es\n") +
               std::string("#extension GL_EXT_shader_io_blocks : enable\n"));
    fRGB.inject(std::string("#version 320 es\n") +
               std::string("#extension GL_EXT_shader_io_blocks : enable\n") +
               std::string("precision highp float;\n") +
               std::string("const int NUM_LIGHTS = ") + std::to_string(NUM_LIGHTS) + ";\n");
    vRGB.compile();
    fRGB.compile();

    sref<Shader> BlinnPhong = make_sref<Shader>("BlinnPhong");
    BlinnPhong->addShader(vBP);
    BlinnPhong->addShader(fBP);
    BlinnPhong->link();
    RM.addShader("BlinnPhong", BlinnPhong);
    glApp->addProgram(BlinnPhong->id());

    sref<Shader> BlinnPhongTex = make_sref<Shader>("BlinnPhongTex");
    BlinnPhongTex->addShader(vBPT);
    BlinnPhongTex->addShader(fBPT);
    BlinnPhongTex->link();
    RM.addShader("BlinnPhongTex", BlinnPhongTex);
    glApp->addProgram(BlinnPhongTex->id());

    sref<Shader> DeferredShading = make_sref<Shader>("DeferredShading");
    DeferredShading->addShader(vDS);
    DeferredShading->addShader(fDS);
    DeferredShading->link();
    RM.addShader("DeferredShading", DeferredShading);
    glApp->addProgram(DeferredShading->id());

    sref<Shader> IndirectIllumination = make_sref<Shader>("IndirectIllumination");
    IndirectIllumination->addShader(vII);
    IndirectIllumination->addShader(fII);
    IndirectIllumination->link();
    RM.addShader("IndirectIllumination", IndirectIllumination);
    glApp->addProgram(IndirectIllumination->id());

    sref<Shader> GBuffer = make_sref<Shader>("GBuffer");
    GBuffer->addShader(vGB);
    GBuffer->addShader(fGB);
    GBuffer->link();
    RM.addShader("GBuffer", GBuffer);

    sref<Shader> RSMGBuffer = make_sref<Shader>("RSMGBuffer");
    RSMGBuffer->addShader(vRGB);
    RSMGBuffer->addShader(fRGB);
    RSMGBuffer->link();
    RM.addShader("RSMGBuffer", RSMGBuffer);

    checkOpenGLError("Error during shader loading and setup!");
    LOG("Shaders loaded...\n");

    /** ==================================================================================
            Textures
    =====================================================================================*/

    /** /
    loadTextures("models/crytek sponza/textures", "png", "textures\\");
    /**/

    checkOpenGLError("Error during texture loading!");
    LOG("Textures loaded...\n");

    /** ==================================================================================
            Models
	=====================================================================================*/

    /**/
    sref<Model> Lucy = make_sref<Model>("Lucy");
    Lucy->loadFromMemory(getAssetSource("models/Lucy/Lucy.obj"), getAssetSource("models/Lucy/Lucy.mtl"));
    RM.addModel("Lucy", Lucy);
    /**/

    /** /
    sref<Model> demo2 = make_sref<Model>("demo2");
    demo2->loadFromMemory(getAssetSource("models/demo2/demo2.obj"), getAssetSource("models/demo2/demo2.mtl"));
    RM.addModel("demo2", demo2);
    /**/

    /** /
    sref<Model> demo_scene = make_sref<Model>("demo_scene");
    demo_scene->loadFromMemory(getAssetSource("models/demo scene closed/demo_scene.obj"), getAssetSource("models/demo scene closed/demo_scene.mtl"));
    RM.addModel("demo_scene", demo_scene);
    /**/

    /** /
    sref<Model> sponza = make_sref<Model>("sponza");
    sponza->loadFromMemory(getAssetSource("models/crytek sponza/sponza.obj"), getAssetSource("models/crytek sponza/sponza.mtl"));
    RM.addModel("sponza", sponza);
    /**/

    checkOpenGLError("Error during model loading!");
    LOG("Meshes and models loaded...\n");
    checkOpenGLError("Error during loading and setup!");
    LOG("AndroidApp successfully initialized!\n");
    glApp->prepare();
    LOG("AndroidApp successfully prepared!\n");
    oldTimeSinceStart = getCurrentTime();
}

void reshape(int w, int h) {
    glApp->reshape(w,h);
}

void render() {
    // get deltaTime, send it to update
    long timeSinceStart = getCurrentTime();
    long deltaTime = timeSinceStart - oldTimeSinceStart;
    oldTimeSinceStart = timeSinceStart;

    // nano to microseconds
    float dt = float(deltaTime) / 1000.0f;

    float secs = dt / 1000000.0f;
    LOG("FPS: %f\n", 1.0f / secs);

    if(dt > 0.25f)
        dt = 0.25f;

    //glApp->getCamera()->updateOrientation(0.0f, dt * 0.1f);
    //glApp->getCamera()->updateViewMatrix();
    glApp->update(dt);

    glApp->render();
}

char* getAssetSource(const char* filepath) {
    AAsset* file = AAssetManager_open(assetManager, filepath, AASSET_MODE_STREAMING);
    size_t filesize = AAsset_getLength(file);

    char* filesource = new char[filesize+1];
    AAsset_read(file, filesource, filesize);
    filesource[filesize] = '\0';

    return filesource;
}

size_t getAssetLength(const char* filepath){
    AAsset* file = AAssetManager_open(assetManager, filepath, AASSET_MODE_STREAMING);
    size_t filesize = AAsset_getLength(file);
    return filesize;
}

bool endsWith (std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

void loadTextures(std::string directory, std::string extension, std::string prefix){

    AAssetDir* dir = AAssetManager_openDir(assetManager, directory.c_str());
    const char* file;
    while((file = AAssetDir_getNextFileName(dir)) != NULL){
        std::string filename(file);
        if(endsWith(filename, extension)){
            std::string fullpath = directory + "/" + filename;
            Image img;
            //LOG("Filename: %s\n", filename.c_str());
            img.loadFromMemory(getAssetSource(fullpath.c_str()), getAssetLength(fullpath.c_str()), IMG_2D);
            sref<Texture> tex = make_sref<Texture>(img);
            RM.addTexture(prefix + filename, tex);
        }
    }
}

/*
 * Returns current time in nanoseconds (10^-9 sec)
 */
long getCurrentTime(){
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    // in nanoseconds
    return (now.tv_sec * 1000000000) + now.tv_nsec;
}

extern "C"
{
    JNIEXPORT void JNICALL Java_com_rbenjamim_rsm_AndroidApp_init(JNIEnv *env, jclass type, jobject aMgr);
    JNIEXPORT void JNICALL Java_com_rbenjamim_rsm_AndroidApp_resize(JNIEnv *env, jclass type, jint width, jint height);
    JNIEXPORT void JNICALL Java_com_rbenjamim_rsm_AndroidApp_render(JNIEnv *env, jclass type);
};

JNIEXPORT void JNICALL Java_com_rbenjamim_rsm_AndroidApp_init(JNIEnv* env, jclass type, jobject aMgr) {
    assetManager = AAssetManager_fromJava(env, aMgr);
    init();
}

JNIEXPORT void JNICALL Java_com_rbenjamim_rsm_AndroidApp_resize(JNIEnv* env, jclass type, jint width, jint height) {
    reshape(width, height);
}

JNIEXPORT void JNICALL Java_com_rbenjamim_rsm_AndroidApp_render(JNIEnv* env, jclass type) {
    render();
}

