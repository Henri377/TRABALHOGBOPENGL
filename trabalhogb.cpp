// ------------------------------
// Bibliotecas e namespaces
// ------------------------------
#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <assert.h>
#include <cmath>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

// ------------------------------
// Structs de dados
// ------------------------------

// Sprite genérico para personagens, bandeira, etc.
struct Sprite {
    GLuint VAO;
    GLuint texID;
    vec3 position;
    vec3 dimensions; 
    float ds, dt;
    int iAnimation, iFrame;
    int nAnimations, nFrames;
};

// Tile do mapa
struct Tile {
    GLuint VAO;
    GLuint texID; 
    int iTile; 
    vec3 position;
    vec3 dimensions; 
    float ds, dt;
    bool caminhavel;
};  

// Moeda animada
struct Moeda {
    GLuint VAO;
    GLuint texID;
    vec3 position;
    vec3 dimensions;
    float ds, dt;
    bool coletada;
    int frameAtual;
    int totalFrames;
    double tempoUltimoFrame;
};

// ------------------------------
// Variáveis globais
// ------------------------------
Sprite flag;
bool flagReached = false;

string tilesetFile;
int nTiles, tileW, tileH;
int tilemapWidth, tilemapHeight;
vector<vector<int>> mapConfig;
vector<Tile> tileset;
vec2 pos; // Posição do personagem no mapa

Sprite personagem; // Sprite do personagem

vector<Moeda> moedas; // Vetor de moedas
GLuint moedaTexID;
int moedaW, moedaH;

// ------------------------------
// Protótipos de funções
// ------------------------------
int setupShader();
int setupSprite(int nAnimations, int nFrames, float &ds, float &dt);
int setupTile(int nTiles, float &ds, float &dt);
int loadTexture(string filePath, int &width, int &height);
void desenharMapa(GLuint shaderID);
void desenharAtualTile(GLuint shaderID);
void desenharPersonagem(GLuint shaderID);
void setupMoedas();
void desenharMoedas(GLuint shaderID);
void setupFlag();
void desenharFlag(GLuint shaderID);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// ------------------------------
// Função para carregar configuração do mapa
// ------------------------------
bool loadMapConfig(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Erro ao abrir " << filename << endl;
        return false;
    }
    string line;
    // Linha 1: tilesetIso.png 7 57 114
    getline(file, line);
    stringstream ss(line);
    ss >> tilesetFile >> nTiles >> tileW >> tileH;

    // Linha 2: largura altura
    getline(file, line);
    ss.clear(); ss.str(line);
    ss >> tilemapWidth >> tilemapHeight;

    // Linhas seguintes: mapa
    mapConfig.clear();
    for (int i = 0; i < tilemapHeight; ++i) {
        getline(file, line);
        ss.clear(); ss.str(line);
        vector<int> row;
        for (int j = 0; j < tilemapWidth; ++j) {
            int v; ss >> v;
            row.push_back(v);
        }
        mapConfig.push_back(row);
    }
    file.close();
    return true;
}

// ------------------------------
// Função para carregar tiles não caminháveis de um arquivo
// ------------------------------
void carregarTilesBloqueados(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Erro ao abrir " << filename << endl;
        return;
    }
    int idx;
    while (file >> idx) {
        if (idx >= 0 && idx < (int)tileset.size())
            tileset[idx].caminhavel = false;
    }
    file.close();
}

// ------------------------------
// Shaders (GLSL)
// ------------------------------
const GLchar *vertexShaderSource = R"(
 #version 400
 layout (location = 0) in vec3 position;
 layout (location = 1) in vec2 texc;
 out vec2 tex_coord;
 uniform mat4 model;
 uniform mat4 projection;
 void main()
 {
    tex_coord = texc;
    gl_Position = projection * model * vec4(position, 1.0);
 }
 )";

const GLchar *fragmentShaderSource = R"(
 #version 400
 in vec2 tex_coord;
 out vec4 color;
 uniform sampler2D tex_buff;
 uniform vec2 offsetTex;
 void main()
 {
     color = texture(tex_buff,tex_coord + offsetTex);
 }
 )";

// ------------------------------
// Função para configurar a bandeira animada
// ------------------------------
void setupFlag() {
    int flagW, flagH;
    GLuint flagTexID = loadTexture("flag animation.png", flagW, flagH); 
    flag.nAnimations = 1; // Número de linhas no spritesheet
    flag.nFrames = 5;     // Número de frames de animação
    flag.iAnimation = 0;
    flag.iFrame = 0;
    flag.dimensions = vec3(tileH/1.3, tileW/1.3, 1.0); // Tamanho da flag
    flag.texID = flagTexID;
    float ds, dt;
    flag.VAO = setupSprite(flag.nAnimations, flag.nFrames, ds, dt);
    flag.ds = ds;
    flag.dt = dt;
    float x0 = 470, y0 = 100;
    int lastX = tilemapWidth - 1, lastY = tilemapHeight - 1;
    flag.position = vec3(
        x0 + (lastX-lastY) * tileset[0].dimensions.x/2.0,
        y0 + (lastX+lastY) * tileset[0].dimensions.y/2.0 - 10,
        0.0
    );
}

// ------------------------------
// Função para desenhar a bandeira animada
// ------------------------------
void desenharFlag(GLuint shaderID) {
    static double lastFrameTime = glfwGetTime();
    double now = glfwGetTime();
    if (now - lastFrameTime > 0.1) { // 10 FPS
        flag.iFrame = (flag.iFrame + 1) % flag.nFrames;
        lastFrameTime = now;
    }
    if (flagReached) return;
    mat4 model = mat4(1);
    model = translate(model, flag.position);
    model = scale(model, flag.dimensions);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));
    glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), flag.iFrame * flag.ds, flag.iAnimation * flag.dt);
    glBindVertexArray(flag.VAO);
    glBindTexture(GL_TEXTURE_2D, flag.texID);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// ------------------------------
// Função para configurar as moedas animadas
// ------------------------------
void setupMoedas() {
    moedaTexID = loadTexture("coin_Sheet.png", moedaW, moedaH);
    vector<vec2> posicoesMoedas = {
        {1, 1}, {3, 2}, {5, 3}, {2, 4}, {4, 5}
    };
    for (auto& pos : posicoesMoedas) {
        Moeda moeda;
        moeda.dimensions = vec3(tileH/2, tileW/2, 1.0);
        moeda.texID = moedaTexID;
        moeda.coletada = false;
        moeda.totalFrames = 10;
        moeda.frameAtual = 0;
        moeda.tempoUltimoFrame = glfwGetTime();
        float ds, dt;
        moeda.VAO = setupSprite(1, moeda.totalFrames, ds, dt);
        moeda.ds = ds;
        moeda.dt = dt;
        float x0 = 340;
        float y0 = 100;
        moeda.position = vec3(
            x0 + (pos.x-pos.y) * tileset[0].dimensions.x/2.0,
            y0 + (pos.x+pos.y) * tileset[0].dimensions.y/2.0 - 10,
            0.0
        );
        moedas.push_back(moeda);
    }
}

// ------------------------------
// Função para desenhar as moedas animadas
// ------------------------------
void desenharMoedas(GLuint shaderID) {
    double tempoAtual = glfwGetTime();
    double intervaloFrame = 0.1; // 10 FPS
    for (auto& moeda : moedas) {
        if (!moeda.coletada) {
            if (tempoAtual - moeda.tempoUltimoFrame > intervaloFrame) {
                moeda.frameAtual = (moeda.frameAtual + 1) % moeda.totalFrames;
                moeda.tempoUltimoFrame = tempoAtual;
            }
            mat4 model = mat4(1);
            model = translate(model, moeda.position);
            model = scale(model, moeda.dimensions);
            glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));
            vec2 offsetTex;
            offsetTex.s = moeda.frameAtual * moeda.ds;
            offsetTex.t = 0.0;
            glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), offsetTex.s, offsetTex.t);
            glBindVertexArray(moeda.VAO);
            glBindTexture(GL_TEXTURE_2D, moeda.texID);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
    }
}

// ------------------------------
// Função principal (main)
// ------------------------------
int main()
{
    // Carrega configuração do mapa
    if (!loadMapConfig("map.txt")) return -1;

    // Inicialização da GLFW
    glfwInit();
    glfwWindowHint(GLFW_SAMPLES, 8);

    // Criação da janela GLFW
    const GLuint WIDTH = 800, HEIGHT = 600;
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Jogo com Moedas Animadas", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Falha ao criar a janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Callback de teclado
    glfwSetKeyCallback(window, key_callback);

    // Inicialização do GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Falha ao inicializar GLAD" << std::endl;
        return -1;
    }

    // Informações de versão
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *version = glGetString(GL_VERSION);
    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version supported " << version << endl;

    // Viewport
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Compila shaders
    GLuint shaderID = setupShader();

    // Carrega textura do tileset
    int imgWidth, imgHeight;
    GLuint texID = loadTexture(tilesetFile, imgWidth, imgHeight);

    // Configura tileset
    tileset.clear();
    for (int i=0; i < nTiles; i++)
    {
        Tile tile;
        tile.dimensions = vec3(tileH, tileW, 1.0);
        tile.iTile = i;
        tile.texID = texID;
        tile.VAO = setupTile(nTiles, tile.ds, tile.dt);
        tile.caminhavel = true;
        tileset.push_back(tile);
    }
    // Define tiles não caminháveis
    carregarTilesBloqueados("tiles_bloqueados.txt");

    // Inicializa posição do personagem
    pos.x = 0;
    pos.y = 0;

    // Configuração do sprite do personagem
    int spriteW, spriteH;
    GLuint spriteTexID = loadTexture("personagem_spritesheet.png", spriteW, spriteH);
    personagem.nAnimations = 4;
    personagem.nFrames = 6;
    personagem.iAnimation = 0;
    personagem.iFrame = 0;
    personagem.position = vec3(0, 0, 0);
    personagem.dimensions = vec3(tileH, tileW, 1.0);
    personagem.texID = spriteTexID;
    personagem.VAO = setupSprite(personagem.nAnimations, personagem.nFrames, personagem.ds, personagem.dt);

    // Configura moedas e flag
    setupMoedas();
    setupFlag();

    glUseProgram(shaderID);

    // Uniforms e projeção
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(shaderID, "tex_buff"), 0);
    mat4 projection = ortho(0.0, 800.0, 600.0, 0.0, -1.0, 1.0);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

    // OpenGL states
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ------------------------------
    // Loop principal do jogo
    // ------------------------------
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glLineWidth(10);
        glPointSize(20);

        // Desenho dos elementos
        desenharMapa(shaderID);
        desenharAtualTile(shaderID);
        desenharMoedas(shaderID);
        desenharFlag(shaderID);
        desenharPersonagem(shaderID);

        glfwSwapBuffers(window);
    }
    // Finaliza GLFW
    glfwTerminate();
    return 0;
}

// ------------------------------
// Função de callback de teclado
// ------------------------------
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    vec2 aux = pos;
    int oldAnimation = personagem.iAnimation;

    // Movimentação do personagem
    if (key == GLFW_KEY_W && action == GLFW_PRESS) { if (pos.x > 0) pos.x--; if (pos.y > 0) pos.y--; personagem.iAnimation = 1; }
    if (key == GLFW_KEY_A && action == GLFW_PRESS) { if (pos.x > 0) pos.x--; if (pos.y <= tilemapHeight - 2) pos.y++; personagem.iAnimation = 2; }
    if (key == GLFW_KEY_S && action == GLFW_PRESS) { if (pos.x <= tilemapWidth -2) pos.x++; if (pos.y <= tilemapHeight - 2) pos.y++; personagem.iAnimation = 0; }
    if (key == GLFW_KEY_D && action == GLFW_PRESS) { if (pos.x <= tilemapWidth -2) pos.x++; if (pos.y > 0) pos.y--; personagem.iAnimation = 3; }
    if (key == GLFW_KEY_Q && action == GLFW_PRESS) { if (pos.x > 0) pos.x--; personagem.iAnimation = 2; }
    if (key == GLFW_KEY_E && action == GLFW_PRESS) { if (pos.y > 0) pos.y--; personagem.iAnimation = 3; }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS) { if (pos.y <= tilemapHeight - 2) pos.y++; personagem.iAnimation = 2; }
    if (key == GLFW_KEY_X && action == GLFW_PRESS) { if (pos.x <= tilemapWidth -2) pos.x++; personagem.iAnimation = 3; }

    // Checa se o tile é caminhável
    if (!tileset[mapConfig[(int)pos.y][(int)pos.x]].caminhavel)
    {
        pos = aux; 
    }
    else if (personagem.iAnimation == oldAnimation) {
        personagem.iFrame = (personagem.iFrame + 1) % personagem.nFrames;
    } else {
        personagem.iFrame = 0;
    }

    // Colisão com moedas
    float x0 = 400;
    float y0 = 100;
    vec3 personagemPos = vec3(
        x0 + (pos.x-pos.y) * personagem.dimensions.x/2.0,
        y0 + (pos.x+pos.y) * personagem.dimensions.y/2.0,
        0.0
    );
    for (auto& moeda : moedas) {
        if (!moeda.coletada) {
            float distancia = distance(personagemPos, moeda.position);
            if (distancia < 20.0f) {
                moeda.coletada = true;
                cout << "Moeda coletada!" << endl;
            }
        }
    }

    // Colisão com a flag
    float distanciaFlag = distance(personagemPos, flag.position);
    if (!flagReached && distanciaFlag < 30.0f) {
        flagReached = true;
        cout << "Você chegou na bandeira! Fim de jogo." << endl;
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    cout << "(" << pos.x <<"," << pos.y << ")" << endl;
}

// ------------------------------
// Funções utilitárias de setup e desenho
// ------------------------------
int setupShader()
{
    // Vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // Checando erros de compilação (exibição via log no terminal)
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }
    // Fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // Checando erros de compilação (exibição via log no terminal)
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }
    // Linkando os shaders e criando o identificador do programa de shader
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // Checando por erros de linkagem
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

int setupSprite(int nAnimations, int nFrames, float &ds, float &dt)
{
    ds = 1.0 / (float) nFrames;
    dt = 1.0 / (float) nAnimations;
    GLfloat vertices[] = {
        // x   y    z    s     t
        -0.5,  0.5, 0.0, 0.0, dt, //V0
        -0.5, -0.5, 0.0, 0.0, 0.0, //V1
         0.5,  0.5, 0.0, ds, dt, //V2
         0.5, -0.5, 0.0, ds, 0.0  //V3
        };

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}

int setupTile(int nTiles, float &ds, float &dt)
{
    ds = 1.0 / (float) nTiles;
    dt = 1.0;
    float th = 1.0, tw = 1.0;

    GLfloat vertices[] = {
        // x   y    z    s     t
        0.0,  th/2.0f,   0.0, 0.0,    dt/2.0f, //A
        tw/2.0f, th,     0.0, ds/2.0f, dt,     //B
        tw/2.0f, 0.0,    0.0, ds/2.0f, 0.0,    //D
        tw,     th/2.0f, 0.0, ds,     dt/2.0f  //C
        };

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}

int loadTexture(string filePath, int &width, int &height)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int nrChannels;

    unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

    if (data)
    {
        if (nrChannels == 3) // jpg, bmp
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        }
        else // png
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }

    stbi_image_free(data);

    glBindTexture(GL_TEXTURE_2D, 0);

    return texID;
}

void desenharMapa(GLuint shaderID)
{
    float x0 = 340;
    float y0 = 100;

    for(int i=0; i<tilemapHeight; i++)
    {
        for (int j=0; j < tilemapWidth; j++)
        {
            mat4 model = mat4(1);
            Tile curr_tile = tileset[mapConfig[i][j]];

            float x = x0 + (j-i) * curr_tile.dimensions.x/2.0;
            float y = y0 + (j+i) * curr_tile.dimensions.y/2.0;

            model = translate(model, vec3(x,y,0.0));
            model = scale(model,curr_tile.dimensions);
            glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

            vec2 offsetTex;
            offsetTex.s = curr_tile.iTile * curr_tile.ds;
            offsetTex.t = 0.0;
            glUniform2f(glGetUniformLocation(shaderID, "offsetTex"),offsetTex.s, offsetTex.t);

            glBindVertexArray(curr_tile.VAO);
            glBindTexture(GL_TEXTURE_2D, curr_tile.texID);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
    }
}

void desenharPersonagem(GLuint shaderID)
{
    float x0 = 400; // Posição inicial do personagem no eixo x necessária para centralizar
    float y0 = 130;

    float x = x0 + (pos.x-pos.y) * personagem.dimensions.x/2.0;
    float y = y0 + (pos.x+pos.y) * personagem.dimensions.y/2.0;

    mat4 model = mat4(1);
    model = translate(model, vec3(x,y,0.0));
    model = scale(model, personagem.dimensions);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

    // Calcula o offset do frame atual
    vec2 offsetTex;
    offsetTex.s = personagem.iFrame * personagem.ds;
    offsetTex.t = personagem.iAnimation * personagem.dt;
    glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), offsetTex.s, offsetTex.t);

    glBindVertexArray(personagem.VAO);
    glBindTexture(GL_TEXTURE_2D, personagem.texID);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void desenharAtualTile(GLuint shaderID)
{
    Tile curr_tile = tileset[6]; //tile rosa

    float x0 = 340;
    float y0 = 100;

    float x = x0 + (pos.x-pos.y) * curr_tile.dimensions.x/2.0;
    float y = y0 + (pos.x+pos.y) * curr_tile.dimensions.y/2.0;

    mat4 model = mat4(1);
    model = translate(model, vec3(x,y,0.0));
    model = scale(model,curr_tile.dimensions);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

    vec2 offsetTex;

    offsetTex.s = curr_tile.iTile * curr_tile.ds;
    offsetTex.t = 0.0;
    glUniform2f(glGetUniformLocation(shaderID, "offsetTex"),offsetTex.s, offsetTex.t);

    glBindVertexArray(curr_tile.VAO); // Conectando ao buffer de geometria
    glBindTexture(GL_TEXTURE_2D, curr_tile.texID); // Conectando ao buffer de textura

    // Chamada de desenho - drawcall
    // Poligono Preenchido - GL_TRIANGLES
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}