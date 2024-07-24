#version 330 core

uniform sampler2D image;
uniform vec2 in_cursor;
uniform vec2 res;
uniform float scale;
uniform float fl_radius;
uniform float fl_shadow;
uniform bool fl_snazzy;

in vec2 out_uv;

void main()
{
    vec4 cursor = vec4(in_cursor.x, (res.y - in_cursor.y), 0, 1);
    vec4 image_mapped = texture(image, out_uv);
    float mix_factor = (distance(cursor, gl_FragCoord) < (fl_radius * scale)) ? 0.0 : (fl_snazzy ? ((distance(cursor, gl_FragCoord) - (fl_radius * scale)) / ((fl_radius + 1) * scale) * fl_shadow) : fl_shadow);
    gl_FragColor = mix(image_mapped, vec4(0.0, 0.0, 0.0, 1.0), mix_factor);
}
