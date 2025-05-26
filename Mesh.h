#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp> 
#include <glm/ext.hpp>

#include "Vertex.h"
#include "ShaderProgram.hpp"
#include <iostream>

class Mesh {
public:
    // mesh data
    glm::vec3 origin{};
    glm::vec3 orientation{};
    
    GLuint texture_id{0}; // texture id=0  means no texture
    GLenum primitive_type = GL_POINT;
    ShaderProgram &shader;
    
    // mesh material
    glm::vec4 ambient_material{1.0f}; //white, non-transparent 
    glm::vec4 diffuse_material{1.0f}; //white, non-transparent 
    glm::vec4 specular_material{1.0f}; //white, non-transparent
    float reflectivity{1.0f}; 

    // These do not function
    // std::vector<Vertex> const & vertices;
    // std::vector<GLuint> const & indices;
    // Using this instead
    size_t size;
    
    // indirect (indexed) draw 
	Mesh(GLenum primitive_type, ShaderProgram & shader, std::vector<Vertex> const & vertices, std::vector<GLuint> const & indices, glm::vec3 const & origin, glm::vec3 const & orientation, GLuint const texture_id = 0):
        primitive_type(primitive_type), shader(shader), /*vertices(vertices), indices(indices),*/ origin(origin), orientation(orientation), texture_id(texture_id)
    {

        glCreateVertexArrays(1, &VAO);

        GLint attrib_pos = shader.attrib_pos();
        glEnableVertexArrayAttrib(VAO, attrib_pos);
        glVertexArrayAttribFormat(VAO, attrib_pos, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Position));
        glVertexArrayAttribBinding(VAO, attrib_pos, 0); // (GLuint vaobj, GLuint attribindex, GLuint bindingindex)
        
        GLint attrib_norm = shader.attrib_norm();
        glEnableVertexArrayAttrib(VAO, attrib_norm);
        glVertexArrayAttribFormat(VAO, attrib_norm, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Normal));
        glVertexArrayAttribBinding(VAO, attrib_norm, 0); // (GLuint vaobj, GLuint attribindex, GLuint bindingindex)
        
        GLint attrib_tex = shader.attrib_tex();
        glEnableVertexArrayAttrib(VAO, attrib_tex);
        glVertexArrayAttribFormat(VAO, attrib_tex, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, TexCoords));
        glVertexArrayAttribBinding(VAO, attrib_tex, 0); // (GLuint vaobj, GLuint attribindex, GLuint bindingindex)

        glCreateBuffers(1, &VBO);
        glNamedBufferData(VBO, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        glCreateBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        
        glVertexArrayVertexBuffer(VAO, 0, VBO, 0, sizeof(Vertex)); // (GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride)
        
        size = indices.size();
    };

    void draw(glm::mat4 const & model_matrix = glm::mat4(1.0f)) const
    {
 		if (VAO == 0)
        {
			std::cerr << "VAO not initialized!\n";
			return;
		}
 
        shader.activate();
        // norm, tex

        glBindVertexArray(VAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

        // TODO apply relative transform
        shader.setUniform("uM_m", model_matrix);

        // glDrawArrays(primitive_type, 0, size);
        glDrawElements(primitive_type, size, GL_UNSIGNED_INT, (void*)0);

        
        // for future use: set uniform variables: position, textures, etc...  
        //set texture id etc...
        //if (texture_id > 0) {
        //    ...
        //}
        
        //TODO: draw mesh: bind vertex array object, draw all elements with selected primitive type 
    }


	void clear(void) {
        texture_id = 0;
        primitive_type = GL_POINT;
        // TODO: clear rest of the member variables to safe default
        
        // TODO: delete all allocations 
        // glDeleteBuffers...
        // glDeleteVertexArrays...
        
    };

private:
    // OpenGL buffer IDs
    // ID = 0 is reserved (i.e. uninitalized)
     unsigned int VAO{0}, VBO{0}, EBO{0};
};
  


