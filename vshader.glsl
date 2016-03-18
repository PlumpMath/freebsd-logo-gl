attribute highp vec4 vertex;
attribute mediump vec3 normal;
uniform mediump mat4 matrix;
varying mediump float v_angle;

// texture bits
attribute vec2 texcoord;
varying vec2 v_texcoord;

void main(void)
{
    vec4 translated_normal = matrix * vec4(normal, 0);
    vec3 toLight = normalize(vec3(0.5, -1.0, 1.0));
    v_angle = max(dot(translated_normal.xyz, toLight), 0.0);
    v_texcoord = texcoord;
    gl_Position = matrix * vertex;
}
