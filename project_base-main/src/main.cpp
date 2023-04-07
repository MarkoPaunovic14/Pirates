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

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

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
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
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

    // build and compile shaders
    // -------------------------
    Shader lightingShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");

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

    Model ocean("resources/objects/oillamp/lantern_obj.obj");
    lamp.SetShaderTextureNamePrefix("material.");


    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(1.4f, 4.8f, -8.0f);
    pointLight.ambient = glm::vec3(0.2, 0.2, 0.2);
    pointLight.diffuse = glm::vec3(0.8, 0.8, 0.8);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.1f;

    PointLight pointLight2;
    pointLight2.position = glm::vec3(-1.4f, 4.8f, -8.0f);
    pointLight2.ambient = glm::vec3(0.2, 0.2, 0.2);
    pointLight2.diffuse = glm::vec3(0.8, 0.8, 0.8);
    pointLight2.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight2.constant = 1.0f;
    pointLight2.linear = 0.09f;
    pointLight2.quadratic = 0.1f;




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
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);
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
        lightingShader.use();
        //pointLight.position = glm::vec3(4.0 * cos(currentFrame), 4.0f, 4.0 * sin(currentFrame));
        lightingShader.setVec3("pointLight.position", pointLight.position);
        lightingShader.setVec3("pointLight.ambient", pointLight.ambient);
        lightingShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        lightingShader.setVec3("pointLight.specular", pointLight.specular);
        lightingShader.setFloat("pointLight.constant", pointLight.constant);
        lightingShader.setFloat("pointLight.linear", pointLight.linear);
        lightingShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        lightingShader.setVec3("viewPosition", programState->camera.Position);
        lightingShader.setFloat("material.shininess", 32.0f);

        lightingShader.setVec3("pointLight2.position", pointLight2.position);
        lightingShader.setVec3("pointLight2.ambient", pointLight2.ambient);
        lightingShader.setVec3("pointLight2.diffuse", pointLight2.diffuse);
        lightingShader.setVec3("pointLight2.specular", pointLight2.specular);
        lightingShader.setFloat("pointLight2.constant", pointLight2.constant);
        lightingShader.setFloat("pointLight2.linear", pointLight2.linear);
        lightingShader.setFloat("pointLight2.quadratic", pointLight2.quadratic);
        lightingShader.setVec3("viewPosition", programState->camera.Position);
        lightingShader.setFloat("material.shininess", 32.0f);

        if(dayNnite) {
            // Dir Light
            lightingShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
            lightingShader.setVec3("dirLight.ambient", 0.4f, 0.4f, 0.4f);
            lightingShader.setVec3("dirLight.diffuse", 0.7f, 0.7f, 0.7f);
            lightingShader.setVec3("dirLight.specular", 0.8f, 0.8f, 0.8f);
        }
        else {
            // Dir Light
            lightingShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
            lightingShader.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
            lightingShader.setVec3("dirLight.diffuse", 0.1f, 0.1f, 0.1f);
            lightingShader.setVec3("dirLight.specular", 0.3f, 0.3f, 0.3f);
        }

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 3000.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        // render the loaded model pirateship
        glm::mat4 model = glm::mat4(1.0f); //inicijalizacija
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
        model = glm:: translate(model, glm::vec3(-3.2f, 3.81f, -3.6f));
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
        model = glm:: translate(model, glm::vec3(-28.0f, 0.0f, -83.0f));
        model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));
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
        model = glm::mat4(1.0f); //inicijalizacija
        model = glm:: translate(model, glm::vec3(1.4f, 4.6f, -8.0f));
        model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
        lightingShader.setMat4("model", model);
        lamp.Draw(lightingShader);
        model = glm::mat4(1.0f); //inicijalizacija
        model = glm:: translate(model, glm::vec3(-1.4f, 4.6f, -8.0f));
        model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
        lightingShader.setMat4("model", model);
        lamp.Draw(lightingShader);

        // flag
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 4.2f, -15.2f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));
        glBindVertexArray(planeVAO);
        glBindTexture(GL_TEXTURE_2D, flagTexture);
        lightingShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // water
        model = glm::mat4(1.0f);
        model = glm::translate(model,programState->shipPosition); // translate it down so it's at the center of the scene
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(20000.0f, 20000.0f, 20000.0f));
        model = glm::scale(model, glm::vec3(programState->shipScale));    // it's a bit too big for our scene, so scale it down
        glBindVertexArray(planeVAO);
        glBindTexture(GL_TEXTURE_2D, waterTexture);
        lightingShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 6);

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
