#pragma once

const char* VERTEX_SHADER = CI_GLSL(150,
    uniform mat4    ciModelViewProjection;
    uniform mat3    uStateTransform; 
    in vec2         ciPosition;

    void main(void)
    {
        vec3 p = uStateTransform * vec3(ciPosition, 1.0);
        gl_Position = ciModelViewProjection * vec4(p, 1.0);
    });                                        

const char* GEOMETRY_SHADER  = CI_GLSL(150,
    uniform float uCellWidth;
    uniform float uCellHeight;

    layout(points) in;
    layout(triangle_strip, max_vertices = 5) out;

    uniform mat4    ciModelViewProjection;

    void main(void)
    {
        gl_Position = gl_in[0].gl_Position;
        EmitVertex();

        vec4 offset = vec4(uCellWidth, 0.0, 0.0, 0.0);
        gl_Position = gl_in[0].gl_Position + offset;
        EmitVertex();

        offset = vec4(uCellWidth, uCellHeight, 0.0, 0.0);
        gl_Position = gl_in[0].gl_Position + offset;
        EmitVertex();

        offset = vec4(0.0, uCellHeight, 0.0, 0.0);
        gl_Position = gl_in[0].gl_Position + offset;
        EmitVertex();

        gl_Position = gl_in[0].gl_Position;
        EmitVertex();

        EndPrimitive();
    });

const char* FRAGMENT_SHADER = CI_GLSL(150,
    out vec4 oColor;

    uniform vec2  uWindowRes;
    uniform float uElapsedTime;

    void main(void)
    {
        vec2 uv = gl_FragCoord.xy / uWindowRes;
        oColor = vec4(uv, 0.5 + 0.5 * sin(uElapsedTime), 1.0);
    });