#version 330 core
out vec4 fragmentColor;

in vec3 fragmentPosition;
in vec3 fragmentVertexNormal;
in vec2 fragmentTextureCoordinate;

struct Material {
    vec3 diffuseColor;
    vec3 specularColor;
    float shininess;
};

struct DirectionalLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    bool bActive;
};

struct PointLight {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    bool bActive;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    bool bActive;
};

#define TOTAL_POINT_LIGHTS 5

uniform bool bUseTexture=false;
uniform bool bUseLighting=false;
uniform vec4 objectColor = vec4(1.0f);
uniform vec3 viewPosition;
uniform DirectionalLight directionalLight;
uniform PointLight pointLights[TOTAL_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform Material material;
uniform sampler2D objectTexture;
uniform vec2 UVscale = vec2(1.0f, 1.0f);

// Shadow mapping (spotlight)
uniform sampler2D spotShadowMap;
uniform mat4 spotLightSpaceMatrix;
uniform int spotShadowMapTextureUnit = 7; // default unit; app will bind accordingly

// Liquid ripple uniforms
uniform bool bIsLiquidSurface = false;
uniform float timeSeconds = 0.0;
uniform vec2 rippleParams = vec2(4.0, 12.0); // x: speed, y: radial frequency
uniform float rippleAmplitude = 0.04;

// function prototypes
vec3 CalcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
float CalcSpotShadow(vec3 fragPos, vec3 normal, vec3 lightDir);

void main()
{
    // base normal
    vec3 norm = normalize(fragmentVertexNormal);

    // optional ripple normal perturbation for liquid surface
    if (bIsLiquidSurface)
    {
        // center UVs around 0.5 and compute radial distance
        vec2 centered = (fragmentTextureCoordinate - vec2(0.5)) * UVscale;
        float r = length(centered);
        // radial wave: concentric rings emanating from center
        float wave = sin(r * rippleParams.y - timeSeconds * rippleParams.x);
        // perturb along the local tangent direction to suggest circular ripple normals
        vec2 tangent = normalize(vec2(-centered.y, centered.x) + 1e-6);
        vec3 rippleN = vec3(tangent.x, 0.0, tangent.y) * (wave * rippleAmplitude);
        norm = normalize(norm + rippleN);
    }

    if(bUseLighting == true)
    {
        vec3 phongResult = vec3(0.0f);
        // properties
        vec3 viewDir = normalize(viewPosition - fragmentPosition);

        // == =====================================================
        // Our lighting is set up in 3 phases: directional, point lights and an optional flashlight
        // For each phase, a calculate function is defined that calculates the corresponding color
        // per light source. In the main() function we take all the calculated colors and sum them
        // up for this fragment's final color.
        // == =====================================================
        // phase 1: directional lighting
        if(directionalLight.bActive == true)
        {
            phongResult += CalcDirectionalLight(directionalLight, norm, viewDir);
        }
        // phase 2: point lights
        for(int i = 0; i < TOTAL_POINT_LIGHTS; i++)
        {
            if(pointLights[i].bActive == true)
            {
                phongResult += CalcPointLight(pointLights[i], norm, fragmentPosition, viewDir);
            }
        }
        // phase 3: spot light
        if(spotLight.bActive == true)
        {
            phongResult += CalcSpotLight(spotLight, norm, fragmentPosition, viewDir);
        }

        if(bUseTexture == true)
        {
            fragmentColor = vec4(phongResult, texture(objectTexture, fragmentTextureCoordinate * UVscale).a);
        }
        else
        {
            fragmentColor = vec4(phongResult, objectColor.a);
        }
    }
    else
    {
        if(bUseTexture == true)
        {
            // base textured color
            vec4 baseTex = texture(objectTexture, fragmentTextureCoordinate * UVscale);
            // apply subtle shimmer when liquid is active
            if (bIsLiquidSurface)
            {
                // radial shimmer tied to concentric ripples
                vec2 centered = (fragmentTextureCoordinate - vec2(0.5)) * UVscale;
                float r = length(centered);
                float wave = sin(r * rippleParams.y - timeSeconds * rippleParams.x);
                float shimmer = 0.95 + 0.05 * wave;
                baseTex.rgb *= shimmer;
            }
            // apply subtle meniscus effect near edge to make liquid feel rounded
            if (bIsLiquidSurface)
            {
                // treat UV as radial domain around center (0.5, 0.5). Adjust if needed.
                vec2 centered = (fragmentTextureCoordinate - vec2(0.5)) * UVscale;
                float r = length(centered);
                // meniscus band adjusted to enlarge center opening
                float edgeStart = 0.5;  // moved inward to enlarge inner opening
                float edgeWidth = 0.5;  // wider band for a more pronounced rim
                float edgeT = smoothstep(edgeStart, edgeStart + edgeWidth, r);
                // darken slightly toward edge, then small bright band at the rim
                float darken = mix(1.0, 0.92, edgeT);
                float highlight = smoothstep(edgeStart + edgeWidth * 0.7, edgeStart + edgeWidth, r) * 0.08;
                baseTex.rgb = baseTex.rgb * darken + highlight;
            }
            fragmentColor = baseTex;
        }
        else
        {
            vec4 baseCol = objectColor;
            // apply subtle shimmer to solid color if liquid
            if (bIsLiquidSurface)
            {
                // radial shimmer tied to concentric ripples
                vec2 centered = (fragmentTextureCoordinate - vec2(0.5)) * UVscale;
                float r = length(centered);
                float wave = sin(r * rippleParams.y - timeSeconds * rippleParams.x);
                float shimmer = 0.95 + 0.05 * wave;
                baseCol.rgb *= shimmer;
            }
            // apply subtle meniscus effect near edge to make liquid feel rounded
            if (bIsLiquidSurface)
            {
                // treat UV as radial domain around center (0.5, 0.5). Adjust if needed.
                vec2 centered = (fragmentTextureCoordinate - vec2(0.5)) * UVscale;
                float r = length(centered);
                // meniscus band adjusted to enlarge center opening
                float edgeStart = 0.38;  // moved inward to enlarge inner opening
                float edgeWidth = 0.10;  // wider band for a more pronounced rim
                float edgeT = smoothstep(edgeStart, edgeStart + edgeWidth, r);
                // darken slightly toward edge, then small bright band at the rim
                float darken = mix(1.0, 0.92, edgeT);
                float highlight = smoothstep(edgeStart + edgeWidth * 0.7, edgeStart + edgeWidth, r) * 0.08;
                baseCol.rgb = baseCol.rgb * darken + highlight;
            }
            fragmentColor = baseCol;
        }
    }
}

// calculates the color when using a directional light.
vec3 CalcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir)
{
    vec3 ambient = vec3(0.0f);
    vec3 diffuse = vec3(0.0f);
    vec3 specular = vec3(0.0f);

    vec3 lightDirection = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDirection), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDirection, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // combine results
    if(bUseTexture == true)
    {
        ambient = light.ambient * vec3(texture(objectTexture, fragmentTextureCoordinate * UVscale));
        diffuse = light.diffuse * diff * material.diffuseColor * vec3(texture(objectTexture, fragmentTextureCoordinate * UVscale));
        specular = light.specular * spec * material.specularColor * vec3(texture(objectTexture, fragmentTextureCoordinate * UVscale));
    }
    else
    {
        ambient = light.ambient * vec3(objectColor);
        diffuse = light.diffuse * diff * material.diffuseColor * vec3(objectColor);
        specular = light.specular * spec * material.specularColor * vec3(objectColor);
    }

    return (ambient + diffuse + specular);
}

// calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 ambient = vec3(0.0f);
    vec3 diffuse = vec3(0.0f);
    vec3 specular= vec3(0.0f);

    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    // Calculate specular component
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    // combine results
    if(bUseTexture == true)
    {
        ambient = light.ambient * vec3(texture(objectTexture, fragmentTextureCoordinate * UVscale));
        diffuse = light.diffuse * diff * material.diffuseColor * vec3(texture(objectTexture, fragmentTextureCoordinate * UVscale));
        specular = light.specular * specularComponent * material.specularColor;
    }
    else
    {
        ambient = light.ambient * vec3(objectColor);
        diffuse = light.diffuse * diff * material.diffuseColor * vec3(objectColor);
        specular = light.specular * specularComponent * material.specularColor;
    }

    return (ambient + diffuse + specular);
}

// calculates the color when using a spot light.
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 ambient = vec3(0.0f);
    vec3 diffuse = vec3(0.0f);
    vec3 specular = vec3(0.0f);

    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    // shadow factor for spotlight (attenuates only diffuse/specular)
    float shadow = CalcSpotShadow(fragPos, normal, lightDir);
    // combine results (apply shadow only to direct lighting terms)
    if(bUseTexture == true)
    {
        ambient = light.ambient * vec3(texture(objectTexture, fragmentTextureCoordinate * UVscale));
        diffuse = (1.0 - shadow) * light.diffuse * diff * material.diffuseColor * vec3(texture(objectTexture, fragmentTextureCoordinate * UVscale));
        specular = (1.0 - shadow) * light.specular * spec * material.specularColor * vec3(texture(objectTexture, fragmentTextureCoordinate * UVscale));
    }
    else
    {
        ambient = light.ambient * vec3(objectColor);
        diffuse = (1.0 - shadow) * light.diffuse * diff * material.diffuseColor * vec3(objectColor);
        specular = (1.0 - shadow) * light.specular * spec * material.specularColor * vec3(objectColor);
    }

    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (ambient + diffuse + specular);
}

// Shadow calculation for spotlight using 2D depth map with simple PCF
float CalcSpotShadow(vec3 fragPos, vec3 normal, vec3 lightDir)
{
    // transform fragment position to light clip space
    vec4 fragPosLightSpace = spotLightSpaceMatrix * vec4(fragPos, 1.0);
    // perform perspective divide to get NDC
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1]
    projCoords = projCoords * 0.5 + 0.5;

    // if outside shadow map bounds or beyond far plane, consider lit
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    // bias to reduce shadow acne; slope-scaled using normal vs light
    float bias = max(0.0008 * (1.0 - dot(normalize(normal), normalize(lightDir))), 0.0002);

    // Percentage-closer filtering (3x3 kernel)
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(spotShadowMap, 0));
    for (int x = -2; x <= 2; ++x)
    {
        for (int y = -2; y <= 2; ++y)
        {
            float pcfDepth = texture(spotShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (projCoords.z - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 25.0;
    return shadow;
}
