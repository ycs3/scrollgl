#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

void process_input(GLFWwindow* window, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

GLFWwindow* init_window(int width, int height, const char* title)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return NULL;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        glfwTerminate();
        return NULL;
    }

    glEnable(GL_DEPTH_TEST);
    return window;
}

int init_shader(GLenum shader_type, const char* path, unsigned int* shader)
{
    /* read path */
    std::string code;
    std::ifstream file;
    file.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    try {
        file.open(path);
        std::stringstream stream;
        stream << file.rdbuf();
        file.close();
        code = stream.str();
    }
    catch (std::ifstream::failure& e) {
        fprintf(stderr, "Failed to read file: %s\n", path);
        fprintf(stderr, "%s\n", e.what());
    }
    const char* source = code.c_str();

    /* compile shader */ 
    int success;
    *shader = glCreateShader(shader_type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);

    char infoLog[512];
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(*shader, 512, NULL, infoLog);
        if (shader_type == GL_VERTEX_SHADER)
            fprintf(stderr, "Failed compilation, vertex shader\n");
        else if (shader_type == GL_FRAGMENT_SHADER)
            fprintf(stderr, "Failed compilation, fragment shader\n");
        else
            fprintf(stderr, "Failed compilation, unknown shader\n");
        fprintf(stderr, "%s", infoLog);
        glfwTerminate();
    }
    return success;
}

int link_program(unsigned int vertex_shader, unsigned int fragment_shader, unsigned int* shader_program)
{
    int success;
    *shader_program = glCreateProgram();
    glAttachShader(*shader_program, vertex_shader);
    glAttachShader(*shader_program, fragment_shader);
    glLinkProgram(*shader_program);

    char infoLog[512];
    glGetProgramiv(*shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(*shader_program, 512, NULL, infoLog);
        fprintf(stderr, "Failed linking, shader program\n");
        fprintf(stderr, "%s", infoLog);
        glfwTerminate();
    }
    return success;
}

int bind_VAO_32(unsigned int* VAO, unsigned int* VBO, float* vertices, size_t vertices_len)
{
    glBindVertexArray(*VAO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices_len, vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind
    glBindVertexArray(0); // unbind
    return 1;
}

int bind_VAO_22(unsigned int* VAO, unsigned int* VBO, float* vertices, size_t vertices_len)
{
    glBindVertexArray(*VAO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices_len, vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind
    glBindVertexArray(0); // unbind
    return 1;
}

int bind_texture(unsigned int* texture, GLint internal_format, GLenum format, const char* path)
{
    int width, height, n_channels;
    unsigned char* data = stbi_load(path, &width, &height, &n_channels, 0);
    if (!data) {
        fprintf(stderr, "Failed image load\n");
        return 0;
    }
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    return 1;
}

int bind_texture_data(unsigned int* texture, GLint internal_format, GLenum format, unsigned char* data, int width, int height)
{
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    return 1;
}

float vertices[] = {
    0.0f, 0.0f,  0.0f, 0.0f,
    1.0f, 0.0f,  1.0f, 0.0f,
    1.0f, 1.0f,  1.0f, 1.0f,
    1.0f, 1.0f,  1.0f, 1.0f,
    0.0f, 1.0f,  0.0f, 1.0f,
    0.0f, 0.0f,  0.0f, 0.0f
};

int main()
{
    GLFWwindow* window;
    unsigned int vertex_shader, fragment_shader, shader_program;
    const char* vertex_shader_path = "texture.vs";
    const char* fragment_shader_path = "texture.fs";
    unsigned int VAO, VBO;
    unsigned int texture;

    //stbi_set_flip_vertically_on_load(true);

    window = init_window(256, 240, "ScrollGL");
    if (window == NULL)
        return -1;

    if (!init_shader(GL_VERTEX_SHADER, vertex_shader_path, &vertex_shader))
        return -1;
    if (!init_shader(GL_FRAGMENT_SHADER, fragment_shader_path, &fragment_shader))
        return -1;
    if (!link_program(vertex_shader, fragment_shader, &shader_program))
        return -1;
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    if (!bind_VAO_22(&VAO, &VBO, vertices, sizeof(vertices)))
        return -1;



    int width, height, n_channels;
    unsigned char* data = stbi_load("tileset.png", &width, &height, &n_channels, 0);
    if (!data) {
        fprintf(stderr, "Failed image load\n");
        return 0;
    }
    unsigned char *cropped_img = (unsigned char *)malloc(16 * 16 * 4);
    int crop_height = 16;
    int crop_width = 16;
    int crop_x = 0;
    int crop_y = 0;
    for (int y = 0; y < crop_height; y++) {
        memcpy(cropped_img + y * crop_width * n_channels,
               data + ((y + crop_y) * width + crop_x) * n_channels,
               crop_width * n_channels);
    }

    glGenTextures(1, &texture);
    if (!bind_texture_data(&texture, GL_RGBA, GL_RGBA, cropped_img, crop_width, crop_height))
        return -1;
    stbi_image_free(data);

    glUseProgram(shader_program);
    glUniform1i(glGetUniformLocation(shader_program, "ourTexture"), 0);

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        process_input(window, deltaTime);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        


        glUseProgram(shader_program);

        glm::mat4 projection;
        projection = glm::ortho(0.0f, 256.0f, 240.0f, 0.0f, -1.0f, 1.0f);

        int projectionLoc = glGetUniformLocation(shader_program, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(glm::vec2(100.0f, 100.0f), 0.0f));
        model = glm::scale(model, glm::vec3(glm::vec2(16.0f, 16.0f), 1.0f));
        int modelLoc = glGetUniformLocation(shader_program, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glDeleteProgram(shader_program);
    glfwTerminate();
    return 0;
}
