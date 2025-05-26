#version 460 core

in float noise_value;

out vec4 FragColor;

void main()
{
    float scale = noise_value;
    if(scale < 0.55f)
    {
      if(scale < 0.2f)
      {
        FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
      }
      else if(scale < 0.5f)
      {
        FragColor = vec4(0.0f, 0.0f, 1.0f, 1.0f);
      }
      else
      {
        FragColor = vec4(1.0f, 1.0f, 0.0f, 1.0f);
      }
    }
    else
    {
      if(scale < 0.65f)
      {
        FragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
      }
      else
      {
        FragColor = vec4(0.5f, 0.5f, 0.5f, 0.5f);
      }
    }
}