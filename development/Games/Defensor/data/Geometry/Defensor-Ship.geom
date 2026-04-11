// vertex formats specify order of Vertex layout
VertexFormat: VertexPosition | VertexColor | VertexTexture0

Vertices Begin:
    { -0.04f,  0.1f , 0.5f }; { 1.0f, 1.0f, 1.0f, 1.0f }; { 0.0f, 0.0f }
    {  0.04f,  0.1f , 0.5f }; { 1.0f, 1.0f, 1.0f, 1.0f }; { 1.0f, 0.0f }
    {  0.04f, -0.1f , 0.5f }; { 1.0f, 1.0f, 1.0f, 1.0f }; { 1.0f, 1.0f }
    { -0.04f, -0.1f , 0.5f }; { 1.0f, 1.0f, 1.0f, 1.0f }; { 0.0f, 1.0f }
Vertices End:


Indices Begin:
    0, 1, 2
    0, 2, 3
Indices End:
