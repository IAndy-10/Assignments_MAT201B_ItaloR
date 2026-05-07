#version 400

uniform vec4 lineColor;
layout(location = 0) out vec4 fragmentColor;

void main() {
  fragmentColor = lineColor;
}
