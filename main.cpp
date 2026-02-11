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

// --- KAMERA I KRETANJE ---
glm::vec3 cameraPos = glm::vec3(0.0f, 3.0f, 10.0f); // Malo podignuta kamera
glm::vec3 cameraFront = glm::vec3(0.0f, -0.2f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 0, lastY = 0;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos; lastY = ypos;
    float sensitivity = 0.1f;
    yaw += xoffset * sensitivity;
    pitch += yoffset * sensitivity;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void processInput(GLFWwindow* window) {
    float cameraSpeed = 3.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
}

unsigned int setupTexture(const char* filepath) {
    unsigned int tex = loadImageToTexture(filepath);
    if (tex == 0) return 0;
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    return tex;
}

int main(void) {
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Bioskop - Nikola Lakic RA 67/2021", primaryMonitor, NULL);

    if (window == NULL) { glfwTerminate(); return 2; }
    glfwMakeContextCurrent(window);
    glewInit();
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int unifiedShader = createShader("basic.vert", "basic.frag");

    unsigned int signatureTex = setupTexture("res/signature.png");
    unsigned int seatTex = setupTexture("stolica.png");

    // Geometrija za Sedište i Platno (isti format)
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,   1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 0.0f,  0,0,1,
         0.5f, -0.5f, 0.0f,   1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 0.0f,  0,0,1,
         0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 1.0f,  0,0,1,
        -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 1.0f,  0,0,1
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
    glBindVertexArray(VAO); glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    unsigned int stride = 12 * sizeof(float);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(7 * sizeof(float)));

    // Potpis (2D)
    float sigVertices[] = {
        0.40f, -0.95f, 0.0f,  1.0f, 1.0f, 1.0f, 0.4f, 0.0f, 0.0f, 0,0,1,
        0.95f, -0.95f, 0.0f,  1.0f, 1.0f, 1.0f, 0.4f, 1.0f, 0.0f, 0,0,1,
        0.95f, -0.85f, 0.0f,  1.0f, 1.0f, 1.0f, 0.4f, 1.0f, 1.0f, 0,0,1,
        0.40f, -0.85f, 0.0f,  1.0f, 1.0f, 1.0f, 0.4f, 0.0f, 1.0f, 0,0,1
    };
    unsigned int sigVAO, sigVBO;
    glGenVertexArrays(1, &sigVAO); glGenBuffers(1, &sigVBO);
    glBindVertexArray(sigVAO); glBindBuffer(GL_ARRAY_BUFFER, sigVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sigVertices), sigVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(7 * sizeof(float)));

    unsigned int modelLoc = glGetUniformLocation(unifiedShader, "uM");
    unsigned int viewLoc = glGetUniformLocation(unifiedShader, "uV");
    unsigned int projLoc = glGetUniformLocation(unifiedShader, "uP");
    unsigned int useTexLoc = glGetUniformLocation(unifiedShader, "useTex");
    unsigned int transLoc = glGetUniformLocation(unifiedShader, "transparent");
    unsigned int ambLoc = glGetUniformLocation(unifiedShader, "uAmb");

    double targetFrameTime = 1.0 / 75.0;

    while (!glfwWindowShouldClose(window)) {
        double frameStartTime = glfwGetTime();
        deltaTime = (float)frameStartTime - lastFrame;
        lastFrame = (float)frameStartTime;
        processInput(window);

        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(unifiedShader);
        glUniform1i(transLoc, 1);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)mode->width / (float)mode->height, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        glBindVertexArray(VAO);

        // 1. PLATNO (Beli pravougaonik naspram stolica)
        glm::mat4 screenModel = glm::mat4(1.0f);
        // Postavljamo ga na Z=-12, a Y na 3.1 da mu dno bude na -0.4 (kao stolice)
        screenModel = glm::translate(screenModel, glm::vec3(0.0f, 3.1f, -12.0f));
        screenModel = glm::scale(screenModel, glm::vec3(12.0f, 7.0f, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(screenModel));
        glUniform1i(useTexLoc, 0); // Iskljuci teksturu za belo platno
        glUniform1f(ambLoc, 0.0f); // Platno ne treba ambijentalno svetlo
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // 2. SEDIŠTA (Mreža 5x6)
        glBindTexture(GL_TEXTURE_2D, seatTex);
        glUniform1i(useTexLoc, 1);
        glUniform1f(ambLoc, 0.15f); // OSVETLJAVAMO CRNE STOLICE (Ambijentalno)

        for (int row = 0; row < 5; row++) {
            for (int col = 0; col < 6; col++) {
                glm::mat4 model = glm::mat4(1.0f);
                float xPos = (col - 2.5f) * 1.5f;
                float yPos = row * 0.5f;
                float zPos = row * 1.5f; // Stolice idu ka kameri
                model = glm::translate(model, glm::vec3(xPos, yPos, zPos));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            }
        }

        // 3. POTPIS (2D)
        glDisable(GL_DEPTH_TEST);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glUniform1i(useTexLoc, 1);
        glUniform1f(ambLoc, 0.0f);
        glBindTexture(GL_TEXTURE_2D, signatureTex);
        glBindVertexArray(sigVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
        while (glfwGetTime() - frameStartTime < targetFrameTime) {}
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &sigVAO);
    glfwTerminate();
    return 0;
}