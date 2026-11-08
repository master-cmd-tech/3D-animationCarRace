#version 330 core
in vec3 positionView, normalView, baseColor, gouraudColor;
out vec4 FragColor;
uniform int shadingMode;
uniform float objectAlpha;
uniform bool flatPass;
uniform vec4 flatColor;

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
    if (flatPass)
    {
        FragColor = vec4(baseColor, 1.0) * flatColor;
        return;
    }
    vec3 result = gouraudColor;
    if (shadingMode == 1)
    {
        result = calculateLight(positionView, normalView, light1DirectionView, baseColor, true) +
                 calculateLight(positionView, normalView, light2PositionView, baseColor, false);
    }
    FragColor = vec4(clamp(result, 0.0, 1.0), objectAlpha);
}
