#version 450
layout(location = 0) in vec4 inPos;

layout(push_constant) uniform PC {
    mat4 mvp;
} pc;

void main() {
    gl_Position = pc.mvp * inPos;
    // simple size; you can make it vary by depth in C++
    gl_PointSize = 4.0;
}
