#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm> // Za std::shuffle, std::remove_if
#include <random>    // Za moderni random
#include <ctime>     // Za seeding

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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
    bool reachedRow = false; // Kod ulaska: stigao do dubine reda. Kod izlaska: stigao do hodnika (X).
    bool reachedSeat = false; // Kod ulaska: stigao na sediste. Kod izlaska: stigao do vrata (Z).
    float speed = 1.0f;
    int modelIndex; // Dodajemo indeks modela (0-14)
};
std::vector<Viewer> visitors;

// --- STANJA ---
enum CinemaState { START, ENTER, PROJECTION, EXIT };
CinemaState currentState = START;

// --- PROJEKCIJA ---
unsigned int movieTextures[20];
float projectionTimer = 0.0f;

// --- SVETLO ---
struct Light {
    glm::vec3 pos;
    glm::vec3 color;
    float intensity;
};

struct ModelVertex {
    glm::vec3 Position;
    glm::vec4 Color;    // Tvoj shader traži layout(location = 1) in vec4 inCol
    glm::vec2 TexCoords;
    glm::vec3 Normal;
};

struct Mesh {
    unsigned int VAO, VBO, EBO;
    std::vector<unsigned int> indices;
    void setupMesh(std::vector<ModelVertex> vertices, std::vector<unsigned int> indices) {
        this->indices = indices;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ModelVertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // Layout mora da se poklapa sa tvojim basic.vert
        glEnableVertexAttribArray(0); // inPos
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)0);
        glEnableVertexAttribArray(1); // inCol
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, Color));
        glEnableVertexAttribArray(2); // inTex
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, TexCoords));
        glEnableVertexAttribArray(3); // inNormal
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, Normal));

        glBindVertexArray(0);
    }
    void Draw() {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

class Model {
public:
    std::vector<Mesh> meshes;
    void loadModel(std::string path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
            return;
        }
        processNode(scene->mRootNode, scene);
    }

private:
    void processNode(aiNode* node, const aiScene* scene) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene) {
        std::vector<ModelVertex> vertices;
        std::vector<unsigned int> indices;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            ModelVertex vertex;
            vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            vertex.Color = glm::vec4(1.0f); // Podrazumevana bela boja
            if (mesh->mTextureCoords[0])
                vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            vertices.push_back(vertex);
        }
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
        Mesh result;
        result.setupMesh(vertices, indices);
        return result;
    }
}; 

std::vector<Model> personModels;
std::vector<unsigned int> personTextures;
int TOTAL_MODELS = 0;

// --- KAMERA ---
glm::vec3 cameraPos = glm::vec3(0.0f, 4.0f, 10.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, -0.2f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

bool firstMouse = true;
float yaw = -90.0f, pitch = 0.0f;
float lastX = 0, lastY = 0;
float deltaTime = 0.0f, lastFrame = 0.0f;
bool mousePressed = false;

// Pozicija vrata za spawn i izlaz
glm::vec3 doorPosSpawn = glm::vec3(11.0f, 0.0f, -12.0f);

void spawnVisitors() {
    visitors.clear();
    projectionTimer = 0.0f;

    std::vector<glm::vec3> occupiedSeatPositions;
    for (int r = 0; r < 5; r++) {
        for (int c = 0; c < 10; c++) {
            if (cinemaSeats[r][c].isReserved || cinemaSeats[r][c].isBought) {
                occupiedSeatPositions.push_back(cinemaSeats[r][c].pos);
            }
        }
    }

    int totalOccupied = (int)occupiedSeatPositions.size();
    if (totalOccupied == 0) return;

    std::default_random_engine engine(static_cast<unsigned int>(time(0)));
    std::uniform_int_distribution<int> modelDist(0, TOTAL_MODELS - 1); // Distribucija za modele
    // Spawnujemo onoliko ljudi koliko ima kupljenih/rezervisanih mesta
    int spawnCount = totalOccupied;

    std::shuffle(occupiedSeatPositions.begin(), occupiedSeatPositions.end(), engine);

    std::uniform_real_distribution<float> speedDist(0.8f, 1.2f);
    float baseSpeed = 3.5f;

    for (int i = 0; i < spawnCount; i++) {
        Viewer v;
        v.currentPos = doorPosSpawn;
        v.targetPos = occupiedSeatPositions[i];
        v.reachedRow = false;
        v.reachedSeat = false;
        v.speed = speedDist(engine) * baseSpeed;
        v.modelIndex = modelDist(engine);
        visitors.push_back(v);
    }
}

// --- CALLBACKS ---
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ENTER) {
            if (currentState == START) {
                currentState = ENTER;
                spawnVisitors();
            }
        }
        if (key == GLFW_KEY_F1) currentState = START;
        if (key == GLFW_KEY_F2) currentState = ENTER;
        if (key == GLFW_KEY_F3) currentState = PROJECTION;
        if (key == GLFW_KEY_F4) {
            currentState = EXIT;
            for (auto& v : visitors) { v.reachedRow = false; v.reachedSeat = false; }
        }

        if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9) {
            if (currentState != START) return;
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
    glm::vec3 nextPos = cameraPos;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) nextPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) nextPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) nextPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) nextPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

    nextPos.x = glm::clamp(nextPos.x, -9.9f, 14.0f);
    nextPos.y = glm::clamp(nextPos.y, 0.5f, 9.5f);
    nextPos.z = glm::clamp(nextPos.z, -11.5f, 9.79f);

    cameraPos = nextPos;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
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

// --- TEKSTURA LJUDI ---
unsigned int humanTex0; //setupTexture("res/tekstura0.jpg");

int main(void) {
    // 1. Inicijalizacija GLFW
    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. Kreiranje prozora
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Cinema Simulation", primaryMonitor, NULL);
    if (!window) {
        glfwTerminate();
        return 2;
    }

    // 3. Postavljanje konteksta (OBAVEZNO PRE GLEW-a)
    glfwMakeContextCurrent(window);

    // 4. Inicijalizacija GLEW-a sa popravkom za glGenerateMipmap
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        return 3;
    }

    // Postavke ulaza i callbacks
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetKeyCallback(window, key_callback);

    // Inicijalizacija sedišta
    for (int r = 0; r < 5; r++)
        for (int c = 0; c < 10; c++)
            cinemaSeats[r][c].pos = glm::vec3((c - 4.5f) * 1.5f, r * 0.5f, r * 1.5f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 5. Učitavanje resursa (Sada je bezbedno jer su funkcije učitane)
    unsigned int unifiedShader = createShader("basic.vert", "basic.frag");
    unsigned int signatureTex = setupTexture("res/signature.png");
    unsigned int seatTex = setupTexture("stolica.png");
    unsigned int doorOpenTex = setupTexture("res/open.png");
    unsigned int doorCloseTex = setupTexture("res/close.png");

    // Učitaj teksturu čoveka (Albedo mapa koju si poslao)
    struct ModelPath {
        std::string model;
        std::string texture;
    };

    std::vector<ModelPath> resourcePaths = {
        {"res/covek0.obj", "res/tekstura0.jpg"},
        {"res/covek1.obj", "res/tekstura1.png"},

    };

    // Automatski postavlja broj modela na osnovu liste iznad
    const int TOTAL_MODELS_COUNT = resourcePaths.size();
    TOTAL_MODELS = TOTAL_MODELS_COUNT;// Program sada zna tačan broj unetih modela

    for (int i = 0; i < TOTAL_MODELS_COUNT; i++) {
        Model m;
        std::cout << "Ucitavam model: " << resourcePaths[i].model << std::endl;
        m.loadModel(resourcePaths[i].model);
        personModels.push_back(m);

        unsigned int tex = setupTexture(resourcePaths[i].texture.c_str());
        if (tex == 0) {
            std::cout << "GRESKA: Tekstura nije pronadjena: " << resourcePaths[i].texture << std::endl;
        }
        personTextures.push_back(tex);
    }

    for (int i = 0; i < 20; i++) {
        std::string path = "res/slika" + std::to_string(i + 1) + ".jfif";
        movieTextures[i] = setupTexture(path.c_str());
    }

    // Bufferi (VAO, VBO)
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
    glEnableVertexAttribArray(3); glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(9 * sizeof(float)));

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
    glEnableVertexAttribArray(3); glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(9 * sizeof(float)));

    unsigned int modelLoc = glGetUniformLocation(unifiedShader, "uM");
    unsigned int viewLoc = glGetUniformLocation(unifiedShader, "uV");
    unsigned int projLoc = glGetUniformLocation(unifiedShader, "uP");
    unsigned int useTexLoc = glGetUniformLocation(unifiedShader, "useTex");
    unsigned int transLoc = glGetUniformLocation(unifiedShader, "transparent");
    unsigned int ambLoc = glGetUniformLocation(unifiedShader, "uAmb");
    unsigned int tintLoc = glGetUniformLocation(unifiedShader, "uTint");

    unsigned int lightPosLoc = glGetUniformLocation(unifiedShader, "uLightPos");
    unsigned int lightColLoc = glGetUniformLocation(unifiedShader, "uLightCol");
    unsigned int lightIntLoc = glGetUniformLocation(unifiedShader, "uLightIntensity");

    double targetFrameTime = 1.0 / 75.0;

   /// Model viewerModel;
    //viewerModel.loadModel("res/covek0.obj");


    while (!glfwWindowShouldClose(window)) {
        double frameStartTime = glfwGetTime();
        deltaTime = (float)frameStartTime - lastFrame;
        lastFrame = (float)frameStartTime;
        processInput(window);

        // --- LOGIKA KRETANJA ---
        if (currentState == ENTER) {
            bool allSeated = true;
            if (visitors.empty()) allSeated = false;
            for (auto& v : visitors) {
                if (!v.reachedRow) {
                    if (std::abs(v.currentPos.z - v.targetPos.z) > 0.05f) {
                        float dir = (v.targetPos.z > v.currentPos.z) ? 1.0f : -1.0f;
                        v.currentPos.z += dir * v.speed * deltaTime;
                        int passedRows = (int)(v.currentPos.z / 1.5f + 0.01f);
                        if (passedRows < 0) passedRows = 0;
                        v.currentPos.y = passedRows * 0.5f;
                        if (v.currentPos.y > v.targetPos.y) v.currentPos.y = v.targetPos.y;
                    }
                    else {
                        v.currentPos.z = v.targetPos.z;
                        v.currentPos.y = v.targetPos.y;
                        v.reachedRow = true;
                    }
                }
                else if (!v.reachedSeat) {
                    if (std::abs(v.currentPos.x - v.targetPos.x) > 0.05f) {
                        float dir = (v.targetPos.x > v.currentPos.x) ? 1.0f : -1.0f;
                        v.currentPos.x += dir * v.speed * deltaTime;
                    }
                    else {
                        v.currentPos.x = v.targetPos.x;
                        v.reachedSeat = true;
                    }
                }
                if (!v.reachedSeat) allSeated = false;
            }
            if (allSeated && !visitors.empty()) {
                currentState = PROJECTION;
                projectionTimer = 0.0f;
            }
        }
        else if (currentState == EXIT) {
            for (auto& v : visitors) {
                if (!v.reachedRow) {
                    if (std::abs(v.currentPos.x - doorPosSpawn.x) > 0.05f) {
                        float dir = (doorPosSpawn.x > v.currentPos.x) ? 1.0f : -1.0f;
                        v.currentPos.x += dir * v.speed * deltaTime;
                    }
                    else {
                        v.currentPos.x = doorPosSpawn.x;
                        v.reachedRow = true;
                    }
                }
                else if (!v.reachedSeat) {
                    if (std::abs(v.currentPos.z - doorPosSpawn.z) > 0.05f) {
                        float dir = (doorPosSpawn.z > v.currentPos.z) ? 1.0f : -1.0f;
                        v.currentPos.z += dir * v.speed * deltaTime;
                        int currentStairRow = (int)(v.currentPos.z / 1.5f + 0.5f);
                        if (currentStairRow < 0) currentStairRow = 0;
                        v.currentPos.y = currentStairRow * 0.5f;
                    }
                    else {
                        v.currentPos.z = doorPosSpawn.z;
                        v.currentPos.y = doorPosSpawn.y;
                        v.reachedSeat = true;
                    }
                }
            }
            visitors.erase(std::remove_if(visitors.begin(), visitors.end(), [](const Viewer& v) {
                return v.reachedSeat;
                }), visitors.end());

            if (visitors.empty()) {
                currentState = START;
                for (int r = 0; r < 5; r++) for (int c = 0; c < 10; c++) {
                    cinemaSeats[r][c].isReserved = false;
                    cinemaSeats[r][c].isBought = false;
                }
            }
        }

        if (currentState == PROJECTION) {
            projectionTimer += deltaTime;
            if (projectionTimer >= 20.0f) {
                currentState = EXIT;
                for (auto& v : visitors) { v.reachedRow = false; v.reachedSeat = false; }
            }
        }

        // --- SVETLO ---
        Light activeLight;
        if (currentState == ENTER || currentState == EXIT) {
            activeLight.pos = glm::vec3(0.0f, 8.0f, 1.5f);
            activeLight.color = glm::vec3(1.0f, 0.95f, 0.85f);
            activeLight.intensity = 2.0f;
        }
        else if (currentState == PROJECTION && projectionTimer <= 20.0f) {
            activeLight.pos = glm::vec3(0.0f, 3.5f, -11.0f);
            activeLight.color = glm::vec3(0.7f, 0.8f, 1.0f);
            activeLight.intensity = 0.8f;
        }
        else {
            activeLight.pos = glm::vec3(0.0f, 0.0f, 0.0f);
            activeLight.color = glm::vec3(0.0f, 0.0f, 0.0f);
            activeLight.intensity = 0.0f;
        }

        glClearColor(0.01f, 0.01f, 0.02f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(unifiedShader);
        glUniform1i(transLoc, 1);
        glUniform3fv(lightPosLoc, 1, glm::value_ptr(activeLight.pos));
        glUniform3fv(lightColLoc, 1, glm::value_ptr(activeLight.color));
        glUniform1f(lightIntLoc, activeLight.intensity);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)mode->width / (float)mode->height, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        // CRTANJE (POD, PLAFON, ZIDOVI...)
        glBindVertexArray(VAO);

        // 1. POD
        glm::mat4 floorModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 6.5f));
        floorModel = glm::rotate(floorModel, glm::radians(-90.0f), glm::vec3(1, 0, 0));
        floorModel = glm::scale(floorModel, glm::vec3(30.0f, 40.0f, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(floorModel));
        glUniform1i(useTexLoc, 0);
        glUniform4f(tintLoc, 0.4f, 0.2f, 0.1f, 1.0f);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // 2. PLAFON
        glm::mat4 ceilModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 10.0f, 6.5f));
        ceilModel = glm::rotate(ceilModel, glm::radians(90.0f), glm::vec3(1, 0, 0));
        ceilModel = glm::scale(ceilModel, glm::vec3(30.0f, 40.0f, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(ceilModel));
        glUniform4f(tintLoc, 0.15f, 0.15f, 0.15f, 1.0f);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // 3. PREDNJI ZID
        glm::mat4 frontWallModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 5.0f, -12.5f));
        frontWallModel = glm::scale(frontWallModel, glm::vec3(30.0f, 10.0f, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(frontWallModel));
        glUniform4f(tintLoc, 0.4f, 0.4f, 0.4f, 1.0f);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // ZADNJI ZID
        glm::mat4 backWallModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 5.0f, 9.9f));
        backWallModel = glm::rotate(backWallModel, glm::radians(180.0f), glm::vec3(0, 1, 0));
        backWallModel = glm::scale(backWallModel, glm::vec3(30.0f, 10.0f, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(backWallModel));
        glUniform4f(tintLoc, 0.35f, 0.35f, 0.35f, 1.0f);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // 4. BOCNI ZIDOVI
        glUniform4f(tintLoc, 0.35f, 0.35f, 0.35f, 1.0f);
        glm::mat4 leftWall = glm::translate(glm::mat4(1.0f), glm::vec3(-10.0f, 5.0f, 6.5f));
        leftWall = glm::rotate(leftWall, glm::radians(90.0f), glm::vec3(0, 1, 0));
        leftWall = glm::scale(leftWall, glm::vec3(40.0f, 10.0f, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(leftWall));
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glm::mat4 rightWall = glm::translate(glm::mat4(1.0f), glm::vec3(15.0f, 5.0f, 6.5f));
        rightWall = glm::rotate(rightWall, glm::radians(-90.0f), glm::vec3(0, 1, 0));
        rightWall = glm::scale(rightWall, glm::vec3(40.0f, 10.0f, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(rightWall));
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // 5. PLATNO
        glm::mat4 screenModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 3.5f, -12.0f));
        screenModel = glm::scale(screenModel, glm::vec3(18.0f, 8.0f, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(screenModel));
        if (currentState == PROJECTION && projectionTimer <= 20.0f) {
            glUniform1i(useTexLoc, 1);
            int frameIndex = (int)(projectionTimer * 2.0f) % 20;
            glBindTexture(GL_TEXTURE_2D, movieTextures[frameIndex]);
            glUniform1f(ambLoc, 0.6f);
        }
        else {
            glUniform1i(useTexLoc, 0);
            glUniform1f(ambLoc, 0.0f);
        }
        glUniform4f(tintLoc, 1, 1, 1, 1);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // 6. VRATA
        glm::mat4 doorModel = glm::translate(glm::mat4(1.0f), glm::vec3(11.0f, 2.0f, -12.0f));
        doorModel = glm::scale(doorModel, glm::vec3(3.0f, 5.0f, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(doorModel));
        glUniform1i(useTexLoc, 1);
        glUniform1f(ambLoc, 0.2f);
        glBindTexture(GL_TEXTURE_2D, (currentState == ENTER || currentState == EXIT ? doorOpenTex : doorCloseTex));
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // 7. SEDIŠTA
        glBindTexture(GL_TEXTURE_2D, seatTex);
        glUniform1i(useTexLoc, 1);
        for (int row = 0; row < 5; row++) {
            for (int col = 0; col < 10; col++) {
                glm::mat4 model = glm::translate(glm::mat4(1.0f), cinemaSeats[row][col].pos + glm::vec3(0.0f, 0.5f, 0.0f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                if (cinemaSeats[row][col].isBought) glUniform4f(tintLoc, 1.0f, 0.0f, 0.0f, 1.0f);
                else if (cinemaSeats[row][col].isReserved) glUniform4f(tintLoc, 1.0f, 1.0f, 0.0f, 1.0f);
                else glUniform4f(tintLoc, 0.3f, 0.3f, 1.0f, 1.0f);
                glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            }
        }

		//humanTex0 = setupTexture("res/tekstura0.jpg");

        // --- 8. LJUDI (3D Modeli) ---
        glUniform1i(useTexLoc, 1);
        glUniform1f(ambLoc, 0.2f);
        glUniform4f(tintLoc, 1.0f, 1.0f, 1.0f, 1.0f);

        for (auto& v : visitors) {
            // 1. Postavi transformaciju
            glm::mat4 model = glm::translate(glm::mat4(1.0f), v.currentPos);
            model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
            // Ako su modeli okrenuti naopako, ovde dodaj glm::rotate
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            // 2. Aktivira teksturu specifičnu za taj model
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, personTextures[v.modelIndex]);

            // 3. Nacrtaj model specifičan za tog posetioca
            for (unsigned int i = 0; i < personModels[v.modelIndex].meshes.size(); i++) {
                personModels[v.modelIndex].meshes[i].Draw();
            }
        }
        glUniform1i(useTexLoc, 0);

        // --- 9. STEPENICE ---
        glBindVertexArray(VAO);
        for (int row = 0; row < 5; row++) {
            glm::mat4 stepModel = glm::mat4(1.0f);
            stepModel = glm::translate(stepModel, glm::vec3(0.0f, row * 0.5f - 0.05f, row * 1.5f));
            stepModel = glm::rotate(stepModel, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            stepModel = glm::scale(stepModel, glm::vec3(30.0f, 1.5f, 1.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(stepModel));
            glUniform4f(tintLoc, 0.2f, 0.1f, 0.05f, 1.0f);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            if (row > 0) {
                glm::mat4 frontModel = glm::mat4(1.0f);
                frontModel = glm::translate(frontModel, glm::vec3(0.0f, row * 0.5f - 0.3f, row * 1.5f - 0.75f));
                frontModel = glm::scale(frontModel, glm::vec3(30.0f, 0.5f, 1.0f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(frontModel));
                glUniform4f(tintLoc, 0.15f, 0.08f, 0.04f, 1.0f);
                glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            }
        }

        // --- 10. POTPIS ---
        glDisable(GL_DEPTH_TEST);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glUniform1i(useTexLoc, 1);
        glBindTexture(GL_TEXTURE_2D, signatureTex);
        glBindVertexArray(sigVAO);
        glUniform4f(tintLoc, 1, 1, 1, 1);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
        while (glfwGetTime() - frameStartTime < targetFrameTime) {}
    }

    glfwTerminate();
    return 0;
}