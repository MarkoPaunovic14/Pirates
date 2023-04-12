#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 TangentLightPos1;
    vec3 TangentViewPos1;
    vec3 TangentFragPos1;
    vec3 TangentLightPos2;
    vec3 TangentViewPos2;
    vec3 TangentFragPos2;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform vec3 lightPos1;
uniform vec3 lightPos2;
uniform vec3 viewPos;

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));   
    vs_out.TexCoords = aTexCoords;
    
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = normalize(normalMatrix * aNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    
    mat3 TBN1 = transpose(mat3(T, B, N));
    vs_out.TangentLightPos1 = TBN1 * lightPos1;
    vs_out.TangentViewPos1  = TBN1 * viewPos;
    vs_out.TangentFragPos1  = TBN1 * vs_out.FragPos;

    mat3 TBN2 = transpose(mat3(T, B, N));
    vs_out.TangentLightPos2 = TBN2 * lightPos2;
    vs_out.TangentViewPos2  = TBN2 * viewPos;
    vs_out.TangentFragPos2  = TBN2 * vs_out.FragPos;
        
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}