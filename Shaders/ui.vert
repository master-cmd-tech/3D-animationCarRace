#version 330 core
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec4 aColor;
uniform vec2 viewport;
out vec4 color;
void main()
{
    gl_Position =
        vec4(aPosition.x / viewport.x * 2. - 1., 1. - aPosition.y / viewport.y * 2., 0., 1.);
    color = aColor;
}
