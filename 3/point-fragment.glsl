#version 400

in Fragment {
  vec4 color;
  vec2 mapping;
}
fragment;

layout(location = 0) out vec4 fragmentColor;

void main() {
  float r = length(fragment.mapping);
  if (r > 1) discard;
  float alpha = exp(-r * r * 4.0);
  fragmentColor = vec4(fragment.color.rgb, alpha);
}
