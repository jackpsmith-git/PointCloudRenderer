#version 450
layout(location = 0) out vec4 outColor;

void main() {
    // soft circular glow using gl_PointCoord
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    float r = length(uv);
    float alpha = smoothstep(1.0, 0.2, r);
    vec3 color = vec3(1.0, 0.85, 0.6);
    float core = smoothstep(0.4, 0.0, r);
    vec3 final = color * (0.6 * alpha + 0.9 * core);
    outColor = vec4(final, alpha);
}
