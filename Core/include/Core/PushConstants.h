#pragma once

struct ComputePushConstants
{
    float time;
    uint32_t numTriangles;
};

struct GraphicsPushConstants
{
    glm::mat4 mvp;
};