#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

using namespace std;

const unsigned SCR_WIDTH = 800;
const unsigned SCR_HEIGHT = 600;

void processInput(GLFWwindow *pWwindow);

int main(){
    // INICIALIZATION

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Project Grafika", NULL, NULL);
    if(window == NULL){
        cout << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }

    //SHADERS AND BUFFERS TOMORROW

    // RENDER LOOP
    while(!glfwWindowShouldClose(window)){
        //INPUT
        processInput(window);

        //RENDER
        glClearColor(0.2f, 0.8f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        //UPDATE
        //AFTER SHADERS AND BUFFERS
    }

    //TODO: DE-ALLOCATION AND FREEING UP RESCOURSES

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *pWwindow) {

}
