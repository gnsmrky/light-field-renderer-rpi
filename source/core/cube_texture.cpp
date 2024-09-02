#include <nanogui/opengl.h>
#include <nanogui/canvas.h>
#include <nanogui/shader.h>


#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <unistd.h>
#include <iostream>

#include "cube_texture.hpp"

constexpr float Pi = 3.14159f;

const char vert_shader_source_310es[] = "#version 310 es              \n"
                                    "precision highp float;     \n"

                                    "uniform mat4 mvp;          \n"
                                    "layout (location = 0) in vec3 position;          \n"
                                    "layout (location = 1) in vec3 color;             \n"
                                    //"in vec2 texcoord;          \n"

                                    "out vec4 frag_color;       \n"
                                    //"out vec2 uv;               \n"

                                    "void main() {                                   \n"
                                    "   gl_Position = vec4(position, 1.0);              \n"
                                    "   frag_color = vec4(color, 1.0);              \n"
                                    //"    gl_Position = mvp * vec4(position, 1.0);    \n"
                                    //"    uv = texcoord;                              \n"
                                    "}";

const char frag_shader_source_310es[] = "#version 310 es              \n"
                                    "precision highp float;     \n"
                                    
                                    "uniform sampler2D image;   \n"
                                    "in vec4 frag_color;        \n"
                                    //"in vec2 uv;                \n"
                                    "out vec4 c;                \n"

                                    "void main() {                                      \n"
                                    "    vec4 value = texture(image, frag_color.xy);    \n"
                                    //"    c = value;                                     \n"
                                    "    c = vec4(value.xyz, 1.0);                   \n"
                                    //"    c = vec4(value.x, 0.0, 0.0, 1.0);                   \n"
                                    //"    c = frag_color;                   \n"
                                    "}";


#define POSITION 0
#define COLOR    1

static GLuint buildShader(const char* shader_source, GLenum type)
{
    GLuint shader;
    GLint status;

    shader = glCreateShader(type);
    if (shader == 0) {
        return 0;
    }

    glShaderSource(shader, 1, &shader_source, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        int length;
        char* log;

        fprintf(stderr, "failed to compile shader\n");
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        if (length > 1) {
            log = (char*)calloc(length, sizeof(char));
            glGetShaderInfoLog(shader, length, &length, log);
            fprintf(stderr, "%s\n", log);
            free(log);
        }
        return false;
    }

    return shader;
}

static GLuint createAndLinkProgram(GLuint v_shader, GLuint f_shader)
{
    GLuint program;
    GLint linked;

    program = glCreateProgram();
    if (program == 0) {
        fprintf(stderr, "failed to create program\n");
        return 0;
    }

    glAttachShader(program, v_shader);
    glAttachShader(program, f_shader);

    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &linked);

    if (linked != GL_TRUE) {
        int length;
        char* log;

        fprintf(stderr, "failed to link program\n");
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        if (length > 1) {
            log = (char*)calloc(length, sizeof(char));
            glGetProgramInfoLog(program, length, &length, log);
            fprintf(stderr, "%s\n", log);
            free(log);
        }
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

void drawTriangle(GLuint program, GLuint vao, GLuint texture)
{
    glUseProgram(program);

    glClear(GL_COLOR_BUFFER_BIT);

    //glEnableVertexAttribArray(POSITION);
    //glBindBuffer(GL_ARRAY_BUFFER, vbo);
    //glVertexAttribPointer(POSITION, 3, GL_FLOAT, GL_FALSE, 0, 0);

    //glEnableVertexAttribArray(COLOR);
    //glBindBuffer(GL_ARRAY_BUFFER, color);
    //glVertexAttribPointer(COLOR, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindVertexArray(vao);

    glBindTexture(GL_TEXTURE_2D, texture);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(POSITION);
    glDisableVertexAttribArray(COLOR);

    //glutSwapBuffers();
}


using nanogui::Shader;
using nanogui::Canvas;
using nanogui::Widget;

MyTextureCanvas::MyTextureCanvas(Widget *parent, const char* texture_file_path) :
    Canvas(parent, 1), m_texture_file_path(texture_file_path), m_rotation(0.f) {
    using namespace nanogui;

    m_shader = new Shader(
        render_pass(),

        // An identifying name
        "a_simple_texture_shader",

#if defined(NANOGUI_USE_OPENGL)
        // Vertex shader
        R"(#version 330
        uniform mat4 mvp;
        in vec3 position;
        in vec3 color;
        out vec4 frag_color;
        void main() {
            frag_color = vec4(color, 1.0);
            gl_Position = mvp * vec4(position, 1.0);
        })",

        // Fragment shader
        R"(#version 330
        out vec4 color;
        in vec4 frag_color;
        void main() {
            color = frag_color;
        })"
#elif defined(NANOGUI_USE_GLES)
        // Vertex shader
        R"(#version 310 es      
        precision highp float;

        uniform mat4 mvp;
        layout (location = 0) in vec3 position;
        layout (location = 1) in vec3 color;
        in vec2 texcoord;

        out vec4 frag_color;
        out vec2 uv;

        void main() {
            frag_color = vec4(color, 1.0);
            gl_Position = mvp * vec4(position, 1.0);
            uv = texcoord;
        })",

        // Fragment shader
        R"(#version 310 es
        precision highp float;
        
        uniform sampler2D image;
        in vec4 frag_color;
        in vec2 uv;
        out vec4 c;

        void main() {
            vec4 value = texture(image, frag_color.xy);
            c = value;

            //NOTE: <-- by un-comment this line, it makes GLES 3 compiler generates incorrect shader (missing 'color')
            //          GLESv3 is implemented by mesa's GLESv2.  But the this library does not compile version 310 es shader correctly...
            //          GLESv3 is not part of mesa environment.
            //c = vec4(1.0, 0.0, 0.0, 1.0);

            //NOTE: Seems like OpenGL ES compiler would strip out unused attributes during compilation.  The above fixed color value 
            //          'c' variable renders 'color' in vertex shader has no dependencies.  Thus 'color' would be removed during 
            //          shader compilation.
            //          However, this following line makes use of 'frag_color', which depends on 'color' in vertex shader.  So 'color'
            //          attribute is kept.
            //c = vec4(value.x, 0.0, 0.0, 1.0);
        })"
#elif defined(NANOGUI_USE_METAL)
        // Vertex shader
        R"(using namespace metal;

        struct VertexOut {
            float4 position [[position]];
            float4 color;
        };

        vertex VertexOut vertex_main(const device packed_float3 *position,
                                        const device packed_float3 *color,
                                        constant float4x4 &mvp,
                                        uint id [[vertex_id]]) {
            VertexOut vert;
            vert.position = mvp * float4(position[id], 1.f);
            vert.color = float4(color[id], 1.f);
            return vert;
        })",

        /* Fragment shader */
        R"(using namespace metal;

        struct VertexOut {
            float4 position [[position]];
            float4 color;
        };

        fragment float4 fragment_main(VertexOut vert [[stage_in]]) {
            return vert.color;
        })"
#endif
    );

    uint32_t indices[3*12] = {
        3, 2, 6, 6, 7, 3,
        4, 5, 1, 1, 0, 4,
        4, 0, 3, 3, 7, 4,
        1, 5, 6, 6, 2, 1,
        0, 1, 2, 2, 3, 0,
        7, 6, 5, 5, 4, 7
    };

    float positions[3*8] = {
        -1.f,  1.f,  1.f, -1.f, -1.f,  1.f,
         1.f, -1.f,  1.f,  1.f,  1.f,  1.f,
        -1.f,  1.f, -1.f, -1.f, -1.f, -1.f,
         1.f, -1.f, -1.f,  1.f,  1.f, -1.f
    };

    float colors[3*8] = {
        0.0, 1.0, 1.0, 0.0, 0.0, 1.0,
        1.0, 0.0, 1.0, 1.0, 1.0, 1.0,
        0.0, 1.0, 0.0, 0.0, 0.0, 0.5,
        1.0, 0.0, 0.0, 1.0, 1.0, 0.5
    };

    m_shader->set_buffer("indices", VariableType::UInt32, {3*12}, indices);
    m_shader->set_buffer("position", VariableType::Float32, {8, 3}, positions);
    m_shader->set_buffer("color", VariableType::Float32, {8, 3}, colors);
    
    // load texture file
    //char cwd[PATH_MAX] = "";
    //getcwd(cwd, sizeof(cwd));

    //std::string texture_file_path = std::string(cwd) + "/shop-1-row/Original Camera_00_00_400.000000_400.000000_30_36.jpg";
    int texture_file_exists = access (m_texture_file_path.c_str(), F_OK);
    if (texture_file_exists == 0) {
        std::cout << "\r" << std::string(96, ' ');
        std::cout << "\rLoading " << texture_file_path;

        //int width, height, channels;
        int channels;
        uint8_t* image_data = stbi_load(m_texture_file_path.c_str(), &m_texture_cx, &m_texture_cy, &channels, 0);
        if (image_data != nullptr) {
            int pixel_format;
            switch (channels)
            {
                case 1: pixel_format = GL_RED; break;
                case 2: pixel_format = GL_RG; break;
                case 3: pixel_format = GL_RGB; break;
                case 4: pixel_format = GL_RGBA; break;
                default:
                    stbi_image_free(image_data);
                    goto errExit;
            }
            
            glGenTextures(1, &m_texture);
            glBindTexture(GL_TEXTURE_2D, m_texture);
            glTexImage2D(GL_TEXTURE_2D, 0, pixel_format, m_texture_cx, m_texture_cy, 0, pixel_format, GL_UNSIGNED_BYTE, image_data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            
            errExit:
            stbi_image_free(image_data);

        }
    }

    // create quad for drawing texture to frame buffer
    #if 0
    GLfloat vVertices[] = {
        -1.0f, -1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        
        -1.0f,  1.0f, 0.0f,
        1.0f,  1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
    };

    
    float vColors[] = { // used as texture st coordinates
        0.0f,  0.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        1.0f,  0.0f, 0.0f,
        
        0.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        1.0f,  0.0f, 0.0f,
    };

    //glClearColor(0, 0, 0, 1);
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices), vVertices, GL_STATIC_DRAW);

    glGenBuffers(1, &m_color);
    glBindBuffer(GL_ARRAY_BUFFER, m_color);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vColors), vColors, GL_STATIC_DRAW);

    #endif
    
    GLfloat vVertices[] = {
        -1.0f, -1.0f, 0.0f,    0.0f,  0.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,    0.0f, -1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,    1.0f,  0.0f, 0.0f,
        
        -1.0f,  1.0f, 0.0f,    0.0f, -1.0f, 0.0f,
            1.0f,  1.0f, 0.0f,    1.0f, -1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,    1.0f,  0.0f, 0.0f
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices), &vVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    // create frame buffer
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glGenTextures(1, &m_fbo_texture);
    glBindTexture(GL_TEXTURE_2D, m_fbo_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_texture_cx, m_texture_cy, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fbo_texture, 0);

    // 310es shaders
    GLuint v_shader = buildShader(vert_shader_source_310es, GL_VERTEX_SHADER);
    if (v_shader == 0) {
        fprintf(stderr, "failed to build vertex shader\n");
        return;
    }

    GLuint f_shader = buildShader(frag_shader_source_310es, GL_FRAGMENT_SHADER);
    if (f_shader == 0) {
        fprintf(stderr, "failed to build fragment shader\n");
        glDeleteShader(v_shader);
        return;
    }

    glReleaseShaderCompiler(); // should release resources allocated for the compiler

    m_program = createAndLinkProgram(v_shader, f_shader);
    if (m_program == 0) {
        fprintf(stderr, "failed to create and link program\n");
        glDeleteShader(v_shader);
        glDeleteShader(f_shader);
        return;
    }

    //glUseProgram(m_program);

    // this won't actually delete the shaders until the program is closed but it's a good practice
    glDeleteShader(v_shader);
    glDeleteShader(f_shader);

    // bind fbo
    glGetIntegerv(GL_VIEWPORT, m_prev_viewport);
    glViewport(0, 0, m_texture_cx, m_texture_cy);

    glGetIntegerv(GL_SCISSOR_BOX, m_prev_viewport);
    glScissor(0, 0, m_texture_cx, m_texture_cy);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);

    // draw texture to frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    drawTriangle (m_program, m_vao, m_texture);

    // unbind fbo
    glViewport(m_prev_viewport[0], m_prev_viewport[1], m_prev_viewport[2], m_prev_viewport[3]);
    glScissor(m_prev_scissor[0], m_prev_scissor[1], m_prev_scissor[2], m_prev_scissor[3]);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

MyTextureCanvas::~MyTextureCanvas() {
    glDeleteTextures(1, &m_texture);
}

void MyTextureCanvas::draw_contents() {
    using namespace nanogui;

    Matrix4f view = Matrix4f::look_at(
        Vector3f(0, -2, -10),
        Vector3f(0, 0, 0),
        Vector3f(0, 1, 0)
    );

    Matrix4f model = Matrix4f::rotate(
        Vector3f(0, 1, 0),
        (float) glfwGetTime()
    );

    Matrix4f model2 = Matrix4f::rotate(
        Vector3f(1, 0, 0),
        m_rotation
    );

    Matrix4f proj = Matrix4f::perspective(
        float(25 * Pi / 180),
        0.1f,
        20.f,
        m_size.x() / (float) m_size.y()
    );

    Matrix4f mvp = proj * view * model * model2;

    m_shader->set_uniform("mvp", mvp);

    // Draw 12 triangles starting at index 0
    m_shader->begin();
    
    //glActiveTexture(GL_TEXTURE0 + m_texture);
    //glBindTexture(GL_TEXTURE_2D, m_texture);
    
    //glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, m_fbo_texture);

    m_shader->draw_array(Shader::PrimitiveType::Triangle, 0, 12*3, true);
    m_shader->end();
}




