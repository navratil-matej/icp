#pragma once

#include <glm/glm.hpp> 

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;

    bool operator==(const Vertex & rhs)
    {
        return this->Position == rhs.Position && this->Normal == rhs.Normal && this->TexCoords == rhs.TexCoords;
    }
};

