#version 330 core

in vec2 textureCoordinate;
out float ambientOcclusion;

uniform sampler2D ssaoInput;

void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(ssaoInput, 0));
    float result = 0.0;
    for (int x = -2; x <= 2; ++x)
    {
        for (int y = -2; y <= 2; ++y)
        {
            result += texture(
                ssaoInput,
                textureCoordinate + vec2(float(x), float(y)) * texelSize).r;
        }
    }
    ambientOcclusion = result / 25.0;
}
