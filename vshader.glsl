attribute highp vec4 vertex;
attribute mediump vec3 normal;
uniform mediump mat4 matrix;
uniform lowp vec4 sourceColor;
varying mediump vec4 color;

void main(void)
{
    vec4 translated_normal = matrix * vec4(normal, 0);
    vec3 toLight = normalize(vec3(0.5, -1.0, 1.0));
    float angle = max(dot(translated_normal.xyz, toLight), 0.0);
    vec3 col = sourceColor.rgb;
    color = vec4(col * 0.2 + col * 0.8 * angle, 1.0);
    color = clamp(color, 0.0, 1.0);
    gl_Position = matrix * vertex;
}

