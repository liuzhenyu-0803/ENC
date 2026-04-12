#version 450

layout(location = 0) in vec4 frag_color;

layout(location = 0) out vec4 out_color;

void main() {
  vec2 centered = gl_PointCoord * 2.0 - 1.0;
  if (dot(centered, centered) > 1.0) {
    discard;
  }
  out_color = frag_color;
}
