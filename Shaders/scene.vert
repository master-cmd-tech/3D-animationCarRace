#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;
uniform mat4 model, view, projection;
uniform mat3 normalMatrix;
uniform bool flatPass;
out vec3 positionView, normalView, baseColor, gouraudColor;

uniform vec3 light1DirectionView, light2PositionView;
uniform vec3 materialAmbient, materialDiffuse, materialSpecular;
uniform float materialShininess;
uniform vec3 lightAmbient, lightDiffuse, lightSpecular;
uniform bool vertexColors;
// Course Phong model: ambient + diffuse + reflected-vector specular.
vec3 calculateLight(vec3 fragPosition, vec3 normal, vec3 lightPosition, vec3 diffuseColor,
                    bool directional)
{
    vec3 N = normalize(normal);
    vec3 L = directional ? normalize(lightPosition) : normalize(lightPosition - fragPosition);
    vec3 V = normalize(-fragPosition);
    vec3 R = reflect(-L, N);
    float diffuse = max(dot(N, L), 0.0);
    float specular = 0.0;
    if (diffuse > 0.0)
    {
        specular = pow(max(dot(V, R), 0.0), materialShininess);
    }
    vec3 ambientColor = vertexColors ? diffuseColor * .28 : materialAmbient;
    return ambientColor * lightAmbient + diffuseColor * lightDiffuse * diffuse +
           materialSpecular * lightSpecular * specular;
}

void main()
{
    vec4 position = view * model * vec4(aPosition, 1.0);
    positionView = position.xyz;
    normalView = normalize(normalMatrix * aNormal);
    baseColor = vertexColors ? aColor : materialDiffuse;
    gouraudColor =
        flatPass
            ? baseColor
            : calculateLight(positionView, normalView, light1DirectionView, baseColor, true) +
                  calculateLight(positionView, normalView, light2PositionView, baseColor, false);
    gl_Position = projection * position;
}
