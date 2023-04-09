#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform sampler2D texture1;
uniform vec3 viewPosition;
uniform DirLight dirLight;

vec4 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
//     vec3 reflectDir = reflect(-lightDir, normal);
//     float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    vec4 texColor = vec4(texture(texture1, TexCoords));
    if(texColor.a < 0.1)
        discard;

    // combine results
    vec4 ambient = vec4(light.ambient, 1.0) * texColor;
    vec4 diffuse = vec4(light.diffuse, 1.0) * diff * texColor;
    vec4 specular = vec4(light.specular, 1.0) * spec * texColor;


    return (ambient + diffuse + specular);
}

void main()
{
    vec3 normal = vec3(0.0f, 1.0f, 0.0f);
    vec3 viewDir = normalize(viewPosition - FragPos);
    vec4 result = CalcDirLight(dirLight, normal, viewDir);
    FragColor = result;
}