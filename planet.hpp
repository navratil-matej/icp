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

    glm::vec3 get_pos()
    {
        if(orbit_origin != nullptr)
        {
            return orbit_origin->get_pos() + orbit_radius * glm::vec3(std::sin(orbit_frac * M_PI * 2), 0.0f, std::cos(orbit_frac * M_PI * 2));
        }
        else
        {
            return default_pos;
        }
    }

    glm::vec3 get_rot()
    {
        // TODO Axial tilt would be fun
        return glm::vec3(0.0f, revolution_frac * M_PI * 2, 0.0f);
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
    /// @return normal vector of the collision in world-space or zero vector.
    glm::vec3 handle_player(glm::vec3 p_position, glm::vec3 p_momentum)
    {
        auto m = model.make_model_matrix(get_pos(), get_rot(), glm::vec3(scale));
        auto norm_p_position = glm::vec3(glm::inverse(m) * glm::vec4(p_position, 1.0f));

        auto apos = glm::normalize(norm_p_position);
        auto cpos = cpu_noise.get(apos);
        if(glm::length2(cpos) - glm::length2(norm_p_position) > -0.001f)
        {
            // Find an orthogonal basis to use for sampling slope TODO Use Gram-Schmidt for u instead to be 100% safe
            auto u = glm::normalize(glm::cross(apos, glm::vec3(-apos.z, -apos.x, -apos.y))) * 1e-3f;
            auto v = glm::normalize(glm::cross(apos, u)) * 1e-3f;
            auto n = glm::normalize(glm::cross(cpu_noise.get(apos + u) - cpos, cpu_noise.get(apos + v) - cpos));

            return glm::normalize(glm::vec3(m * glm::vec4(n, 1.0f)));

            // std::cout << "Collision" << std::endl;

            // Cancel movement step that caused intersection, reflect momentum along the normal, not 100% elastic

            // co -= momentum;
            // momentum = 0.5f * (momentum - 2.0f * glm::dot(momentum, n) * n);
            // play_audio("impact.wav", cpos, viewmat);
        }

        return glm::vec3(0.0f);
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