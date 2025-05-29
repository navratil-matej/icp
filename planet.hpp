#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "Model.h"
#include "cpunoise.hpp"
#include <glm/gtx/norm.hpp>

class Planet
{
public:
    Planet() {}

    Planet(
        ModelAsset & planetoid,
        ShaderProgram & noise_shader,
        float scale,
        std::vector<glm::vec3> uniform_Color,
        std::vector<float> uniform_Height,
        std::vector<float> uniform_Threshold,
        float uniform_AmplitudeRatio,
        glm::vec3 uniform_DistortionAmount,
        float uniform_DistortionSpatial,
        glm::vec3 uniform_BaseSpatial,
        float uniform_SpatialRatio,
        bool uniform_DoMakePoles
    ) {
        // std::copy(uniform_Color     .begin(), uniform_Color     .end(), this->uniform_Color     );
        // std::copy(uniform_Height    .begin(), uniform_Height    .end(), this->uniform_Height    );
        // std::copy(uniform_Threshold .begin(), uniform_Threshold .end(), this->uniform_Threshold );
        this->uniform_Color = uniform_Color;
        this->uniform_Height = uniform_Height;
        this->uniform_Threshold = uniform_Threshold;
        this->uniform_AmplitudeRatio = uniform_AmplitudeRatio;
        this->uniform_DistortionAmount = uniform_DistortionAmount;
        this->uniform_DistortionSpatial = uniform_DistortionSpatial;
        this->uniform_BaseSpatial = uniform_BaseSpatial;
        this->uniform_SpatialRatio = uniform_SpatialRatio;
        this->uniform_DoMakePoles = uniform_DoMakePoles;

        this->model = planetoid.create(noise_shader);
        this->shader = noise_shader;
        this->scale = scale;

        // TODO better way to do this
        std::copy(uniform_Height    .begin(), uniform_Height    .end(), cpu_noise.uniform_Height    );
        std::copy(uniform_Threshold .begin(), uniform_Threshold .end(), cpu_noise.uniform_Threshold );
        cpu_noise.uniform_AmplitudeRatio = uniform_AmplitudeRatio;
        cpu_noise.uniform_DistortionAmount = uniform_DistortionAmount;
        cpu_noise.uniform_DistortionSpatial = uniform_DistortionSpatial;
        cpu_noise.uniform_BaseSpatial = uniform_BaseSpatial;
        cpu_noise.uniform_SpatialRatio = uniform_SpatialRatio;
        cpu_noise.uniform_DoMakePoles = uniform_DoMakePoles;
    }

    void set_default_pos(glm::vec3 default_pos)
    {
        this->default_pos = default_pos;
    }

    void set_orbit(Planet* orbit_origin, float orbit_radius, float orbit_time, float orbit_frac, float revolution_time, float revolution_frac)
    {
        this->orbit_origin = orbit_origin;
        this->orbit_radius = orbit_radius;
        this->orbit_time = orbit_time;
        this->orbit_frac = orbit_frac;
        this->revolution_time = revolution_time;
        this->revolution_frac = revolution_frac;
    }

    void update(float dt)
    {
        // TODO accumulating precision issues
        revolution_frac += dt / revolution_time;
        orbit_frac += dt / orbit_time;
    }

    glm::vec3 get_pos(float sub_dt = 0.0f)
    {
        if(orbit_origin != nullptr)
        {
            return orbit_origin->get_pos() + orbit_radius * glm::vec3(
                std::sin((orbit_frac + sub_dt / orbit_time) * M_PI * 2),
                0.0f,
                std::cos((orbit_frac + sub_dt / orbit_time) * M_PI * 2)
            );
        }
        else
        {
            return default_pos;
        }
    }

    glm::vec3 get_rot(float sub_dt = 0.0f)
    {
        // TODO Axial tilt would be fun
        return glm::vec3(0.0f, (revolution_frac + sub_dt / revolution_time) * M_PI * 2, 0.0f);
    }

    void draw()
    {
        shader.activate();

        shader.setUniform("uniform_Color",      uniform_Color);
        shader.setUniform("uniform_Height",     uniform_Height);
        shader.setUniform("uniform_Threshold",  uniform_Threshold);

        shader.setUniform("uniform_AmplitudeRatio",    uniform_AmplitudeRatio);
        shader.setUniform("uniform_DistortionAmount",  uniform_DistortionAmount);
        shader.setUniform("uniform_DistortionSpatial", uniform_DistortionSpatial);
        shader.setUniform("uniform_BaseSpatial",       uniform_BaseSpatial);
        shader.setUniform("uniform_SpatialRatio",      uniform_SpatialRatio);
        shader.setUniform("uniform_DoMakePoles",       uniform_DoMakePoles);

        model.draw(get_pos(), get_rot(), glm::vec3(scale));
    }

    /// @brief Handles gravity, collision, air drag (TODO).
    /// @param p_position 
    /// @param p_momentum 
    /// @return whether collision happened, to play sound or similar.
    bool act_on(glm::vec3 & p_position, glm::vec3 & p_momentum, glm::vec3 & p_basis_x, glm::vec3 & p_basis_y, glm::vec3 & p_basis_z, float sim_dt)
    {
        auto m = model.make_model_matrix(get_pos(), get_rot(), glm::vec3(scale));
        auto norm_p_position = glm::vec3(glm::inverse(m) * glm::vec4(p_position, 1.0f));
        auto norm_p_sq_dist = glm::length2(norm_p_position);

        // Collision
        bool collided = false;
        auto apos = glm::normalize(norm_p_position);
        auto cpos = cpu_noise.get(apos);
        if(glm::length2(cpos) - norm_p_sq_dist > -0.001f)
        {
            // Find an orthogonal basis to use for sampling slope TODO Use Gram-Schmidt for u instead to be 100% safe
            auto u = glm::normalize(glm::cross(apos, glm::vec3(-apos.z, -apos.x, -apos.y))) * 1e-3f;
            auto v = glm::normalize(glm::cross(apos, u)) * 1e-3f;
            auto n = glm::normalize(glm::cross(cpu_noise.get(apos + u) - cpos, cpu_noise.get(apos + v) - cpos));

            n = glm::normalize(glm::vec3(m * glm::vec4(n, 1.0f)));
            p_position -= p_momentum;
            p_momentum = 0.5f * (p_momentum - 2.0f * glm::dot(p_momentum, n) * n);
            p_position += p_momentum;

            collided = true;
        }

        // Drag - this one could use a lot of work, but as is:
        // Very close = planetoid movement fully transforms player info as if it was static
        // Approaching = the transformations are diminished by distance
        if(norm_p_sq_dist < 9.0f)
        {
            float str = std::min(1.0f, 2.0f / norm_p_sq_dist);
            auto dpos = get_pos() - get_pos(-sim_dt);
            auto drot = get_rot() - get_rot(-sim_dt);
            glm::mat4 m_ry = glm::rotate(glm::mat4(1.0f), drot.y * str, glm::vec3(0.0f, 1.0f, 0.0f));
            p_basis_x = glm::vec3(m_ry * glm::vec4(p_basis_x, 1.0f));
            p_basis_y = glm::vec3(m_ry * glm::vec4(p_basis_y, 1.0f));
            p_basis_z = glm::vec3(m_ry * glm::vec4(p_basis_z, 1.0f));
            // Rotating momentum is very wrong but seems to feel better
            p_momentum = glm::vec3(m_ry * glm::vec4(p_momentum, 1.0f));
            p_position += glm::vec3(m_ry * glm::vec4((p_position - get_pos(-sim_dt)), 1.0f)) - (p_position - get_pos(-sim_dt)) + dpos * str;
        }

        // Grativy - TODO cutoff shouldn't be constant
        if(norm_p_sq_dist < 9.0f)
        {
            p_momentum -= (p_position - get_pos(-sim_dt)) * 0.0001f / norm_p_sq_dist * sim_dt;
        }

        return collided;
    }
private:

    std::vector<glm::vec3> uniform_Color;
    std::vector<float> uniform_Height;
    std::vector<float> uniform_Threshold;

    float uniform_AmplitudeRatio;
    glm::vec3 uniform_DistortionAmount;
    float uniform_DistortionSpatial;
    glm::vec3 uniform_BaseSpatial;
    float uniform_SpatialRatio;
    bool uniform_DoMakePoles;

    Model model;
    ShaderProgram shader;
    CpuNoise cpu_noise;

    float scale = 1.0f;

    Planet* orbit_origin = nullptr;
    glm::vec3 default_pos = glm::vec3(0.0f);
    float orbit_radius = 0.0f;
    float orbit_time = INFINITY;
    float orbit_frac = 0.0f;
    float revolution_time = INFINITY;
    float revolution_frac = 0.0f;
};