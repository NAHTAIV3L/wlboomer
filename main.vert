#version 330 core

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 uv;

uniform float scale;
uniform vec2 camera;
uniform vec2 res;
uniform vec2 in_cursor;

out vec2 out_uv;

vec2 project(vec2 point) {
    vec2 ratio = vec2(1.0 / scale, 1.0 / scale);
    return (point * 2.0 - 1.0) / ratio;
}

void main() {
    out_uv = uv;
    vec2 new_pos = pos - ((camera / res) * vec2(1.0, -1.0));
    gl_Position = vec4(project(new_pos), 0.0, 1.0);
}
