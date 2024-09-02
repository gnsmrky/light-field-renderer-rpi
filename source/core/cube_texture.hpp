#pragma once

#include <memory>

#include <nanogui/object.h>
#include <nanogui/opengl.h>
#include <nanogui/canvas.h>
#include <nanogui/shader.h>


using nanogui::Shader;
using nanogui::Canvas;
using nanogui::Widget;

class MyTextureCanvas : public Canvas {
public:
    MyTextureCanvas(Widget *parent, const char* texture_file_path);

    virtual ~MyTextureCanvas();

    void set_rotation(float rotation) {
        m_rotation = rotation;
    }

    virtual void draw_contents();

private:
    std::string m_texture_file_path;

    //ref<Shader> m_shader;
    //std::unique_ptr<Shader> m_shader;
    Shader* m_shader;
    float m_rotation;
    
    GLuint m_texture;
    int    m_texture_cx;
    int    m_texture_cy;

    GLuint m_fbo;
    GLuint m_fbo_texture;

    GLuint m_program;

    GLuint m_vao;
    GLuint m_vbo, m_color;
    
    int m_prev_viewport[4] = { 0 };
    int m_prev_scissor[4] = { 0 };
};



