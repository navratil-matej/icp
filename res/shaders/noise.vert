#version 460 core

in vec3 attribute_Position;
in vec3 attribute_Normal;
in vec2 attribute_TexCoord;

uniform mat4 uP_m = mat4(1.0f);
uniform mat4 uM_m = mat4(1.0f);
uniform mat4 uV_m = mat4(1.0f);

uniform float[9] uniform_Height;
uniform float[9] uniform_Threshold;


// -- Light module
uniform vec3 uniform_SunPos, uniform_PlayerPos, uniform_PlayerForward;
// -- / Light module

// uniform float uniform_BaseAmplitude = 1.0f;
uniform float uniform_AmplitudeRatio = 0.5f;
uniform vec3 uniform_DistortionAmount = vec3(0.0f);
uniform float uniform_DistortionSpatial = 1.0f;
uniform vec3  uniform_BaseSpatial = vec3(1.0f);
uniform float uniform_SpatialRatio = 2.0f;
uniform bool uniform_DoMakePoles = true;

uniform bool uniform_RecalcNormals = true;
uniform float uniform_RecalcDist = 3e-3;

out float noise_value;

// Outputs to the fragment shader
out VS_OUT {
    vec3 N;
    vec3 SunL;
    vec3 PlayerL;
    vec3 V;
    vec3 F;
    vec2 texCoord;
} vs_out;

//
// Description : Array and textureless GLSL 2D/3D/4D simplex 
//               noise functions.
//      Author : Ian McEwan, Ashima Arts.
//  Maintainer : stegu
//     Lastmod : 20201014 (stegu)
//     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//               Distributed under the MIT License. See LICENSE file.
//               https://github.com/ashima/webgl-noise
//               https://github.com/stegu/webgl-noise
// 

vec3 mod289(vec3 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x) {
     return mod289(((x*34.0)+10.0)*x);
}

vec4 taylorInvSqrt(vec4 r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}

float snoise(vec3 v)
{ 
  const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
  const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

// First corner
  vec3 i  = floor(v + dot(v, C.yyy) );
  vec3 x0 =   v - i + dot(i, C.xxx) ;

// Other corners
  vec3 g = step(x0.yzx, x0.xyz);
  vec3 l = 1.0 - g;
  vec3 i1 = min( g.xyz, l.zxy );
  vec3 i2 = max( g.xyz, l.zxy );

  //   x0 = x0 - 0.0 + 0.0 * C.xxx;
  //   x1 = x0 - i1  + 1.0 * C.xxx;
  //   x2 = x0 - i2  + 2.0 * C.xxx;
  //   x3 = x0 - 1.0 + 3.0 * C.xxx;
  vec3 x1 = x0 - i1 + C.xxx;
  vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
  vec3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

// Permutations
  i = mod289(i); 
  vec4 p = permute( permute( permute( 
             i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
           + i.y + vec4(0.0, i1.y, i2.y, 1.0 )) 
           + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

// Gradients: 7x7 points over a square, mapped onto an octahedron.
// The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
  float n_ = 0.142857142857; // 1.0/7.0
  vec3  ns = n_ * D.wyz - D.xzx;

  vec4 j = p - 49.0 * floor(p * ns.z * ns.z);  //  mod(p,7*7)

  vec4 x_ = floor(j * ns.z);
  vec4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)

  vec4 x = x_ *ns.x + ns.yyyy;
  vec4 y = y_ *ns.x + ns.yyyy;
  vec4 h = 1.0 - abs(x) - abs(y);

  vec4 b0 = vec4( x.xy, y.xy );
  vec4 b1 = vec4( x.zw, y.zw );

  //vec4 s0 = vec4(lessThan(b0,0.0))*2.0 - 1.0;
  //vec4 s1 = vec4(lessThan(b1,0.0))*2.0 - 1.0;
  vec4 s0 = floor(b0)*2.0 + 1.0;
  vec4 s1 = floor(b1)*2.0 + 1.0;
  vec4 sh = -step(h, vec4(0.0));

  vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
  vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

  vec3 p0 = vec3(a0.xy,h.x);
  vec3 p1 = vec3(a0.zw,h.y);
  vec3 p2 = vec3(a1.xy,h.z);
  vec3 p3 = vec3(a1.zw,h.w);

//Normalise gradients
  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;

// Mix final noise value
  vec4 m = max(0.5 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
  m = m * m;
  return 105.0 * dot( m*m, vec4(dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3)));
}

float mix_at(float lo, float hi, float val)
{
  return (val - lo) / (hi - lo);
}

float noise_value_at(vec3 at)
{
  vec3 s1 = uniform_BaseSpatial;
  vec3 s2 = s1 * uniform_SpatialRatio;
  vec3 s3 = s2 * uniform_SpatialRatio;
  vec3 s4 = s3 * uniform_SpatialRatio;
  vec3 s5 = s4 * uniform_SpatialRatio;
  vec3 s6 = s5 * uniform_SpatialRatio;
  
  float a1 = uniform_AmplitudeRatio;
  float a2 = a1 * uniform_AmplitudeRatio;
  float a3 = a2 * uniform_AmplitudeRatio;
  float a4 = a3 * uniform_AmplitudeRatio;
  float a5 = a4 * uniform_AmplitudeRatio;
  float a6 = a5 * uniform_AmplitudeRatio;

  float b_amplitude = a1 + a2 + a3 + a4 + a5 + a6;

  // vec3 pos = attribute_Position + uniform_DistortionAmount * snoise(attribute_Position * uniform_DistortionSpatial + 9999.0f);
  vec3 pos = at + uniform_DistortionAmount * vec3(
    snoise(at * uniform_DistortionSpatial + 9999.0f),
    snoise(at * uniform_DistortionSpatial - 4444.0f),
    snoise(at * uniform_DistortionSpatial - 9999.0f)
  );

  float scale = b_amplitude
    + a1 * snoise(pos * s1 + 1234.0f)
    + a2 * snoise(pos * s2 + 4321.0f)
    + a3 * snoise(pos * s3 + 5678.0f)
    + a4 * snoise(pos * s4 + 8765.0f)
    + a5 * snoise(pos * s5 + 1111.0f)
    + a6 * snoise(pos * s6 + 3333.0f);
  // scale = scale / (b_amplitude + a1 + a2 + a3 + a4 + a5 + a6);
  scale = scale / (b_amplitude * 2);

  if(uniform_DoMakePoles)
  {
    float mixt = at.y * at.y;
    mixt = mixt * mixt;
    mixt = mixt * mixt;
    mixt = 1.0f - 1.5f * mixt;
    return scale * mixt;
  }
  else
  {
    return scale;
  }
}

// TODO benchmark this - is it really worth it?
float displacement_for_value(float val)
{
  if(noise_value < uniform_Threshold[4])
  {
    if(noise_value < uniform_Threshold[2])
    {
      if(noise_value < uniform_Threshold[1])
      {
        return mix(uniform_Height[0], uniform_Height[1], mix_at(uniform_Threshold[0], uniform_Threshold[1], val));
      }
      else
      {
        return mix(uniform_Height[1], uniform_Height[2], mix_at(uniform_Threshold[1], uniform_Threshold[2], val));
      }
    }
    else
    {
      if(noise_value < uniform_Threshold[3])
      {
        return mix(uniform_Height[2], uniform_Height[3], mix_at(uniform_Threshold[2], uniform_Threshold[3], val));
      }
      else
      {
        return mix(uniform_Height[3], uniform_Height[4], mix_at(uniform_Threshold[3], uniform_Threshold[4], val));
      }
    }
  }
  else
  {
    if(noise_value < uniform_Threshold[6])
    {
      if(noise_value < uniform_Threshold[5])
      {
        return mix(uniform_Height[4], uniform_Height[5], mix_at(uniform_Threshold[4], uniform_Threshold[5], val));
      }
      else
      {
        return mix(uniform_Height[5], uniform_Height[6], mix_at(uniform_Threshold[5], uniform_Threshold[6], val));
      }
    }
    else
    {
      if(noise_value < uniform_Threshold[7])
      {
        return mix(uniform_Height[6], uniform_Height[7], mix_at(uniform_Threshold[6], uniform_Threshold[7], val));
      }
      else
      {
        return mix(uniform_Height[7], uniform_Height[8], mix_at(uniform_Threshold[7], uniform_Threshold[8], val));
      }
    }
  }
}

void main()
{
  noise_value = noise_value_at(attribute_Position);
  float displacement = displacement_for_value(noise_value);
  // gl_Position = uP_m * uV_m * uM_m * vec4(attribute_Position * displacement, 1.0f);

  vec3 n;
  if(uniform_RecalcNormals)
  {
    // Checked -zxy to be safe: https://www.wolframalpha.com/input?i=x+-+z+%3D+t+*+x%2C+y+-+x+%3D+t+*+y%2C+z+-+y+%3D+t+*+z
    vec3 displacement_vec_x = attribute_Position * displacement;
    vec3 u = normalize(cross(displacement_vec_x, -displacement_vec_x.zxy)) * uniform_RecalcDist;
    vec3 v = normalize(cross(displacement_vec_x, u)) * uniform_RecalcDist;
    vec3 displacement_vec_u = (attribute_Position + u) * displacement_for_value(noise_value_at(attribute_Position + u));
    vec3 displacement_vec_v = (attribute_Position + v) * displacement_for_value(noise_value_at(attribute_Position + v));
    n = normalize(cross(displacement_vec_u - displacement_vec_x, displacement_vec_v - displacement_vec_x));
    n = n * sign(dot(displacement_vec_x, n));
  }
  else
  {
    n = attribute_Normal;
  }

  // Create Model-View matrix
  mat4 mv_m = uV_m * uM_m;

  // Calculate view-space coordinate - in P point 
  // we are computing the color
  vec4 P = mv_m * vec4(attribute_Position * displacement, 1.0f);

  // -- Light module
  // Calculate normal in view space
  vs_out.N = mat3(mv_m) * n;
  // Calculate view-space light vector
  vs_out.SunL = (uV_m * vec4(uniform_SunPos, 1.0)).xyz - P.xyz;
  // Calculate view-space light vector
  vs_out.PlayerL = (uV_m * vec4(uniform_PlayerPos, 1.0)).xyz - P.xyz;
  // Calculate view vector (negative of the view-space position)
  vs_out.V = -P.xyz;
  // Calculate player-forward in view-space TODO shouldn't this just be +z or -z
  vs_out.F = (uV_m * vec4(uniform_PlayerForward, 0.0)).xyz;
  // -- / Light module

  // Assigns the texture coordinates from the Vertex Data to "texCoord"
  vs_out.texCoord = attribute_TexCoord;

  // Calculate the clip-space position of each vertex
  gl_Position = uP_m * P;
}