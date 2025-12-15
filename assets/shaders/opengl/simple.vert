#version 460

struct Vertex
{
    vec2 position;
};

const Vertex vertices[3] = Vertex[](
    Vertex(vec2( 0.0,  0.5)),
    Vertex(vec2( 0.5, -0.5)),
    Vertex(vec2(-0.5, -0.5))
);

void main()
{
    gl_Position = vec4(vertices[gl_VertexID].position, 0.0, 1.0);
}