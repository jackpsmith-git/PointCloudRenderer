#version 450
layout(location = 0) in vec4 inPos;

layout(push_constant) uniform PC {
    mat4 mvp;
} pc;

void main() {
    gl_Position = pc.mvp * inPos;
    gl_PointSize = 1.0;
}
