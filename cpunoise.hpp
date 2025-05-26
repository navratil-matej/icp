// Made from the shader to make sure the exact algorithm is used for collisions
#include <glm/glm.hpp>
#include <glm/ext.hpp>

class CpuNoise
{
public:
    float uniform_Height[9];
    float uniform_Threshold[9];

    // TODO defaults duplicated from shader -> redundancy
    // uniform float uniform_BaseAmplitude = 1.0f;
    float uniform_AmplitudeRatio = 0.5f;
    glm::vec3 uniform_DistortionAmount = glm::vec3(0.0f);
    float uniform_DistortionSpatial = 1.0f;
    glm::vec3 uniform_BaseSpatial = glm::vec3(1.0f);
    float uniform_SpatialRatio = 2.0f;
    bool uniform_DoMakePoles = true;

    glm::vec3 mod289(glm::vec3 x)
    {
        return x - floor(x * (1.0f / 289.0f)) * 289.0f;
    }

    glm::vec4 mod289(glm::vec4 x)
    {
        return x - floor(x * (1.0f / 289.0f)) * 289.0f;
    }

    glm::vec4 permute(glm::vec4 x)
    {
        return mod289(((x * 34.0f) + 10.0f) * x);
    }

    glm::vec4 taylorInvSqrt(glm::vec4 r)
    {
        return 1.79284291400159f - 0.85373472095314f * r;
    }

    float snoise(glm::vec3 v)
    {
        const glm::vec2 C = glm::vec2(1.0f / 6.0f, 1.0f / 3.0f);
        const glm::vec4 D = glm::vec4(0.0f, 0.5f, 1.0f, 2.0f);

        // First corner
        glm::vec3 i = glm::floor(v + dot(v, glm::vec3(C.y)));
        glm::vec3 x0 = v - i + dot(i, glm::vec3(C.x));

        // Other corners
        glm::vec3 g = step(glm::vec3(x0.y, x0.z, x0.x), x0);
        glm::vec3 l = 1.0f - g;
        glm::vec3 i1 = min(g, glm::vec3(l.z, l.x, l.y));
        glm::vec3 i2 = max(g, glm::vec3(l.z, l.x, l.y));

        //   x0 = x0 - 0.0 + 0.0 * C.xxx;
        //   x1 = x0 - i1  + 1.0 * C.xxx;
        //   x2 = x0 - i2  + 2.0 * C.xxx;
        //   x3 = x0 - 1.0 + 3.0 * C.xxx;
        glm::vec3 x1 = x0 - i1 + glm::vec3(C.x);
        glm::vec3 x2 = x0 - i2 + glm::vec3(C.y); // 2.0*C.x = 1/3 = C.y
        glm::vec3 x3 = x0 - glm::vec3(D.y);      // -1.0+3.0*C.x = -0.5 = -D.y

        // Permutations
        i = mod289(i);
        glm::vec4 p = permute(permute(permute(
            i.z + glm::vec4(0.0, i1.z, i2.z, 1.0)) +
            i.y + glm::vec4(0.0, i1.y, i2.y, 1.0)) +
            i.x + glm::vec4(0.0, i1.x, i2.x, 1.0));

        // Gradients: 7x7 points over a square, mapped onto an octahedron.
        // The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
        float n_ = 0.142857142857; // 1.0/7.0
        glm::vec3 ns = n_ * glm::vec3(D.w, D.y, D.z) - glm::vec3(D.x, D.z, D.x);

        glm::vec4 j = p - 49.0f * floor(p * ns.z * ns.z); //  mod(p,7*7)

        glm::vec4 x_ = floor(j * ns.z);
        glm::vec4 y_ = floor(j - 7.0f * x_); // mod(j,N)

        glm::vec4 x = x_ * ns.x + glm::vec4(ns.y);
        glm::vec4 y = y_ * ns.x + glm::vec4(ns.y);
        glm::vec4 h = 1.0f - abs(x) - abs(y);

        glm::vec4 b0 = glm::vec4(x.x, x.y, y.x, y.y);
        glm::vec4 b1 = glm::vec4(x.z, x.w, y.z, y.w);

        // glm::vec4 s0 = glm::vec4(lessThan(b0,0.0))*2.0 - 1.0;
        // glm::vec4 s1 = glm::vec4(lessThan(b1,0.0))*2.0 - 1.0;
        glm::vec4 s0 = floor(b0) * 2.0f + 1.0f;
        glm::vec4 s1 = floor(b1) * 2.0f + 1.0f;
        glm::vec4 sh = -step(h, glm::vec4(0.0));

        glm::vec4 a0 = glm::vec4(b0.x, b0.z, b0.y, b0.w) + glm::vec4(s0.x, s0.z, s0.y, s0.w) * glm::vec4(sh.x, sh.x, sh.y, sh.y);
        glm::vec4 a1 = glm::vec4(b1.x, b1.z, b1.y, b1.w) + glm::vec4(s1.x, s1.z, s1.y, s1.w) * glm::vec4(sh.z, sh.z, sh.w, sh.w);

        glm::vec3 p0 = glm::vec3(a0.x, a0.y, h.x);
        glm::vec3 p1 = glm::vec3(a0.z, a0.w, h.y);
        glm::vec3 p2 = glm::vec3(a1.x, a1.y, h.z);
        glm::vec3 p3 = glm::vec3(a1.z, a1.w, h.w);

        // Normalise gradients
        glm::vec4 norm = taylorInvSqrt(glm::vec4(dot(p0, p0), dot(p1, p1), dot(p2, p2), dot(p3, p3)));
        p0 *= norm.x;
        p1 *= norm.y;
        p2 *= norm.z;
        p3 *= norm.w;

        // Mix final noise value
        glm::vec4 m = max(0.5f - glm::vec4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), 0.0f);
        m = m * m;
        return 105.0 * dot(m * m, glm::vec4(dot(p0, x0), dot(p1, x1), dot(p2, x2), dot(p3, x3)));
    }

    float mix_at(float lo, float hi, float val)
    {
        return (val - lo) / (hi - lo);
    }

    glm::vec3 get(glm::vec3 attribute_Position)
    {
        glm::vec3 s1 =      uniform_BaseSpatial;
        glm::vec3 s2 = s1 * uniform_SpatialRatio;
        glm::vec3 s3 = s2 * uniform_SpatialRatio;
        glm::vec3 s4 = s3 * uniform_SpatialRatio;
        glm::vec3 s5 = s4 * uniform_SpatialRatio;
        glm::vec3 s6 = s5 * uniform_SpatialRatio;

        float a1 =      uniform_AmplitudeRatio;
        float a2 = a1 * uniform_AmplitudeRatio;
        float a3 = a2 * uniform_AmplitudeRatio;
        float a4 = a3 * uniform_AmplitudeRatio;
        float a5 = a4 * uniform_AmplitudeRatio;
        float a6 = a5 * uniform_AmplitudeRatio;

        float b_amplitude = a1 + a2 + a3 + a4 + a5 + a6;

        // glm::vec3 pos = attribute_Position + uniform_DistortionAmount * snoise(attribute_Position * uniform_DistortionSpatial + 9999.0f);
        glm::vec3 pos = attribute_Position + uniform_DistortionAmount * glm::vec3(
            snoise(attribute_Position * uniform_DistortionSpatial + 9999.0f),
            snoise(attribute_Position * uniform_DistortionSpatial - 4444.0f),
            snoise(attribute_Position * uniform_DistortionSpatial - 9999.0f)
        );

        float scale = b_amplitude
            + a1 * snoise(pos * s1 + 1234.0f)
            + a2 * snoise(pos * s2 + 4321.0f)
            + a3 * snoise(pos * s3 + 5678.0f)
            + a4 * snoise(pos * s4 + 8765.0f)
            + a5 * snoise(pos * s5 + 1111.0f)
            + a6 * snoise(pos * s6 + 3333.0f);
        // scale = scale / (b_amplitude + a1 + a2 + a3 + a4 + a5 + a6);
        scale = scale / (b_amplitude * 2); // Very similar for 0.5 ratio, not so much if different

        float noise_value;
        if (uniform_DoMakePoles)
        {
            float mixt = attribute_Position.y * attribute_Position.y;
            mixt = mixt * mixt;
            mixt = mixt * mixt;
            mixt = 1.0f - 1.5f * mixt;
            noise_value = scale * mixt;
        }
        else
        {
            noise_value = scale;
        }

        // TODO benchmark this - is it really worth it?
        float displacement;
        if (noise_value < uniform_Threshold[4])
        {
            if (noise_value < uniform_Threshold[2])
            {
                if (noise_value < uniform_Threshold[1])
                {
                    displacement = glm::mix(uniform_Height[0], uniform_Height[1], mix_at(uniform_Threshold[0], uniform_Threshold[1], noise_value));
                }
                else
                {
                    displacement = glm::mix(uniform_Height[1], uniform_Height[2], mix_at(uniform_Threshold[1], uniform_Threshold[2], noise_value));
                }
            }
            else
            {
                if (noise_value < uniform_Threshold[3])
                {
                    displacement = glm::mix(uniform_Height[2], uniform_Height[3], mix_at(uniform_Threshold[2], uniform_Threshold[3], noise_value));
                }
                else
                {
                    displacement = glm::mix(uniform_Height[3], uniform_Height[4], mix_at(uniform_Threshold[3], uniform_Threshold[4], noise_value));
                }
            }
        }
        else
        {
            if (noise_value < uniform_Threshold[6])
            {
                if (noise_value < uniform_Threshold[5])
                {
                    displacement = glm::mix(uniform_Height[4], uniform_Height[5], mix_at(uniform_Threshold[4], uniform_Threshold[5], noise_value));
                }
                else
                {
                    displacement = glm::mix(uniform_Height[5], uniform_Height[6], mix_at(uniform_Threshold[5], uniform_Threshold[6], noise_value));
                }
            }
            else
            {
                if (noise_value < uniform_Threshold[7])
                {
                    displacement = glm::mix(uniform_Height[6], uniform_Height[7], mix_at(uniform_Threshold[6], uniform_Threshold[7], noise_value));
                }
                else
                {
                    displacement = glm::mix(uniform_Height[7], uniform_Height[8], mix_at(uniform_Threshold[7], uniform_Threshold[8], noise_value));
                }
            }
        }
        // gl_Position = uP_m * uV_m * uM_m * glm::vec4(attribute_Position * displacement, 1.0f);
        // gl_Position = uP_m * uV_m * uM_m * glm::vec4(attribute_Position * (0.7 + 0.2 * max(scale * mix, 0.5f)), 1.0f);
        return attribute_Position * displacement;
    }
};
