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

// --- ZADATAK 2: ENUM STANJA ---
enum CinemaState { START, ENTER, PROJECTION, EXIT };
CinemaState currentState = START;

// --- KAMERA I KRETANJE ---
glm::vec3 cameraPos = glm::vec3(0.0f, 4.0f, 15.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, -0.2f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

bool firstMouse = true;
float yaw = -90.0f, pitch = 0.0f;
float lastX = 0, lastY = 0;
float deltaTime = 0.0f, lastFrame = 0.0f;

// Callback za promenu stanja na tastere
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_1) { currentState = START; std::cout << "Stanje: POCETNO\n"; }
        if (key == GLFW_KEY_2) { currentState = ENTER; std::cout << "Stanje: LJUDI ULAZE\n"; }
        if (key == GLFW_KEY_3) { currentState = PROJECTION; std::cout << "Stanje: PROJEKCIJA\n"; }
        if (key == GLFW_KEY_4) { currentState = EXIT; std::cout << "Stanje: LJUDI IZLAZE\n"; }
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) { lastX = (float)xpos; lastY = (float)ypos; firstMouse = false; }
    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX = (float)xpos; lastY = (float)ypos;
    float sensitivity = 0.1f;
    yaw += xoffset * sensitivity; pitch += yoffset * sensitivity;
    if (pitch > 89.0f) pitch = 89.0f; if (pitch < -89.0f) pitch = -89.0f;
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void processInput(GLFWwindow* window) {
    float cameraSpeed = 5.0f * deltaTime;
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Bioskop Logika - RA 67/2021", primaryMonitor, NULL);

    if (window == NULL) { glfwTerminate(); return 2; }
    glfwMakeContextCurrent(window);
    glewInit();
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetKeyCallback(window, key_callback);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int unifiedShader = createShader("basic.vert", "basic.frag");

    // UCITAVANJE TEKSTURA
    unsigned int signatureTex = setupTexture("res/signature.png");
    unsigned int seatTex = setupTexture("stolica.png");
    // Zadatak 3: Teksture vrata
    unsigned int doorOpenTex = setupTexture("res/open.png");
    unsigned int doorCloseTex = setupTexture("res/close.png");

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

        glClearColor(0.01f, 0.01f, 0.02f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(unifiedShader);
        glUniform1i(transLoc, 1);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)mode->width / (float)mode->height, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        glBindVertexArray(VAO);

        // --- 1. PLATNO ---
        glm::mat4 screenModel = glm::mat4(1.0f);
        screenModel = glm::translate(screenModel, glm::vec3(0.0f, 3.5f, -12.0f));
        screenModel = glm::scale(screenModel, glm::vec3(16.0f, 8.0f, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(screenModel));
        glUniform1i(useTexLoc, 0);
        // Efekat projekcije: Platno malo sija ako je film u toku
        float screenAmb = (currentState == PROJECTION) ? 0.3f : 0.0f;
        glUniform1f(ambLoc, screenAmb);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // --- 2. VRATA (Zadatak 1 & 3) ---
        glm::mat4 doorModel = glm::mat4(1.0f);
        // Postavljamo vrata desno od platna (platno se zavrsava na X=8, vrata na X=10)
        doorModel = glm::translate(doorModel, glm::vec3(10.5f, 0.5f, -12.0f));
        doorModel = glm::scale(doorModel, glm::vec3(1.5f, 2.0f, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(doorModel));
        glUniform1i(useTexLoc, 1);
        glUniform1f(ambLoc, 0.3f); // Vrata moraju uvek biti malo osvetljena

        // Logika za teksturu vrata
        if (currentState == ENTER || currentState == EXIT)
            glBindTexture(GL_TEXTURE_2D, doorOpenTex);
        else
            glBindTexture(GL_TEXTURE_2D, doorCloseTex);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // --- 3. SEDIŠTA (50 komada) ---
        glBindTexture(GL_TEXTURE_2D, seatTex);
        glUniform1i(useTexLoc, 1);
        glUniform1f(ambLoc, 0.15f);
        for (int row = 0; row < 5; row++) {
            for (int col = 0; col < 10; col++) {
                glm::mat4 model = glm::mat4(1.0f);
                float xPos = (col - 4.5f) * 1.5f;
                float yPos = row * 0.5f;
                float zPos = row * 1.5f;
                model = glm::translate(model, glm::vec3(xPos, yPos, zPos));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            }
        }

        // --- 4. POTPIS (2D) ---
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

    glfwTerminate();
    return 0;
}