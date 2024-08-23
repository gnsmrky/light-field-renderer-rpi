#pragma once

inline constexpr char screen_vert[] = R"glsl(
#version 140
#line 5

in vec2 position;
in vec2 texcoord;

out vec2 interpolated_texcoord;

void main()
{
    interpolated_texcoord = texcoord;
    gl_Position = vec4(position * 2.0, 0.0, 1.0);
})glsl";