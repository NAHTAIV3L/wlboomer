#version 330 core

uniform sampler2D image;
uniform vec2 in_cursor;
uniform vec2 res;
uniform float scale;
uniform float fl_radius;
uniform float fl_shadow;

in vec2 out_uv;

void main()
{
    vec4 cursor = vec4(in_cursor.x, (res.y - in_cursor.y), 0, 1);
    gl_FragColor = mix(
        texture(image, out_uv), vec4(0.0, 0.0, 0.0, 1.0),
        ((distance(cursor, gl_FragCoord) < (fl_radius * scale)) ? 0.0 : fl_shadow));
}
