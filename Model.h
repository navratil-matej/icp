#pragma once

#include <filesystem>
#include <string>
#include <vector> 
#include <glm/glm.hpp> 

#include "Vertex.h"
#include "Mesh.h"
#include "ShaderProgram.hpp"
#include "OBJloader.hpp"

class Model
{
public:
    std::vector<Mesh> meshes;
    std::string name;
    glm::vec3 origin{};
    glm::vec3 orientation{};

    Model() { }

    Model(const std::filesystem::path & filename, ShaderProgram & shader)
    {
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        loadOBJidx(filename.c_str(), vertices, indices);
        
        meshes.push_back(Mesh(GL_TRIANGLES, shader, vertices, indices, glm::vec3(0), glm::vec3(0), 0));

        // load mesh (all meshes) of the model, load material of each mesh, load textures...
        // TODO: call LoadOBJFile √, LoadMTLFile (if exist), process data √, create mesh√? and set its properties
        //    notice: you can load multiple meshes and place them to proper positions, 
        //            multiple textures (with reusing) etc. to construct single complicated Model   
    }

    Model(const std::vector<std::filesystem::path> & filenames, ShaderProgram & shader)
    {
        for(auto filename : filenames)
        {
            std::vector<Vertex> vertices;
            std::vector<GLuint> indices;
            loadOBJidx(filename.c_str(), vertices, indices);
            
            meshes.push_back(Mesh(GL_TRIANGLES, shader, vertices, indices, glm::vec3(0), glm::vec3(0), 0));

            // load mesh (all meshes) of the model, load material of each mesh, load textures...
            // TODO: call LoadOBJFile √, LoadMTLFile (if exist), process data √, create mesh√? and set its properties
            //    notice: you can load multiple meshes and place them to proper positions, 
            //            multiple textures (with reusing) etc. to construct single complicated Model  
        } 
    }

    Model(const std::vector<Vertex> & vertices, const std::vector<GLuint> & indices, ShaderProgram & shader)
    {
        meshes.push_back(Mesh(GL_TRIANGLES, shader, vertices, indices, glm::vec3(0), glm::vec3(0), 0));
    }

    // update position etc. based on running time
    void update(const float delta_t) {
        // origin += glm::vec3(3,0,0) * delta_t; s=s0+v*dt
    }
    
    void draw(glm::vec3 const & offset = glm::vec3(0.0), glm::vec3 const & rotation = glm::vec3(0.0f), glm::vec3 const & scale = glm::vec3(1.0f)) {
        // call draw() on mesh (all meshes)
        draw(make_model_matrix(offset, rotation, scale));
    }
    
    void draw(const glm::mat4 & model_matrix) {
        for (auto const& mesh : meshes)
        {
            mesh.draw(model_matrix);
        }
    }

    const glm::mat4 make_model_matrix(glm::vec3 const & offset = glm::vec3(0.0), glm::vec3 const & rotation = glm::vec3(0.0f), glm::vec3 const & scale = glm::vec3(1.0f))
    {
        // compute complete transformation
		glm::mat4 m_t = glm::translate(glm::mat4(1.0f), origin);
		glm::mat4 m_rx = glm::rotate(glm::mat4(1.0f), orientation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 m_ry = glm::rotate(glm::mat4(1.0f), orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 m_rz = glm::rotate(glm::mat4(1.0f), orientation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);

		glm::mat4 d_t = glm::translate(glm::mat4(1.0f), offset);
		glm::mat4 d_rx = glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 d_ry = glm::rotate(glm::mat4(1.0f), rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 d_rz = glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		// glm::mat4 m_s = glm::scale(glm::mat4(1.0f), scale_change);

		// !!! Tweaked this a lil: move to origin -> rotate -> move to position
        // Also, y -> x rather than x -> y is necessary for yaw/pitch to function
        // Also moved scale step, should be scaled from origin
        
        return glm::mat4(1.0f) * /*s **/ d_t * d_rz * d_rx * d_ry * /*m_s **/ m_rz * m_rx * m_ry * s * m_t;
    }
};

class ModelAsset
{
public:
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    ModelAsset(const std::filesystem::path & filename)
    {
        loadOBJidx(filename.c_str(), vertices, indices);
    }

    Model create(ShaderProgram & shader)
    {
        return Model(vertices, indices, shader);
    }
};