#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
unsigned int loadTexture(char const * path);
unsigned int loadCubemap(vector<std::string> faces);
void renderQuad();

// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 1050;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = false;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool dayNnite = true; // day is true

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct DirLight {
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 shipPosition = glm::vec3(0.0f);
    float shipScale = 1.0f;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;
void DrawImGui(ProgramState *programState);
void lightIt(Shader shader, PointLight pointLight1, PointLight pointLight2, PointLight pointLight3, PointLight pointLight4, PointLight pointLight5, PointLight pointLight6);
void lightDirLight(Shader shader, DirLight dirLightDay, DirLight dirLightNight);
void lightPointNormal(Shader shader, PointLight pointLight1, PointLight pointLight2);
int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // Face culling
    glCullFace(GL_FRONT);


    // build and compile shaders
    // -------------------------
    Shader lightingShader("resources/shaders/lighting.vs", "resources/shaders/lighting.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader blendingShader("resources/shaders/blending.vs", "resources/shaders/blending.fs");
    Shader lightCubeShader("resources/shaders/lightCube.vs", "resources/shaders/lightCube.fs");
    Shader normalMappingShader("resources/shaders/normal_mapping.vs", "resources/shaders/normal_mapping.fs");

    // load models
    // -----------
    stbi_set_flip_vertically_on_load(false);
    Model pirateShip("resources/objects/pirateship/pirateship.obj");
    pirateShip.SetShaderTextureNamePrefix("material.");

    Model pirate("resources/objects/pirate/14051_Pirate_Captain_v1_L1.obj");
    pirate.SetShaderTextureNamePrefix("material.");

    Model pirate2("resources/objects/pirate2/14053_Pirate_Shipmate_Old_v1_L1.obj");
    pirate2.SetShaderTextureNamePrefix("material.");

    Model cannon("resources/objects/cannon/14054_Pirate_Ship_Cannon_on_Cart_v1_l3.obj");
    cannon.SetShaderTextureNamePrefix("material.");

    Model island("resources/objects/island/island/island.obj");
    island.SetShaderTextureNamePrefix("material.");

    Model treasure("resources/objects/treasurechest/10803_TreasureChest_v2_L3.obj");
    treasure.SetShaderTextureNamePrefix("material.");

    Model lamp("resources/objects/oillamp/lantern_obj.obj");
    lamp.SetShaderTextureNamePrefix("material.");

    Model nightlamp("resources/objects/oillampnight/lantern_obj.obj");
    nightlamp.SetShaderTextureNamePrefix("material.");

    Model table("resources/objects/table/Old wooden table.obj");
    table.SetShaderTextureNamePrefix("material.");

    Model zajecarac("resources/objects/zajecarac/Beer_Bottle.obj");
    zajecarac.SetShaderTextureNamePrefix("material.");

    Model chair("resources/objects/chair/Simple_Wooden_Chair.obj");
    chair.SetShaderTextureNamePrefix("material.");

    Model campfire("resources/objects/campfire/Campfire.obj");
    campfire.SetShaderTextureNamePrefix("material.");

    DirLight dirLightDay;
    dirLightDay.direction = glm::vec3(0.7f, -0.5f, -0.5f);
    dirLightDay.ambient = glm::vec3(0.4f, 0.4f, 0.4f);
    dirLightDay.diffuse = glm::vec3(0.7f, 0.7f, 0.7f);
    dirLightDay.specular = glm::vec3(0.8f, 0.8f, 0.8f);

    DirLight dirLightNight;
    dirLightNight.direction = glm::vec3(-0.3f, -0.9f, -0.25f);
    dirLightNight.ambient = glm::vec3(0.05f, 0.05f, 0.05f);
    dirLightNight.diffuse = glm::vec3(0.1f, 0.1f, 0.1f);
    dirLightNight.specular = glm::vec3(0.3f, 0.3f, 0.3f);

    // Point Lights
    PointLight pointLight1;
    pointLight1.position = glm::vec3(1.4f, 4.8f, -8.0f);
    pointLight1.ambient = glm::vec3(0.3, 0.06, 0.0);
    pointLight1.diffuse = glm::vec3(1.0, 0.72, 0.11);
    pointLight1.specular = glm::vec3(1.0, 0.72, 0.11);
    pointLight1.constant = 1;
    pointLight1.linear = 0.09f;
    pointLight1.quadratic = 0.1f;

    PointLight pointLight2;
    pointLight2.position = glm::vec3(-1.4f, 4.8f, -8.0f);
    pointLight2.ambient = glm::vec3(0.3, 0.06, 0.0);
    pointLight2.diffuse = glm::vec3(1.0, 0.72, 0.11);
    pointLight2.specular = glm::vec3(1.0, 0.72, 0.11);
    pointLight2.constant = 1;
    pointLight2.linear = 0.09f;
    pointLight2.quadratic = 0.1f;

    PointLight pointLight3;
    pointLight3.position = glm::vec3(-3.22f, 6.6f, -12.9f);
    pointLight3.ambient = glm::vec3(0.3, 0.06, 0.0);
    pointLight3.diffuse = glm::vec3(1.0, 0.72, 0.11);
    pointLight3.specular = glm::vec3(1.0, 0.72, 0.11);
    pointLight3.constant = 1;
    pointLight3.linear = 0.09f;
    pointLight3.quadratic = 0.5f;

    PointLight pointLight4;
    pointLight4.position = glm::vec3(3.22f, 6.6f, -12.9f);
    pointLight4.ambient = glm::vec3( 0.3, 0.06, 0.0);
    pointLight4.diffuse = glm::vec3(1.0, 0.72, 0.11);
    pointLight4.specular = glm::vec3(1.0, 0.72, 0.11);
    pointLight4.constant = 1;
    pointLight4.linear = 0.09f;
    pointLight4.quadratic = 0.5f;

    PointLight pointLight5;
    pointLight5.position = glm::vec3(-37.5f, 23.0f, -100.0f);
    pointLight5.ambient = glm::vec3(0.3, 0.06, 0.0);
    pointLight5.diffuse = glm::vec3(1.0, 0.72, 0.11);
    pointLight5.specular = glm::vec3(1.0, 0.72, 0.11);
    pointLight5.constant = 1;
    pointLight5.linear = 0.09f;
    pointLight5.quadratic = 0.1f;

    PointLight pointLight6;
    pointLight6.position = glm::vec3(-14.6f, 2.5f, -53.5f);
    pointLight6.ambient = glm::vec3(0.3, 0.06, 0.0);
    pointLight6.diffuse = glm::vec3(1.0, 0.72, 0.11);
    pointLight6.specular = glm::vec3(1.0, 0.72, 0.11);
    pointLight6.constant = 1;
    pointLight6.linear = 0.09f;
    pointLight6.quadratic = 0.01f;


    float skullFlag[] = {
                    // positions            //normals             // texture Coords
            1.0f, -0.0f,  1.0f, 0.0f, 0.0f, 0.0f,  1.0f, 0.0f,
            -1.0f, -0.0f,  1.0f,  0.0f, 0.0f, 0.0f,0.0f, 0.0f,
            -1.0f, -0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,

            1.0f, -0.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            -1.0f, -0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            1.0f, -0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f
    };

    float water[] = {
            // positions            //normals             // texture Coords
            1.0f, -0.0f,  1.0f, 0.0f, 0.0f, 0.0f,  1.0f, 0.0f,
            -1.0f, -0.0f,  1.0f,  0.0f, 0.0f, 0.0f,0.0f, 0.0f,
            -1.0f, -0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,

            1.0f, -0.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            -1.0f, -0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            1.0f, -0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f
    };

    float grass[] = {
            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };

    float cube[] = {
            -0.5f, -0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f,  0.5f, -0.5f,
            0.5f,  0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,

            -0.5f, -0.5f,  0.5f,
            0.5f, -0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,

            -0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,

            0.5f,  0.5f,  0.5f,
            0.5f,  0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f, -0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,

            -0.5f, -0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f, -0.5f,  0.5f,
            0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f, -0.5f,

            -0.5f,  0.5f, -0.5f,
            0.5f,  0.5f, -0.5f,
            0.5f,  0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f, -0.5f
    };


    // SkyBox
    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };



    // plane VAO
    unsigned int planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(skullFlag), &skullFlag, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    glBufferData(GL_ARRAY_BUFFER, sizeof(water), &water, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    // grass VAO
    unsigned int grassVAO, grassVBO;
    glGenVertexArrays(1, &grassVAO);
    glGenBuffers(1, &grassVBO);
    glBindVertexArray(grassVAO);
    glBindBuffer(GL_ARRAY_BUFFER, grassVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(grass), &grass, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    // cube VAO
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube), &cube, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);


    // SkyBox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);



    // loading textures
    unsigned int flagTexture = loadTexture(FileSystem::getPath("resources/textures/pirateskull.png").c_str());
    unsigned int waterTexture = loadTexture(FileSystem::getPath("resources/textures/water.jpg").c_str());
    unsigned int grassTexture = loadTexture(FileSystem::getPath("resources/textures/grass.png").c_str());
    unsigned int woodDiffTexture = loadTexture(FileSystem::getPath("resources/textures/wood2/diff.jpg").c_str());
    unsigned int woodNormTexture = loadTexture(FileSystem::getPath("resources/textures/wood2/normal.jpg").c_str());

    // SkyBox textures and shader configuration
    vector<std::string> day
            {
                    FileSystem::getPath("resources/textures/skybox/skyboxday/right.jpg"),
                    FileSystem::getPath("resources/textures/skybox/skyboxday/left.jpg"),
                    FileSystem::getPath("resources/textures/skybox/skyboxday/top.jpg"),
                    FileSystem::getPath("resources/textures/skybox/skyboxday/bottom.jpg"),
                    FileSystem::getPath("resources/textures/skybox/skyboxday/front.jpg"),
                    FileSystem::getPath("resources/textures/skybox/skyboxday/back.jpg")
            };
    unsigned int cubemapTextureDay = loadCubemap(day);

    vector<std::string> night
            {
                    FileSystem::getPath("resources/textures/skybox/skyboxnight/right.jpg"),
                    FileSystem::getPath("resources/textures/skybox/skyboxnight/left.jpg"),
                    FileSystem::getPath("resources/textures/skybox/skyboxnight/top.jpg"),
                    FileSystem::getPath("resources/textures/skybox/skyboxnight/bottom.jpg"),
                    FileSystem::getPath("resources/textures/skybox/skyboxnight/front.jpg"),
                    FileSystem::getPath("resources/textures/skybox/skyboxnight/back.jpg")
            };
    unsigned int cubemapTextureNight = loadCubemap(night);

    vector<glm::vec3> vegetation {
        glm::vec3(34.0f, 9.0f, -58.5f),
        glm::vec3(-23.5f, 3.8f, -57.8f),
        glm::vec3(-3.8f, 12.2f, -83.3f),
        glm::vec3(-32.0f, 25.0f, -119.3f),
        glm::vec3(-63.0f, 16.3f, -105.3f)
    };

    // shader configuration
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    lightingShader.use();
    lightingShader.setInt("material.texture_diffuse1", 0);
//    lightingShader.setInt("material.texture_specular1", 1);

    blendingShader.use();
    blendingShader.setInt("texture1", 0);

    normalMappingShader.use();
    normalMappingShader.setInt("diffuseMap", 0);
    normalMappingShader.setInt("normalMap", 1);


    // lighting info
    // -------------
    glm::vec3 lightPos(0.5f, 1.0f, 0.3f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // don't forget to enable shader before setting uniforms
        //pointLight.position = glm::vec3(4.0 * cos(currentFrame), 4.0f, 4.0 * sin(currentFrame));
//        lightingShader.setVec3("pointLight.position", pointLight.position);
//        lightingShader.setVec3("pointLight.ambient", pointLight.ambient);
//        lightingShader.setVec3("pointLight.diffuse", pointLight.diffuse);
//        lightingShader.setVec3("pointLight.specular", pointLight.specular);
//        lightingShader.setFloat("pointLight.constant", pointLight.constant);
//        lightingShader.setFloat("pointLight.linear", pointLight.linear);
//        lightingShader.setFloat("pointLight.quadratic", pointLight.quadratic);
//        lightingShader.setVec3("viewPosition", programState->camera.Position);
//        lightingShader.setFloat("material.shininess", 32.0f);
//
//        lightingShader.setVec3("pointLight2.position", pointLight2.position);
//        lightingShader.setVec3("pointLight2.ambient", pointLight2.ambient);
//        lightingShader.setVec3("pointLight2.diffuse", pointLight2.diffuse);
//        lightingShader.setVec3("pointLight2.specular", pointLight2.specular);
//        lightingShader.setFloat("pointLight2.constant", pointLight2.constant);
//        lightingShader.setFloat("pointLight2.linear", pointLight2.linear);
//        lightingShader.setFloat("pointLight2.quadratic", pointLight2.quadratic);
//        lightingShader.setVec3("viewPosition", programState->camera.Position);
//        lightingShader.setFloat("material.shininess", 32.0f);



        lightingShader.use();
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 3000.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        normalMappingShader.use();
        // view/projection transformations
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 3000.0f);
        view = programState->camera.GetViewMatrix();
        normalMappingShader.setMat4("projection", projection);
        normalMappingShader.setMat4("view", view);

        normalMappingShader.use();
        // render normal-mapped quad
        glm::mat4 model = glm::mat4(1.0f);
        //model = glm::rotate(model, glm::radians((float)glfwGetTime() * -10.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0))); // rotate the quad to show normal mapping from multiple directions
        model = glm::translate(model, glm::vec3(0.0f, 5.0f, -8.3f));
        model = glm::translate(model, programState->shipPosition);
        model = glm::scale(model, glm::vec3(0.8f, 1.2f, 1.2f));
        normalMappingShader.setMat4("model", model);
        normalMappingShader.setVec3("viewPos", programState->camera.Position);
        normalMappingShader.setVec3("lightPos1", pointLight1.position);
        normalMappingShader.setVec3("lightPos2", pointLight2.position);

        lightPointNormal(normalMappingShader, pointLight1, pointLight2);
        lightDirLight(normalMappingShader, dirLightDay, dirLightNight);


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, woodDiffTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, woodNormTexture);
        renderQuad();


        lightingShader.use();
        // render the loaded model pirateship
        model = glm::mat4(1.0f); //inicijalizacija
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.2f, 1.2f, 1.2f));
        lightingShader.setMat4("model", model);
        pirateShip.Draw(lightingShader);

        // render pirate
        model = glm::mat4(1.0f); //inicijalizacija
        model = glm:: translate(model, glm::vec3(0.5f, 6.72f, -10.6f));
        model = glm::rotate(model, glm::radians(270.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.017f, 0.017f, 0.017f));
        lightingShader.setMat4("model", model);
        pirate.Draw(lightingShader);

        // render pirate2
        model = glm::mat4(1.0f); //inicijalizacija
        model = glm:: translate(model, glm::vec3(3.2f, 3.81f, -3.6f));
        model = glm::rotate(model, glm::radians(270.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(0.04f, 0.04f, 0.04f));
        lightingShader.setMat4("model", model);
        pirate2.Draw(lightingShader);

        //cannon
        model = glm::mat4(1.0f); //inicijalizacija
        model = glm:: translate(model, glm::vec3(1.9f, 3.81f, 2.7f));
        model = glm::rotate(model, glm::radians(270.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(-28.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(0.022f, 0.022f, 0.022f));
        lightingShader.setMat4("model", model);
        cannon.Draw(lightingShader);
        model = glm::mat4(1.0f); //inicijalizacija
        model = glm:: translate(model, glm::vec3(-1.9f, 3.81f, 2.7f));
        model = glm::rotate(model, glm::radians(270.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(-150.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(0.022f, 0.022f, 0.022f));
        lightingShader.setMat4("model", model);
        cannon.Draw(lightingShader);

        // render island
        model = glm::mat4(1.0f); //inicijalizacija
        model = glm:: translate(model, glm::vec3(-28.0f, 0.0f, -111.0f));
        model = glm::scale(model, glm::vec3(0.6f, 0.6f, 0.6f));
        lightingShader.setMat4("model", model);
        island.Draw(lightingShader);

        // render treasure
        model = glm::mat4(1.0f); //inicijalizacija
        model = glm:: translate(model, glm::vec3(-3.22f, 6.6f, -12.9f));
        model = glm::rotate(model, glm::radians(270.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(0.017f, 0.017f, 0.017f));
        lightingShader.setMat4("model", model);
        treasure.Draw(lightingShader);
        model = glm::mat4(1.0f); //inicijalizacija
        model = glm:: translate(model, glm::vec3(3.22f, 6.6f, -12.9f));
        model = glm::rotate(model, glm::radians(270.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(0.017f, 0.017f, 0.017f));
        lightingShader.setMat4("model", model);
        treasure.Draw(lightingShader);

        // render lamp
        if(dayNnite) {
            model = glm::mat4(1.0f); //inicijalizacija
            model = glm::translate(model, glm::vec3(1.4f, 4.6f, -8.0f));
            model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
            lightingShader.setMat4("model", model);
            lamp.Draw(lightingShader);
            model = glm::mat4(1.0f); //inicijalizacija
            model = glm::translate(model, glm::vec3(-1.4f, 4.6f, -8.0f));
            model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
            lightingShader.setMat4("model", model);
            lamp.Draw(lightingShader);
            model = glm::mat4(1.0f); //inicijalizacija
            model = glm::translate(model, glm::vec3(-37.5f, 22.65f, -100.0f));
            model = glm::scale(model, glm::vec3(0.017f, 0.017f, 0.017f));
            lightingShader.setMat4("model", model);
            lamp.Draw(lightingShader);
        }
        else {
            model = glm::mat4(1.0f); //inicijalizacija
            model = glm::translate(model, glm::vec3(1.4f, 4.6f, -8.0f));
            model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
            lightingShader.setMat4("model", model);
            nightlamp.Draw(lightingShader);
            model = glm::mat4(1.0f); //inicijalizacija
            model = glm::translate(model, glm::vec3(-1.4f, 4.6f, -8.0f));
            model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
            lightingShader.setMat4("model", model);
            nightlamp.Draw(lightingShader);
            model = glm::mat4(1.0f); //inicijalizacija
            model = glm::translate(model, glm::vec3(-37.5f, 22.65f, -100.0f));
            model = glm::scale(model, glm::vec3(0.017f, 0.017f, 0.017f));
            lightingShader.setMat4("model", model);
            nightlamp.Draw(lightingShader);
        }
        // table
        model = glm::mat4(1.0f); //inicijalizacija

        model = glm:: translate(model, glm::vec3(-37.0f, 20.0f, -100.0f));
        model = glm::rotate(model, glm::radians(-15.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f, 0.3f, 0.4f));
        lightingShader.setMat4("model", model);
        table.Draw(lightingShader);

        // beer
        model = glm::mat4(1.0f); //inicijalizacija
        model = glm:: translate(model, glm::vec3(-35.0f, 22.6f, -99.5f));
        model = glm::rotate(model, glm::radians(120.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
        lightingShader.setMat4("model", model);
        zajecarac.Draw(lightingShader);
        model = glm::mat4(1.0f); //inicijalizacija
        model = glm:: translate(model, glm::vec3(-38.5f, 22.6f, -101.1f));
        model = glm::rotate(model, glm::radians(170.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
        lightingShader.setMat4("model", model);
        zajecarac.Draw(lightingShader);

        // chair
        model = glm::mat4(1.0f); //inicijalizacija
        model = glm:: translate(model, glm::vec3(-38.0f, 20.0f, -104.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(4.0f, 4.0f, 4.0f));
        lightingShader.setMat4("model", model);
        chair.Draw(lightingShader);
        model = glm::mat4(1.0f); //inicijalizacija
        model = glm:: translate(model, glm::vec3(-33.5f, 20.0f, -98.0f));
        model = glm::rotate(model, glm::radians(165.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(4.0f, 4.0f, 4.0f));
        lightingShader.setMat4("model", model);
        chair.Draw(lightingShader);

        // campfire
        model = glm::mat4(1.0f); //inicijalizacija
        model = glm:: translate(model, glm::vec3(-14.6f, 1.8f, -53.5f));
        //model = glm::rotate(model, glm::radians(165.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
        lightingShader.setMat4("model", model);
        campfire.Draw(lightingShader);

        // flag
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 4.2f, -15.2f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));
        glBindVertexArray(planeVAO);
        glBindTexture(GL_TEXTURE_2D, flagTexture);
        lightingShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glEnable(GL_CULL_FACE);
        // water
        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(20000.0f, 0.0f, 20000.0f));
        glBindVertexArray(planeVAO);
        glBindTexture(GL_TEXTURE_2D, waterTexture);
        lightingShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisable(GL_CULL_FACE);

        // grass
        blendingShader.use();
        blendingShader.setMat4("view", view);
        blendingShader.setMat4("projection", projection);
        for(int i = 0; i < vegetation.size(); i++){
            model = glm::mat4(1.0f);
            model = glm::translate(model, vegetation[i]);
            model = glm::scale(model, glm::vec3(3.0f, 3.0f, 3.0f));
            glBindVertexArray(grassVAO);
            glBindTexture(GL_TEXTURE_2D, grassTexture);
            blendingShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // firecube
        lightCubeShader.use();
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);
        glBindVertexArray(cubeVAO);
        model = glm::mat4(1.0);
        model = glm::translate(model, programState->shipPosition);
        model = glm::translate(model, glm::vec3(-14.6f, 2.5f, -53.5f));
        model = glm::scale(model,glm::vec3(programState->shipScale));
        lightCubeShader.setMat4("model", model);
        lightCubeShader.setVec3("lightColor", glm::vec3(0.99f, 0.31f, 0.0f)); //254,80,0
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // lighting


        lightIt(lightingShader, pointLight1, pointLight2, pointLight3, pointLight4, pointLight5, pointLight6);
        lightIt(blendingShader, pointLight1, pointLight2, pointLight3, pointLight4, pointLight5, pointLight6);

        lightDirLight(lightingShader, dirLightDay, dirLightNight);
        lightDirLight(blendingShader, dirLightDay, dirLightNight);

        // draw skybox as last
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        if(dayNnite)
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTextureDay);
        else
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTextureNight);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default
        glDepthMask(GL_TRUE);



        if (programState->ImGuiEnabled)
            DrawImGui(programState);



        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteVertexArrays(1, &grassVAO);
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &planeVBO);
    glDeleteBuffers(1, &skyboxVBO);
    glDeleteBuffers(1, &grassVBO);
    glDeleteBuffers(1, &cubeVBO);
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Backpack position", (float*)&programState->shipPosition);
        ImGui::DragFloat("Backpack scale", &programState->shipScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    if (key == GLFW_KEY_L && action == GLFW_PRESS)
        dayNnite = !dayNnite;
}
unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void lightDirLight(Shader shader, DirLight dirLightDay, DirLight dirLightNight){
    shader.use();
    if(dayNnite){

        // Dir Light
        shader.setVec3("dirLight.direction", dirLightDay.direction);
        shader.setVec3("dirLight.ambient", dirLightDay.ambient);
        shader.setVec3("dirLight.diffuse", dirLightDay.diffuse);
        shader.setVec3("dirLight.specular", dirLightDay.specular);
    }
    else{
        // Dir Light
        shader.setVec3("dirLight.direction", dirLightNight.direction);
        shader.setVec3("dirLight.ambient", dirLightNight.ambient);
        shader.setVec3("dirLight.diffuse", dirLightNight.diffuse);
        shader.setVec3("dirLight.specular", dirLightNight.specular);
    }
}

void lightPointNormal(Shader shader, PointLight pointLight1, PointLight pointLight2){
    shader.use();

    if(dayNnite){
        shader.setVec3("pointLight1.position", pointLight1.position);
        shader.setVec3("pointLight1.ambient", glm::vec3(0.0f, 0.0f, 0.0f));
        shader.setVec3("pointLight1.diffuse", glm::vec3(0.0f, 0.0f, 0.0f));
        shader.setVec3("pointLight1.specular", glm::vec3(0.0f, 0.0f, 0.0f));
        shader.setFloat("pointLight1.constant", pointLight1.constant);
        shader.setFloat("pointLight1.linear", pointLight1.linear);
        shader.setFloat("pointLight1.quadratic", pointLight1.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);

        shader.setVec3("pointLight2.position", pointLight2.position);
        shader.setVec3("pointLight2.ambient", glm::vec3(0.0f, 0.0f, 0.0f));
        shader.setVec3("pointLight2.diffuse", glm::vec3(0.0f, 0.0f, 0.0f));
        shader.setVec3("pointLight2.specular", glm::vec3(0.0f, 0.0f, 0.0f));
        shader.setFloat("pointLight2.constant", pointLight2.constant);
        shader.setFloat("pointLight2.linear", pointLight2.linear);
        shader.setFloat("pointLight2.quadratic", pointLight2.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);
    }
    else{
        shader.setVec3("pointLight1.position", pointLight1.position);
        shader.setVec3("pointLight1.ambient", pointLight1.ambient);
        shader.setVec3("pointLight1.diffuse", pointLight1.diffuse);
        shader.setVec3("pointLight1.specular", pointLight1.specular);
        shader.setFloat("pointLight1.constant", pointLight1.constant);
        shader.setFloat("pointLight1.linear", pointLight1.linear);
        shader.setFloat("pointLight1.quadratic", pointLight1.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);

        shader.setVec3("pointLight2.position", pointLight2.position);
        shader.setVec3("pointLight2.ambient", pointLight2.ambient);
        shader.setVec3("pointLight2.diffuse", pointLight2.diffuse);
        shader.setVec3("pointLight2.specular", pointLight2.specular);
        shader.setFloat("pointLight2.constant", pointLight2.constant);
        shader.setFloat("pointLight2.linear", pointLight2.linear);
        shader.setFloat("pointLight2.quadratic", pointLight2.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);
    }

}

void lightIt(Shader shader, PointLight pointLight1, PointLight pointLight2, PointLight pointLight3, PointLight pointLight4, PointLight pointLight5, PointLight pointLight6) {
    //treasure lighting
    shader.use();
    shader.setVec3("pointLight3.position", pointLight3.position);
    shader.setVec3("pointLight3.ambient", pointLight3.ambient);
    shader.setVec3("pointLight3.diffuse", pointLight3.diffuse);
    shader.setVec3("pointLight3.specular", pointLight3.specular);
    shader.setFloat("pointLight3.constant", pointLight3.constant);
    shader.setFloat("pointLight3.linear", pointLight3.linear);
    shader.setFloat("pointLight3.quadratic", pointLight3.quadratic);
    shader.setVec3("viewPosition", programState->camera.Position);
    shader.setFloat("material.shininess", 32.0f);

    shader.setVec3("pointLight4.position", pointLight4.position);
    shader.setVec3("pointLight4.ambient", pointLight4.ambient);
    shader.setVec3("pointLight4.diffuse", pointLight4.diffuse);
    shader.setVec3("pointLight4.specular", pointLight4.specular);
    shader.setFloat("pointLight4.constant", pointLight4.constant);
    shader.setFloat("pointLight4.linear", pointLight4.linear);
    shader.setFloat("pointLight4.quadratic", pointLight4.quadratic);
    shader.setVec3("viewPosition", programState->camera.Position);
    shader.setFloat("material.shininess", 32.0f);


    if(dayNnite) {
        // Point light

        shader.setVec3("pointLight1.position", pointLight1.position);
        shader.setVec3("pointLight1.ambient", glm::vec3(0.0f, 0.0f, 0.0f));
        shader.setVec3("pointLight1.diffuse", glm::vec3(0.0f, 0.0f, 0.0f));
        shader.setVec3("pointLight1.specular", glm::vec3(0.0f, 0.0f, 0.0f));
        shader.setFloat("pointLight1.constant", pointLight1.constant);
        shader.setFloat("pointLight1.linear", pointLight1.linear);
        shader.setFloat("pointLight1.quadratic", pointLight1.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);

        shader.setVec3("pointLight2.position", pointLight2.position);
        shader.setVec3("pointLight2.ambient", glm::vec3(0.0f, 0.0f, 0.0f));
        shader.setVec3("pointLight2.diffuse", glm::vec3(0.0f, 0.0f, 0.0f));
        shader.setVec3("pointLight2.specular", glm::vec3(0.0f, 0.0f, 0.0f));
        shader.setFloat("pointLight2.constant", pointLight2.constant);
        shader.setFloat("pointLight2.linear", pointLight2.linear);
        shader.setFloat("pointLight2.quadratic", pointLight2.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);

        shader.setVec3("pointLight5.position", pointLight5.position);
        shader.setVec3("pointLight5.ambient", glm::vec3(0.0f, 0.0f, 0.0f));
        shader.setVec3("pointLight5.diffuse", glm::vec3(0.0f, 0.0f, 0.0f));
        shader.setVec3("pointLight5.specular", glm::vec3(0.0f, 0.0f, 0.0f));
        shader.setFloat("pointLight5.constant", pointLight5.constant);
        shader.setFloat("pointLight5.linear", pointLight5.linear);
        shader.setFloat("pointLight5.quadratic", pointLight5.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);

        shader.setVec3("pointLight6.position", pointLight6.position);
        shader.setVec3("pointLight6.ambient", pointLight6.ambient);
        shader.setVec3("pointLight6.diffuse", pointLight6.diffuse);
        shader.setVec3("pointLight6.specular", pointLight6.specular);
        shader.setFloat("pointLight6.constant", pointLight6.constant);
        shader.setFloat("pointLight6.linear", pointLight6.linear);
        shader.setFloat("pointLight6.quadratic", pointLight6.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);

    }
    else {
        // Point light 255,184,28
        shader.setVec3("pointLight1.position", pointLight1.position);
        shader.setVec3("pointLight1.ambient", pointLight1.ambient);
        shader.setVec3("pointLight1.diffuse", pointLight1.diffuse);
        shader.setVec3("pointLight1.specular", pointLight1.specular);
        shader.setFloat("pointLight1.constant", pointLight1.constant);
        shader.setFloat("pointLight1.linear", pointLight1.linear);
        shader.setFloat("pointLight1.quadratic", pointLight1.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);

        shader.setVec3("pointLight2.position", pointLight2.position);
        shader.setVec3("pointLight2.ambient", pointLight2.ambient);
        shader.setVec3("pointLight2.diffuse", pointLight2.diffuse);
        shader.setVec3("pointLight2.specular", pointLight2.specular);
        shader.setFloat("pointLight2.constant", pointLight2.constant);
        shader.setFloat("pointLight2.linear", pointLight2.linear);
        shader.setFloat("pointLight2.quadratic", pointLight2.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);

        shader.setVec3("pointLight5.position", pointLight5.position);
        shader.setVec3("pointLight5.ambient", pointLight5.ambient);
        shader.setVec3("pointLight5.diffuse", pointLight5.diffuse);
        shader.setVec3("pointLight5.specular", pointLight5.specular);
        shader.setFloat("pointLight5.constant", pointLight5.constant);
        shader.setFloat("pointLight5.linear", pointLight5.linear);
        shader.setFloat("pointLight5.quadratic", pointLight5.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);

        shader.setVec3("pointLight6.position", pointLight6.position);
        shader.setVec3("pointLight6.ambient", pointLight6.ambient);
        shader.setVec3("pointLight6.diffuse", pointLight6.diffuse);
        shader.setVec3("pointLight6.specular", pointLight6.specular);
        shader.setFloat("pointLight6.constant", pointLight6.constant);
        shader.setFloat("pointLight6.linear", pointLight6.linear);
        shader.setFloat("pointLight6.quadratic", pointLight6.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);
    }
}

// renders a 1x1 quad in NDC with manually calculated tangent vectors
// ------------------------------------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        // positions
        glm::vec3 pos1(-1.0f,  1.0f, 0.0f);
        glm::vec3 pos2(-1.0f, -1.0f, 0.0f);
        glm::vec3 pos3( 1.0f, -1.0f, 0.0f);
        glm::vec3 pos4( 1.0f,  1.0f, 0.0f);
        // texture coordinates
        glm::vec2 uv1(0.0f, 1.0f);
        glm::vec2 uv2(0.0f, 0.0f);
        glm::vec2 uv3(1.0f, 0.0f);
        glm::vec2 uv4(1.0f, 1.0f);
        // normal vector
        glm::vec3 nm(0.0f, 0.0f, 1.0f);

        // calculate tangent/bitangent vectors of both triangles
        glm::vec3 tangent1, bitangent1;
        glm::vec3 tangent2, bitangent2;
        // triangle 1
        // ----------
        glm::vec3 edge1 = pos2 - pos1;
        glm::vec3 edge2 = pos3 - pos1;
        glm::vec2 deltaUV1 = uv2 - uv1;
        glm::vec2 deltaUV2 = uv3 - uv1;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

        // triangle 2
        // ----------
        edge1 = pos3 - pos1;
        edge2 = pos4 - pos1;
        deltaUV1 = uv3 - uv1;
        deltaUV2 = uv4 - uv1;

        f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);


        bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);


        float quadVertices[] = {
                // positions            // normal         // texcoords  // tangent                          // bitangent
                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
                pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,

                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
                pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
        };
        // configure plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
    }


    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}