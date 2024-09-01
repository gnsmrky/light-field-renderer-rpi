#pragma once

class LFShader
{
public:
    LFShader(const char* vert_source, const char* frag_source);

    ~LFShader();

    int getLocation(const char* name);

    void use();

    int handle;
};