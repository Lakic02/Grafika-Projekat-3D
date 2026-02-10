// Autori: Nedeljko Tesanovic i Vasilije Markovic
// Opis: 
 
#include <iostream>
#include <fstream>
#include <sstream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

//GLM biblioteke
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Util.h"

bool useTex = false;
bool transparent = false;

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_G && action == GLFW_PRESS) {
        useTex = !useTex;
    }
    if (key == GLFW_KEY_T && action == GLFW_PRESS) {
        transparent = !transparent;
    }
}

unsigned int preprocessTexture(const char* filepath) {
    unsigned int texture = loadImageToTexture(filepath); // Učitavanje teksture
    glBindTexture(GL_TEXTURE_2D, texture); // Vezujemo se za teksturu kako bismo je podesili

    // Generisanje mipmapa - predefinisani različiti formati za lakše skaliranje po potrebi (npr. da postoji 32 x 32 verzija slike, ali i 16 x 16, 256 x 256...)
    glGenerateMipmap(GL_TEXTURE_2D);

    // Podešavanje strategija za wrap-ovanje - šta da radi kada se dimenzije teksture i poligona ne poklapaju
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // S - tekseli po x-osi
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // T - tekseli po y-osi

    // Podešavanje algoritma za smanjivanje i povećavanje rezolucije: nearest - bira najbliži piksel, linear - usrednjava okolne piksele
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return texture;
}

bool firstMouse = true;
float lastX, lastY = 500.0f; // Ekran nam je 1000 x 1000 piksela, kursor je inicijalno na sredini
float yaw = -90.0f, pitch = 0.0f; // yaw -90: kamera gleda u pravcu z ose; pitch = 0: kamera gleda vodoravno
glm::vec3 cameraFront = glm::vec3(0.0, 0.0, -1.0); // at-vektor je inicijalno u pravcu z ose

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}
float fov = 45.0f;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 45.0f)
        fov = 45.0f;
}

int main(void)
{
    if (!glfwInit())
    {
        std::cout<<"GLFW Biblioteka se nije ucitala! :(\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window;
    unsigned int wWidth = 1000;
    unsigned int wHeight = 1000;
    const char wTitle[] = "Vezbe 7";
    window = glfwCreateWindow(wWidth, wHeight, wTitle, NULL, NULL);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    
    if (window == NULL)
    {
        std::cout << "Prozor nije napravljen! :(\n";
        glfwTerminate();
        return 2;
    }
    
    glfwMakeContextCurrent(window);

    
    if (glewInit() != GLEW_OK)
    {
        std::cout << "GLEW nije mogao da se ucita! :'(\n";
        return 3;
    }
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++ PROMJENLJIVE I BAFERI +++++++++++++++++++++++++++++++++++++++++++++++++
    
    unsigned int unifiedShader = createShader("basic.vert", "basic.frag");
    glUseProgram(unifiedShader);
    glUniform1i(glGetUniformLocation(unifiedShader, "uTex"), 0);

    unsigned int dice[6] = { };
    for (int i = 0; i < 6; ++i) {
        std::string path = "res/dice" + std::to_string(i + 1) + ".png";
        dice[i] = preprocessTexture(path.c_str());
    }

    float vertices[] =
    {
      //X    Y    Z      R    G    B    A         S   T
        // Prednja strana (1)
        0.1, 0.1, 0.1,   1.0, 0.0, 0.0, 1.0,      0,  0,    0, 0, 1,
       -0.1, 0.1, 0.1,   1.0, 0.0, 0.0, 1.0,      1,  0,    0, 0, 1,
       -0.1,-0.1, 0.1,   1.0, 0.0, 0.0, 1.0,      1,  1,    0, 0, 1,
        0.1,-0.1, 0.1,   1.0, 0.0, 0.0, 1.0,      0,  1,    0, 0, 1,
       
        // Leva strana (2)
       -0.1, 0.1, 0.1,   0.0, 0.0, 1.0, 1.0,      0,  0,    -1, 0, 0,
       -0.1, 0.1,-0.1,   0.0, 0.0, 1.0, 1.0,      1,  0,    -1, 0, 0,
       -0.1,-0.1,-0.1,   0.0, 0.0, 1.0, 1.0,      1,  1,    -1, 0, 0,
       -0.1,-0.1, 0.1,   0.0, 0.0, 1.0, 1.0,      0,  1,    -1, 0, 0,
       
        // Donja strana (3)
        0.1,-0.1, 0.1,   1.0, 1.0, 1.0, 1.0,      0,  0,    0, -1, 0,
       -0.1,-0.1, 0.1,   1.0, 1.0, 1.0, 1.0,      1,  0,    0, -1, 0,
       -0.1,-0.1,-0.1,   1.0, 1.0, 1.0, 1.0,      1,  1,    0, -1, 0,
        0.1,-0.1,-0.1,   1.0, 1.0, 1.0, 1.0,      0,  1,    0, -1, 0,

        // Gornja strana (4)
        0.1, 0.1, 0.1,   1.0, 1.0, 0.0, 1.0,      0,  0,    0, 1, 0,
        0.1, 0.1,-0.1,   1.0, 1.0, 0.0, 1.0,      1,  0,    0, 1, 0,
       -0.1, 0.1,-0.1,   1.0, 1.0, 0.0, 1.0,      1,  1,    0, 1, 0,
       -0.1, 0.1, 0.1,   1.0, 1.0, 0.0, 1.0,      0,  1,    0, 1, 0,

        // Desna strana (5)
        0.1, 0.1, 0.1,   0.0, 1.0, 0.0, 1.0,      0,  0,    1, 0, 0,
        0.1,-0.1, 0.1,   0.0, 1.0, 0.0, 1.0,      1,  0,    1, 0, 0,
        0.1,-0.1,-0.1,   0.0, 1.0, 0.0, 1.0,      1,  1,    1, 0, 0,
        0.1, 0.1,-0.1,   0.0, 1.0, 0.0, 1.0,      0,  1,    1, 0, 0,
        
        // Zadnja strana (6)
        0.1, 0.1,-0.1,   1.0, 0.5, 0.0, 1.0,      0,  0,    0, 0, -1,
        0.1,-0.1,-0.1,   1.0, 0.5, 0.0, 1.0,      1,  0,    0, 0, -1,
       -0.1,-0.1,-0.1,   1.0, 0.5, 0.0, 1.0,      1,  1,    0, 0, -1,
       -0.1, 0.1,-0.1,   1.0, 0.5, 0.0, 1.0,      0,  1,    0, 0, -1,
    };
    unsigned int stride = (3 + 4 + 2 + 3) * sizeof(float); 
    
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(7 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(10 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++            UNIFORME            +++++++++++++++++++++++++++++++++++++++++++++++++

    glm::mat4 model = glm::mat4(1.0f); //Matrica transformacija - mat4(1.0f) generise jedinicnu matricu
    unsigned int modelLoc = glGetUniformLocation(unifiedShader, "uM");
    
    glm::mat4 view; //Matrica pogleda (kamere)
    glm::vec3 cameraPos = glm::vec3(0.0, 0.0, 2.0);
    glm::vec3 cameraUp = glm::vec3(0.0, 1.0, 0.0);

    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp); // lookAt(Gdje je kamera, u sta kamera gleda, jedinicni vektor pozitivne Y ose svijeta  - ovo rotira kameru)
    unsigned int viewLoc = glGetUniformLocation(unifiedShader, "uV");
    
    
    glm::mat4 projectionP = glm::perspective(glm::radians(fov), (float)wWidth / (float)wHeight, 0.1f, 100.0f); //Matrica perspektivne projekcije (FOV, Aspect Ratio, prednja ravan, zadnja ravan)
    glm::mat4 projectionO = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f); //Matrica ortogonalne projekcije (Lijeva, desna, donja, gornja, prednja i zadnja ravan)
    unsigned int projectionLoc = glGetUniformLocation(unifiedShader, "uP");


    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++ RENDER LOOP - PETLJA ZA CRTANJE +++++++++++++++++++++++++++++++++++++++++++++++++
    glUseProgram(unifiedShader); //Slanje default vrijednosti uniformi
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); //(Adresa matrice, broj matrica koje saljemo, da li treba da se transponuju, pokazivac do matrica)
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projectionO));
    glBindVertexArray(VAO);

    glClearColor(0.5, 0.5, 0.5, 1.0);
    glCullFace(GL_BACK);//Biranje lica koje ce se eliminisati (tek nakon sto ukljucimo Face Culling)

    while (!glfwWindowShouldClose(window))
    {
        double startTime = glfwGetTime();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }

        //Testiranje dubine
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        {
            glEnable(GL_DEPTH_TEST); //Ukljucivanje testiranja Z bafera
        }
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        {
            glDisable(GL_DEPTH_TEST);
        }

        //Odstranjivanje lica (Prethodno smo podesili koje lice uklanjamo sa glCullFace)
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
        {
            glEnable(GL_CULL_FACE);
        }
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
        {
            glDisable(GL_CULL_FACE);
        }

        //Transformisanje trouglova
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            //model = glm::translate(model, glm::vec3(-0.01, 0.0, 0.0)); //Pomjeranje (Matrica transformacije, pomjeraj po XYZ)
            model = glm::rotate(model, glm::radians(-0.5f), glm::vec3(0.0f, 1.0f, 0.0f)); //Rotiranje (Matrica transformacije, ugao rotacije u radijanima, osa rotacije)
            //model = glm::scale(model, glm::vec3(0.99, 1.0, 1.0)); //Skaliranje (Matrica transformacije, skaliranje po XYZ)
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            //model = glm::translate(model, glm::vec3(0.01, 0.0, 0.0));
            model = glm::rotate(model, glm::radians(0.5f), glm::vec3(0.0f, 1.0f, 0.0f));
            //model = glm::scale(model, glm::vec3(1.01, 1.0, 1.0));
        }
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            //model = glm::translate(model, glm::vec3(0.0, 0.01, 0.0));
            model = glm::rotate(model, glm::radians(-0.5f), glm::vec3(1.0f, 0.0f, 1.0f));
            //model = glm::scale(model, glm::vec3(1.0, 1.01, 1.0));
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            //model = glm::translate(model, glm::vec3(0.0, -0.01, 0.0));
            model = glm::rotate(model, glm::radians(0.5f), glm::vec3(1.0f, 0.0f, 1.0f));
            //model = glm::scale(model, glm::vec3(1.0, 0.99, 1.0));
        }

        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        {   
            cameraPos += 0.01f * glm::normalize(glm::vec3(cameraFront.z, 0, -cameraFront.x));
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        {
            cameraPos -= 0.01f * glm::normalize(glm::vec3(cameraFront.z, 0, -cameraFront.x));
        }
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        {
            cameraPos += 0.01f * glm::normalize(glm::vec3(cameraFront.x, 0, cameraFront.z));
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        {
            cameraPos -= 0.01f * glm::normalize(glm::vec3(cameraFront.x, 0, cameraFront.z));
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Osvjezavamo i Z bafer i bafer boje
        
        glUseProgram(unifiedShader);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        projectionP = glm::perspective(glm::radians(fov), (float)wWidth / (float)wHeight, 0.1f, 100.0f);
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projectionP));

        glUniform1i(glGetUniformLocation(unifiedShader, "useTex"), useTex);
        glUniform1i(glGetUniformLocation(unifiedShader, "transparent"), transparent);
        glActiveTexture(GL_TEXTURE0);
        for (int i = 0; i < 6; ++i) {
            glBindTexture(GL_TEXTURE_2D, dice[i]);
            glDrawArrays(GL_TRIANGLE_FAN, i * 4, 4);
        }

        while (glfwGetTime() - startTime < 1.0 / 60) {}
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++ POSPREMANJE +++++++++++++++++++++++++++++++++++++++++++++++++


    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(unifiedShader);

    glfwTerminate();
    return 0;
}
