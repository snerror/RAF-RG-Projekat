#version 330 core
out vec4 FragColor;

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos;

uniform vec3 viewPos;
uniform Light light;

void main()
{
    vec3 ambient = light.ambient;
    FragColor = vec4(ambient, 1.0);
}