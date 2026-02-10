#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Util.h"

// Infrastruktura (ostavljeno za buduci rad)
bool firstMouse = true;
float lastX, lastY;
float yaw = -90.0f, pitch = 0.0f;
glm::vec3 cameraFront = glm::vec3(0.0, 0.0, -1.0);

// Funkcija za ucitavanje i podesavanje teksture
unsigned int setupSignatureTexture(const char* filepath) {
    unsigned int tex = loadImageToTexture(filepath);
    if (tex == 0) return 0;
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

int main(void) {
    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // ZADATAK 1: FULLSCREEN
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    int wWidth = mode->width;
    int wHeight = mode->height;

    GLFWwindow* window = glfwCreateWindow(wWidth, wHeight, "Nikola Lakic RA 67/2021", primaryMonitor, NULL);

    if (window == NULL) {
        glfwTerminate();
        return 2;
    }

    glfwMakeContextCurrent(window);
    glewInit();
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Omogucavanje blending-a za providnost
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    unsigned int unifiedShader = createShader("basic.vert", "basic.frag");

    // ZADATAK 3: UCITAVANJE POTPISA
    unsigned int signatureTex = setupSignatureTexture("res/signature.png");

    // PODEŠAVANJE POLUPROVIDNOSTI
    float alphaVal = 0.4f; // 40% vidljivosti

    // PODACI ZA POTPIS (Pravougaonik prilagodjen dugackom tekstu)
    float sigVertices[] = {
        // Pozicija (X, Y, Z)     Boja (RGBA)                     UV (S, T)      Normala
        0.40f, -0.95f, 0.0f,      1.0f, 1.0f, 1.0f, alphaVal,     0.0f, 0.0f,    0,0,1,
        0.95f, -0.95f, 0.0f,      1.0f, 1.0f, 1.0f, alphaVal,     1.0f, 0.0f,    0,0,1,
        0.95f, -0.85f, 0.0f,      1.0f, 1.0f, 1.0f, alphaVal,     1.0f, 1.0f,    0,0,1,
        0.40f, -0.85f, 0.0f,      1.0f, 1.0f, 1.0f, alphaVal,     0.0f, 1.0f,    0,0,1
    };

    unsigned int sigVAO, sigVBO;
    glGenVertexArrays(1, &sigVAO);
    glGenBuffers(1, &sigVBO);
    glBindVertexArray(sigVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sigVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sigVertices), sigVertices, GL_STATIC_DRAW);

    unsigned int stride = 12 * sizeof(float);
    glEnableVertexAttribArray(0); // Pozicija
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1); // Boja (channelCol u sejderu)
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2); // UV (channelTex u sejderu)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(7 * sizeof(float)));

    // Uniforme
    unsigned int modelLoc = glGetUniformLocation(unifiedShader, "uM");
    unsigned int viewLoc = glGetUniformLocation(unifiedShader, "uV");
    unsigned int projectionLoc = glGetUniformLocation(unifiedShader, "uP");
    unsigned int useTexLoc = glGetUniformLocation(unifiedShader, "useTex");
    unsigned int transLoc = glGetUniformLocation(unifiedShader, "transparent");

    // ZADATAK 2: 75 FPS LIMITER
    double targetFrameTime = 1.0 / 75.0;

    while (!glfwWindowShouldClose(window)) {
        double frameStartTime = glfwGetTime();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(unifiedShader);

        // Resetovanje matrica za 2D prikaz
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

        // Slanje uniformi za teksturu i providnost
        glUniform1i(useTexLoc, 1);
        glUniform1i(transLoc, 1); // OBAVEZNO: Aktivira providnost u tvom sejderu

        // CRTANJE POTPISA
        glDisable(GL_DEPTH_TEST);
        glBindTexture(GL_TEXTURE_2D, signatureTex);
        glBindVertexArray(sigVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GL_TRUE);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // ZADATAK 2: 75 FPS LIMITER
        while (glfwGetTime() - frameStartTime < targetFrameTime) {
            // Cekaj dok ne prodje 1/75 sekunde
        }
    }

    glDeleteVertexArrays(1, &sigVAO);
    glDeleteBuffers(1, &sigVBO);
    glDeleteProgram(unifiedShader);
    glfwTerminate();
    return 0;
}