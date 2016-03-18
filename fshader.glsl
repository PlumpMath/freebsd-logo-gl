uniform sampler2D texture;
varying mediump float v_angle;
uniform lowp vec4 sourceColor;
varying vec2 v_texcoord;

void main(void)
{
    vec4 t_color = texture2D(texture, v_texcoord);
    // vec3 col = (t_color.rgb*t_color.a + sourceColor.rgb*sourceColor.a)/2.0;
    vec3 col = t_color.rgb;
    if (t_color.a == 0.0)
        col = sourceColor.rgb;
    vec4 color = vec4(col * 0.2 + col * 0.8 * v_angle, 1.0);
    color = clamp(color, 0.0, 1.0);

    gl_FragColor = color;
}
