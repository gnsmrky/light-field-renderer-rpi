#pragma once

class NSidedPolygon
{
public:
    NSidedPolygon(int N);

    ~NSidedPolygon();

    void bind();
    void draw();
    void unbind();

    unsigned int VBO, VAO, EBO;

    int num_indices;
};