#version 330

in vec4 fragmentColor;
in vec4 vertexWorldPosition;
in vec3 vertexWorldNormal;

out vec4 finalColor;

uniform InputMaterial {
    float roughness;
    float specular;
    float metallic;
} material;

struct LightProperties {
    vec4 color;
};
uniform LightProperties genericLight;

struct PointLight {
    vec4 pointPosition;
    float radius;
};
uniform PointLight pointLight;

struct DirectionalLight {
    vec4 direction;
};
uniform DirectionalLight directionalLight;

struct HemisphereLight {
    vec4 csky;
    vec4 cground;
};
uniform HemisphereLight hemisphereLight;

uniform vec4 cameraPosition;

uniform float constantAttenuation;
uniform float linearAttenuation;
uniform float quadraticAttenuation;

uniform int lightingType;

const float PI = 3.1415926535897932384626433832795;

vec4 lightSubroutine(vec4 worldPosition, vec3 worldNormal)
{
    vec3 N = normalize(worldNormal);
    vec3 L = normalize(vec3(0));
    if (lightingType == 1) {
        L = normalize(pointLight.pointPosition.xyz - worldPosition.xyz);
    } else if (lightingType == 2) {
        L = normalize(-directionalLight.direction.xyz);
    } else if (lightingType == 3) {
        L = N;
    }
    vec3 V = normalize(cameraPosition.xyz - worldPosition.xyz);
    vec3 H = normalize(L+V);

    vec4 cdiff = (1-material.metallic) * fragmentColor;
    vec4 cspec = mix(vec4(0.08) * material.specular, fragmentColor, material.metallic);
    vec4 clight;
    if (lightingType != 3) {
        clight = genericLight.color;
    } else {
        clight = mix(hemisphereLight.cground, hemisphereLight.csky, clamp(dot(N, vec3(0, 1, 0)) * 0.5 + 0.5, 0.0, 1.0));
    }

    float a = material.roughness * material.roughness;
    vec4 d = cdiff / PI;
    float vh = dot(V, H);
    float nh2 = pow(dot(N, H), 2);
    float a2 = a*a;
    float D = a2 / (PI * pow(nh2*(a2-1)+1, 2));
    float k = pow(material.roughness + 1, 2) / 8.0;
    vec4 F = cspec + (1-cspec)*exp2((-5.55473*vh-6.98316)*vh);
    
    float nl = dot(N, L);
    float nv = dot(N, V);
    float G = (nl/(nl*(1-k)+k)) * (nv/(nv*(1-k)+k));

    vec4 s = D * F * G / (4 * nl * nv);
    
    return vec4(clight * nl * (d + s));
}

vec4 globalLightSubroutine(vec4 worldPosition, vec3 worldNormal)
{
    return vec4(0);
}

vec4 AttenuateLight(vec4 originalColor, vec4 worldPosition)
{
    if (lightingType != 1) {
        return originalColor;
    }
    float t = length(pointLight.pointPosition - worldPosition);
    float x = pow(clamp(1-pow(t/pointLight.radius, 4), 0, 1), 2);
    float y = constantAttenuation + linearAttenuation * t + quadraticAttenuation * t * t;
    return x / y * originalColor;
}

void main()
{
    vec4 lightingColor = vec4(0);
    if (lightingType == 0) {
        lightingColor = globalLightSubroutine(vertexWorldPosition, vertexWorldNormal);
    } else {
        lightingColor = lightSubroutine(vertexWorldPosition, vertexWorldNormal);
    }
    finalColor = AttenuateLight(lightingColor, vertexWorldPosition);
}
