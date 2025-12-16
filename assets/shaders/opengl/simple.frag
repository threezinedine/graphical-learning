#version 460

layout(binding = 0) uniform sampler2D uTexture;

layout(location = 0) in vec3 vColor;
layout(location = 1) in vec2 vTexCoord;

out vec4 fragColor;

void main()
{
#if 1
    vec4 texColor = texture(uTexture, vTexCoord);
    if (texColor.r < 0.1)
    {
        fragColor = vec4(vColor, 1.0);
        return;
    }
    fragColor = vec4(vColor, 1.0) * texColor;
#else
    fragColor = vec4(vColor, 1.0);
#endif
}