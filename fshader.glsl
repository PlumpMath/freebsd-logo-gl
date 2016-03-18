uniform sampler2D texture;
varying mediump float v_angle;
uniform lowp vec4 sourceColor;
uniform lowp float alpha;
varying vec2 v_texcoord;

void main(void)
{
    vec4 color = texture2D(texture, v_texcoord);
    // vec3 col = (t_color.rgb*t_color.a + sourceColor.rgb*sourceColor.a)/2.0;
    if (color.a == 0.0)
        color = sourceColor;
    else
        color.a = alpha;
    color = vec4(color.rgb * 0.2 + color.rgb * 0.8 * v_angle, color.a);
    color = clamp(color, 0.0, 1.0);

    gl_FragColor = color;
}
