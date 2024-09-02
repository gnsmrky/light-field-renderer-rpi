#pragma once

class Quad
{
public:
    Quad();

    ~Quad();

    void bind();
    void draw();
    void unbind();

    unsigned int VBO, VAO;
};