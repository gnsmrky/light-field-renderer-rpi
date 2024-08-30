#pragma once

inline constexpr char screen_vert[] = R"glsl(#version 310 es
#line 5

precision mediump float;

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texcoord;

out vec2 interpolated_texcoord;

void main()
{
    interpolated_texcoord = texcoord;
    gl_Position = vec4(position * 2.0, 0.0, 1.0);
})glsl";