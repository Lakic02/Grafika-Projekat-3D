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

// --- LOGIKA SEDIŠTA ---
struct Seat {
    glm::vec3 pos;
    bool isReserved = false;
    bool isBought = false;
};
Seat cinemaSeats[5][10];

// --- ENTITET: GLEDALAC ---
struct Viewer {
    glm::vec3 currentPos;
    glm::vec3 targetPos;
    bool reachedRow = false;
    bool reachedSeat = false;
    float speed = 3.0f;
};
std::vector<Viewer> visitors;

// --- STANJA ---
enum CinemaState { START, ENTER, PROJECTION, EXIT };
CinemaState currentState = START;

// --- KAMERA ---
glm::vec3 cameraPos = glm::vec3(0.0f, 4.0f, 15.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, -0.2f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

bool firstMouse = true;
float yaw = -90.0f, pitch = 0.0f;
float lastX = 0, lastY = 0;
float deltaTime = 0.0f, lastFrame = 0.0f;
bool mousePressed = false;

// Pozicija vrata za spawn
glm::vec3 doorPosSpawn = glm::vec3(11.0f, 0.0f, -12.0f);

void spawnVisitors() {
    visitors.clear();
    for (int r = 0; r < 5; r++) {
        for (int c = 0; c < 10; c++) {
            if (cinemaSeats[r][c].isReserved || cinemaSeats[r][c].isBought) {
                Viewer v;
                v.currentPos = doorPosSpawn;
                v.targetPos = cinemaSeats[r][c].pos;
                v.reachedRow = false;
                v.reachedSeat = false;
                visitors.push_back(v);
            }
        }
    }
}

// --- CALLBACKS (Vraćena originalna logika + Enter proširenje) ---
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
      //  if (key == GLFW_KEY_1) currentState = START;
        if (key == GLFW_KEY_ENTER) currentState = ENTER;
        //if (key == GLFW_KEY_3) currentState = PROJECTION;
       // if (key == GLFW_KEY_4) currentState = EXIT;

        // Prosirenje 1: Enter taster
        if (key == GLFW_KEY_ENTER) {
            currentState = ENTER;
            spawnVisitors();
        }

        // --- KUPLJENJE SEDIŠTA (Originalna logika nepromenjena) ---
        if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9 ) {
			if (currentState != START) return;  // Kupovina dozvoljena samo u START stanju

			printf("Attempting to buy %d seats...\n", key - GLFW_KEY_0);
            int nSeats = key - GLFW_KEY_0;
            for (int row = 4; row >= 0; row--) {
                for (int col = 0; col <= 10 - nSeats; col++) {
                    bool canBuy = true;
                    for (int k = 0; k < nSeats; k++) {
                        if (cinemaSeats[row][col + k].isBought || cinemaSeats[row][col + k].isReserved) {
                            canBuy = false;
                            break;
                        }
                    }
                    if (canBuy) {
                        for (int k = 0; k < nSeats; k++)
                            cinemaSeats[row][col + k].isBought = true;
                        goto boughtDone;
                    }
                }
            }
        boughtDone:;
        }
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

    // --- REZERVACIJA NA KLIK (Originalna logika nepromenjena) ---
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && currentState == START) {
        if (!mousePressed && currentState == START) {
            mousePressed = true;
            bool seatClicked = false;
            for (int r = 0; r < 5 && !seatClicked; r++) {
                for (int c = 0; c < 10 && !seatClicked; c++) {
                    glm::vec3 toSeat = glm::normalize(cinemaSeats[r][c].pos - cameraPos);
                    float dotProduct = glm::dot(cameraFront, toSeat);
                    float distance = glm::distance(cameraPos, cinemaSeats[r][c].pos);
                    if (dotProduct > 0.995f && distance < 20.0f && !cinemaSeats[r][c].isBought) {
                        cinemaSeats[r][c].isReserved = !cinemaSeats[r][c].isReserved;
                        seatClicked = true;
                    }
                }
            }
        }
    }
    else {
        mousePressed = false;
    }
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
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Cinema Simulation", primaryMonitor, NULL);
    if (!window) { glfwTerminate(); return 2; }
    glfwMakeContextCurrent(window);
    glewInit();
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetKeyCallback(window, key_callback);

    for (int r = 0; r < 5; r++)
        for (int c = 0; c < 10; c++)
            cinemaSeats[r][c].pos = glm::vec3((c - 4.5f) * 1.5f, r * 0.5f, r * 1.5f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int unifiedShader = createShader("basic.vert", "basic.frag");
    unsigned int signatureTex = setupTexture("res/signature.png");
    unsigned int seatTex = setupTexture("stolica.png");
    unsigned int doorOpenTex = setupTexture("res/open.png");
    unsigned int doorCloseTex = setupTexture("res/close.png");

    float vertices[] = {
        -0.5f, -0.5f, 0.0f,   1,1,1,1,  0,0,  0,0,1,
         0.5f, -0.5f, 0.0f,   1,1,1,1,  1,0,  0,0,1,
         0.5f,  0.5f, 0.0f,   1,1,1,1,  1,1,  0,0,1,
        -0.5f,  0.5f, 0.0f,   1,1,1,1,  0,1,  0,0,1
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
    glBindVertexArray(VAO); glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    unsigned int stride = 12 * sizeof(float);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(7 * sizeof(float)));

    float sigVertices[] = {
        0.40f, -0.95f, 0.0f,  1,1,1,0.4f,  0,0,  0,0,1,
        0.95f, -0.95f,0.0f,  1,1,1,0.4f,  1,0,  0,0,1,
        0.95f, -0.85f,0.0f,  1,1,1,0.4f,  1,1,  0,0,1,
        0.40f, -0.85f,0.0f,  1,1,1,0.4f,  0,1,  0,0,1
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
    unsigned int tintLoc = glGetUniformLocation(unifiedShader, "uTint");

    double targetFrameTime = 1.0 / 75.0;

    while (!glfwWindowShouldClose(window)) {
        double frameStartTime = glfwGetTime();
        deltaTime = (float)frameStartTime - lastFrame;
        lastFrame = (float)frameStartTime;
        processInput(window);

        // LOGIKA KRETANJA POSITILACA
        if (currentState == ENTER) {
            for (auto& v : visitors) {
                if (!v.reachedRow) {
                    if (v.currentPos.z < v.targetPos.z) {
                        v.currentPos.z += v.speed * deltaTime;
                        if (v.currentPos.z >= v.targetPos.z) { v.currentPos.z = v.targetPos.z; v.reachedRow = true; }
                    }
                    else {
                        v.currentPos.z -= v.speed * deltaTime;
                        if (v.currentPos.z <= v.targetPos.z) { v.currentPos.z = v.targetPos.z; v.reachedRow = true; }
                    }
                }
                else if (!v.reachedSeat) {
                    if (v.currentPos.x < v.targetPos.x) {
                        v.currentPos.x += v.speed * deltaTime;
                        if (v.currentPos.x >= v.targetPos.x) { v.currentPos.x = v.targetPos.x; v.reachedSeat = true; }
                    }
                    else {
                        v.currentPos.x -= v.speed * deltaTime;
                        if (v.currentPos.x <= v.targetPos.x) { v.currentPos.x = v.targetPos.x; v.reachedSeat = true; }
                    }
                }
                v.currentPos.y = v.targetPos.y;
            }
        }

        glClearColor(0.01f, 0.01f, 0.02f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(unifiedShader);
        glUniform1i(transLoc, 1);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)mode->width / (float)mode->height, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        glBindVertexArray(VAO);

        // 1. PLATNO (Originalni kod)
        glm::mat4 screenModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 3.5f, -12.0f));
        screenModel = glm::scale(screenModel, glm::vec3(18.0f, 8.0f, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(screenModel));
        glUniform1i(useTexLoc, 0);
        glUniform1f(ambLoc, (currentState == PROJECTION ? 0.3f : 0.0f));
        glUniform4f(tintLoc, 1, 1, 1, 1);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // 2. VRATA (Vraćena originalna logika koda bez belih okvira)
        glm::mat4 doorModel = glm::translate(glm::mat4(1.0f), glm::vec3(11.0f, 2.0f, -12.0f));
        doorModel = glm::scale(doorModel, glm::vec3(3.0f, 5.0f, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(doorModel));
        glUniform1i(useTexLoc, 1);
        glUniform1f(ambLoc, 0.2f);
        glBindTexture(GL_TEXTURE_2D, (currentState == ENTER || currentState == EXIT ? doorOpenTex : doorCloseTex));
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // 3. SEDIŠTA (Vraćena originalna logika koda za bojenje)
        glBindTexture(GL_TEXTURE_2D, seatTex);
        glUniform1i(useTexLoc, 1);
        for (int row = 0; row < 5; row++) {
            for (int col = 0; col < 10; col++) {
                glm::mat4 model = glm::translate(glm::mat4(1.0f), cinemaSeats[row][col].pos);
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

                if (cinemaSeats[row][col].isBought)
                    glUniform4f(tintLoc, 1.0f, 0.0f, 0.0f, 1.0f);
                else if (cinemaSeats[row][col].isReserved)
                    glUniform4f(tintLoc, 1.0f, 1.0f, 0.0f, 1.0f);
                else
                    glUniform4f(tintLoc, 0.3f, 0.3f, 1.0f, 1.0f);

                glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            }
        }

        // 4. LJUDI (Novo proširenje - Ljubičasti kvadri)
        glUniform1i(useTexLoc, 0);
        glUniform1f(ambLoc, 0.1f);
        for (auto& v : visitors) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), v.currentPos + glm::vec3(0, 0.5f, 0));
            model = glm::scale(model, glm::vec3(0.6f, 1.2f, 0.2f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform4f(tintLoc, 0.6f, 0.1f, 0.8f, 1.0f);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }

        // 5. POTPIS (Originalni kod)
        glDisable(GL_DEPTH_TEST);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glUniform1i(useTexLoc, 1);
        glUniform1f(ambLoc, 0.0f);
        glUniform4f(tintLoc, 1, 1, 1, 1);
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