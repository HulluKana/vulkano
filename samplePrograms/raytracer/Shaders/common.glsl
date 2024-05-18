struct payload {
    vec4 hitValue;
    vec3 emission;
    vec3 pos;
};

vec3 sRGBToAlbedo(vec3 sRGB)
{
    const vec3 prePow = (sRGB + vec3(0.055)) / 1.055;
    return vec3(pow(prePow.x, 2.4), pow(prePow.y, 2.4), pow(prePow.z, 2.4));
}

vec3 albedoToSRGB(vec3 albedo)
{
    const vec3 prePow = albedo * 1.055;
    return vec3(pow(prePow.x, 0.41667), pow(prePow.y, 0.41667), pow(prePow.z, 0.41167)) - vec3(0.055);
}

float sRGBToAlbedo(float sRGB)
{
    const float prePow = (sRGB + 0.055) / 1.055;
    return pow(prePow, 2.4);
}

float albedoToSRGB(float albedo)
{
    const float prePow = albedo * 1.055;
    return pow(prePow, 0.41667) - 0.055;
}
