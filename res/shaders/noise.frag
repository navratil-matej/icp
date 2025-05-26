#version 460 core

in float noise_value;

// Input from vertex shader
in VS_OUT {
    vec3 N;
    vec3 SunL;
    vec3 PlayerL;
    vec3 V;
    vec3 F;
    vec2 texCoord;
} fs_in;

uniform vec3[9] uniform_Color;
uniform float[9] uniform_Threshold;

// -- Light module
uniform vec3 uniform_SunPos, uniform_PlayerPos, uniform_PlayerForward;
uniform float uniform_FlashlightInner, uniform_FlashlightOuter;
uniform vec3 uniform_AmbientI, uniform_DiffuseSunI, uniform_SpecularSunI, uniform_DiffusePlayerI, uniform_SpecularPlayerI;
uniform vec3 uniform_AmbientM, uniform_DiffuseM, uniform_SpecularM;
uniform float uniform_SpecularShininess;
// -- / Light module

out vec4 frag_color;

float mix_at(float lo, float hi, float val)
{
  return (val - lo) / (hi - lo);
}

void main()
{
  // TODO benchmark this - is it really worth it?
  
  vec4 col = vec4(0.0f);
  if(noise_value < uniform_Threshold[4])
  {
    if(noise_value < uniform_Threshold[2])
    {
      if(noise_value < uniform_Threshold[1])
      {
        col = vec4(mix(uniform_Color[0], uniform_Color[1], mix_at(uniform_Threshold[0], uniform_Threshold[1], noise_value)), 1.0f);
      }
      else
      {
        col = vec4(mix(uniform_Color[1], uniform_Color[2], mix_at(uniform_Threshold[1], uniform_Threshold[2], noise_value)), 1.0f);
      }
    }
    else
    {
      if(noise_value < uniform_Threshold[3])
      {
        col = vec4(mix(uniform_Color[2], uniform_Color[3], mix_at(uniform_Threshold[2], uniform_Threshold[3], noise_value)), 1.0f);
      }
      else
      {
        col = vec4(mix(uniform_Color[3], uniform_Color[4], mix_at(uniform_Threshold[3], uniform_Threshold[4], noise_value)), 1.0f);
      }
    }
  }
  else
  {
    if(noise_value < uniform_Threshold[6])
    {
      if(noise_value < uniform_Threshold[5])
      {
        col = vec4(mix(uniform_Color[4], uniform_Color[5], mix_at(uniform_Threshold[4], uniform_Threshold[5], noise_value)), 1.0f);
      }
      else
      {
        col = vec4(mix(uniform_Color[5], uniform_Color[6], mix_at(uniform_Threshold[5], uniform_Threshold[6], noise_value)), 1.0f);
      }
    }
    else
    {
      if(noise_value < uniform_Threshold[7])
      {
        col = vec4(mix(uniform_Color[6], uniform_Color[7], mix_at(uniform_Threshold[6], uniform_Threshold[7], noise_value)), 1.0f);
      }
      else
      {
        col = vec4(mix(uniform_Color[7], uniform_Color[8], mix_at(uniform_Threshold[7], uniform_Threshold[8], noise_value)), 1.0f);
      }
    }
  }

    // -- Light module
    // Ambient
    vec3 ambient = uniform_AmbientM * uniform_AmbientI;

    // Sun
    // Normalize the incoming N, L and V vectors
    vec3 N = normalize(fs_in.N);
    vec3 V = normalize(fs_in.V);
    vec3 L = normalize(fs_in.SunL);
    // Calculate R by reflecting -L around the plane defined by N
    vec3 R = reflect(-L, N);
    // Calculate diffuse and specular contribution
    vec3 diffuse = max(dot(N, L), 0.0) * uniform_DiffuseM * uniform_DiffuseSunI;
    vec3 specular = pow(max(dot(R, V), 0.0), uniform_SpecularShininess) * uniform_SpecularM * uniform_SpecularSunI;

    // Player
    L = normalize(fs_in.PlayerL);
    R = reflect(-L, N);
    // float theta = dot(L, V);
    float theta = dot(L, -normalize(fs_in.F));
    float epsilon = uniform_FlashlightInner - uniform_FlashlightOuter;
    float q = clamp((theta - uniform_FlashlightOuter) / epsilon, 0.0, 1.0);
    diffuse += max(dot(N, L), 0.0) * q * uniform_DiffuseM * uniform_DiffusePlayerI;
    specular += pow(max(dot(R, V), 0.0), uniform_SpecularShininess) * q * uniform_SpecularM * uniform_SpecularPlayerI;

    // Add it up
    frag_color = vec4( (ambient + diffuse) * col.xyz + specular, 1.0);
    // -- / Light module

    // frag_color = col;
}



// uniform vec3 uniform_AmbientI, uniform_DiffuseI, uniform_SpecularI;
// uniform vec3 uniform_AmbientM, uniform_DiffuseM, uniform_SpecularM;
// uniform float uniform_SpecularShininess;