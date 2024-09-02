#include "n-sided-polygon.hpp"

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <nanogui/opengl.h>

NSidedPolygon::NSidedPolygon(int N)
{
    std::vector<glm::uvec3> triangles(N);

    std::vector<glm::vec4> vertices(N + 1); // xyst
    vertices[0] = glm::vec4(0.0f, 0.0f, 0.5f, 0.5f);
    vertices[1] = glm::vec4(0.5f * std::cos(0), 0.5f * std::sin(0), 0.5f * std::cos(0) + 0.5f, 0.5f * std::sin(0) + 0.5f);

    for (int i = 1; i < N; i++)
    {
        float theta = i * glm::two_pi<float>() / N;
        float x = 0.5f * std::cos(theta);
        float y = 0.5f * std::sin(theta);

        vertices[i + 1] = glm::vec4(x, y, x + 0.5f, y + 0.5f);
        triangles[i - 1] = glm::uvec3(0, i, i + 1);
    }
    triangles[N - 1] = glm::uvec3(0, N, 1);

    num_indices = N * 3;

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, N * sizeof(glm::uvec3), triangles.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, (N + 1) * sizeof(glm::vec4), vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
    //NOTE: OpenGL ES 3.1 needs to unbind the buffers/arrays to work correctly
    glBindBuffer(GL_ARRAY_BUFFER,0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

NSidedPolygon::~NSidedPolygon()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void NSidedPolygon::bind()
{
    glBindVertexArray(VAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
}

void NSidedPolygon::draw()
{
    glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, 0);
}