#version 450

layout(location = 0) in vec2 inUV;

layout(set = 3, binding = 0) uniform UniformBufferObject
{
    float time;
    float speed;
    float frequency;
    float amplitude;
} ubo;

layout(set = 2, binding = 0) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

// thanks to https://www.shadertoy.com/view/Mls3DH
vec2 shift(vec2 p)
{
    float d = ubo.time * ubo.speed;
    vec2 f = ubo.frequency * (p + d);
    vec2 q = cos(vec2(
            cos(f.x - f.y) * cos(f.y),
            sin(f.x + f.y) * sin(f.y)
        )
    );
    return q;                                  
} 

void main()
{
    vec2 r = inUV;
	vec2 p = shift(r);
    vec2 q = shift(r + 1.0);
    vec2 s = r + ubo.amplitude * (p - q);

    outColor = texture(texSampler, s);
}