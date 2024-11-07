#version 330 core
layout(location = 0) in vec2 inPosition;
uniform vec2 position;
uniform float angle;
void main() {
  mat2 rotation = mat2(
    cos(angle), -sin(angle),
    sin(angle),  cos(angle)
  );
  vec2 rotatedPosition = rotation * inPosition;
  gl_Position = vec4(rotatedPosition + position, 0.0, 1.0);
}