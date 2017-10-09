#include "common/Rendering/Shaders/EpicShader.h"
#include "common/Rendering/Textures/Texture2D.h"
#include "common/Scene/Light/Light.h"
#include "common/Scene/Light/LightProperties.h"
#include "common/Scene/Light/EpicLightProperties.h"
#include "common/Scene/Camera/Camera.h"
#include "common/Utility/Texture/TextureLoader.h"
#include "assimp/material.h"

#define DISABLE_OPENGL_SUBROUTINES

std::array<const char*, 3> EpicShader::MATERIAL_PROPERTY_NAMES = {
    "InputMaterial.roughness", 
    "InputMaterial.specular", 
    "InputMaterial.metallic", 
};
const int EpicShader::MATERIAL_BINDING_POINT = 0;

EpicShader::EpicShader(const std::unordered_map<GLenum, std::string>& inputShaders, GLenum lightingStage):
    ShaderProgram(inputShaders), roughness(2.f), specular(0.f), metallic(0.f), ambient(glm::vec3(0.1f), 1.f), 
    materialBlockLocation(0), materialBlockSize(0), materialBuffer(0),
    lightingShaderStage(lightingStage), maxDisplacement(0.5f)
{
    if (!shaderProgram) {
        return;
    }

    SetupUniformBlock<3>("InputMaterial", MATERIAL_PROPERTY_NAMES, materialIndices, materialOffsets, materialStorage, materialBlockLocation, materialBlockSize, materialBuffer);
    UpdateMaterialBlock();

#ifdef DISABLE_OPENGL_SUBROUTINES
    (void)lightingShaderStage;
#endif

    defaultTexture = TextureLoader::LoadTexture("required/defaultTexture.png");
    if (!defaultTexture) {
        std::cerr << "WARNING: Failed to load the default texture." << std::endl;
        return;
    }
}

EpicShader::~EpicShader()
{
    OGL_CALL(glDeleteBuffers(1, &materialBuffer));
}

void EpicShader::SetupShaderLighting(const Light* light) const
{
    if (!light) {
#ifndef DISABLE_OPENGL_SUBROUTINES
        SetShaderSubroutine("inputLightSubroutine", "globalLightSubroutine", lightingShaderStage);
#else
        SetShaderUniform("lightingType", static_cast<int>(Light::LightType::GLOBAL));
#endif
    } else {
		const EpicLightProperties* lightProperties = static_cast<const EpicLightProperties*>(light->GetPropertiesRaw());
        // Select proper lighting subroutine based on the light's type.
        switch(light->GetLightType()) {
        case Light::LightType::POINT:
#ifndef DISABLE_OPENGL_SUBROUTINES
			SetShaderSubroutine("inputLightSubroutine", "pointLightSubroutine", lightingShaderStage);
#else
			SetShaderUniform("lightingType", static_cast<int>(Light::LightType::POINT));
			SetShaderUniform("pointLight.radius", lightProperties->radius);
#endif
			break;
		case Light::LightType::DIRECTIONAL:
#ifdef DISABLE_OPENGL_SUBROUTINES
			SetShaderUniform("lightingType", static_cast<int>(Light::LightType::DIRECTIONAL));
			SetShaderUniform("directionalLight.direction", light->GetForwardDirection());
#endif
			break;
		case Light::LightType::HEMISPHERE:
#ifdef DISABLE_OPENGL_SUBROUTINES
			SetShaderUniform("lightingType", static_cast<int>(Light::LightType::HEMISPHERE));
			SetShaderUniform("hemisphereLight.csky", lightProperties->skyColor);
			SetShaderUniform("hemisphereLight.cground", lightProperties->groundColor);
#endif
			break;
        default:
            std::cerr << "WARNING: Light type is not supported. Defaulting to global light. Your output may look wrong. -- Ignoring: " << static_cast<int>(light->GetLightType()) << std::endl;
#ifndef DISABLE_OPENGL_SUBROUTINES
            SetShaderSubroutine("inputLightSubroutine", "globalLightSubroutine", lightingShaderStage);
#else
            SetShaderUniform("lightingType", static_cast<int>(Light::LightType::GLOBAL));
#endif
            break;
        }

        SetShaderUniform("genericLight.color", lightProperties->diffuseColor);
        light->SetupShaderUniforms(this);
    }
    UpdateAttenuationUniforms(light);
}

void EpicShader::UpdateMaterialBlock() const
{
    StartUseShader();

    memcpy((void*)(materialStorage.data() + materialOffsets[0]), &roughness, sizeof(float));
    memcpy((void*)(materialStorage.data() + materialOffsets[1]), &specular, sizeof(float));
    memcpy((void*)(materialStorage.data() + materialOffsets[2]), &metallic, sizeof(float));

    if (materialBuffer && materialBlockLocation != GL_INVALID_INDEX) {
        OGL_CALL(glBindBuffer(GL_UNIFORM_BUFFER, materialBuffer));
        OGL_CALL(glBufferData(GL_UNIFORM_BUFFER, materialBlockSize, materialStorage.data(), GL_STATIC_DRAW));
        OGL_CALL(glBindBufferBase(GL_UNIFORM_BUFFER, MATERIAL_BINDING_POINT, materialBuffer));
        OGL_CALL(glUniformBlockBinding(shaderProgram, materialBlockLocation, MATERIAL_BINDING_POINT));
        OGL_CALL(glBindBuffer(GL_UNIFORM_BUFFER, 0));
    }

    StopUseShader();
}

void EpicShader::UpdateAttenuationUniforms(const Light* light) const
{
    float constant = 1.f, linear = 0.f, quadratic = 0.f;
    if (light) {
        light->GetAttenuation(constant, linear, quadratic);
    }

    SetShaderUniform("constantAttenuation", constant);
    SetShaderUniform("linearAttenuation", linear);
    SetShaderUniform("quadraticAttenuation", quadratic);
}

void EpicShader::SetupShaderMaterials() const
{
    // Need to make sure the material buffer is bound.
    OGL_CALL(glBindBufferBase(GL_UNIFORM_BUFFER, MATERIAL_BINDING_POINT, materialBuffer));

    // Make sure the right textures are bound.
    const Texture* diffuseTexture = defaultTexture.get();
    if (textureSlotMapping.find(TextureSlots::DIFFUSE) != textureSlotMapping.end()) {
        diffuseTexture = textureSlotMapping.at(TextureSlots::DIFFUSE).get();
    }
    assert(diffuseTexture);
    diffuseTexture->BeginRender(static_cast<int>(TextureSlots::DIFFUSE));

    const Texture* specularTexture = defaultTexture.get();
    if (textureSlotMapping.find(TextureSlots::SPECULAR) != textureSlotMapping.end()) {
        specularTexture = textureSlotMapping.at(TextureSlots::SPECULAR).get();
    }
    assert(specularTexture);
    specularTexture->BeginRender(static_cast<int>(TextureSlots::SPECULAR));
    
    if (textureSlotMapping.find(TextureSlots::NORMAL) != textureSlotMapping.end()) {
        const Texture* normalTexture = textureSlotMapping.at(TextureSlots::NORMAL).get();
        normalTexture->BeginRender(static_cast<int>(TextureSlots::NORMAL));
        SetShaderUniform("useNormalTexture", (int)true);
    } else {
        SetShaderUniform("useNormalTexture", (int)false);
    }

    if (textureSlotMapping.find(TextureSlots::DISPLACEMENT) != textureSlotMapping.end()) {
        const Texture* displacementTexture = textureSlotMapping.at(TextureSlots::DISPLACEMENT).get();
        displacementTexture->BeginRender(static_cast<int>(TextureSlots::DISPLACEMENT));
        SetShaderUniform("useDisplacementTexture", (int)true);
    } else {
        SetShaderUniform("useDisplacementTexture", (int)false);
    }

    // While we're here, also setup the textures too.
    SetShaderUniform("diffuseTexture", static_cast<int>(TextureSlots::DIFFUSE));
    SetShaderUniform("specularTexture", static_cast<int>(TextureSlots::SPECULAR));
    SetShaderUniform("normalTexture", static_cast<int>(TextureSlots::NORMAL));
    SetShaderUniform("displacementTexture", static_cast<int>(TextureSlots::DISPLACEMENT));
    SetShaderUniform("maxDisplacement", maxDisplacement);
}

void EpicShader::SetupShaderCamera(const class Camera* camera) const
{
    SetShaderUniform("cameraPosition", camera->GetPosition());
}

void EpicShader::SetRoughness(float inRoughness) 
{ 
    roughness = inRoughness;
    UpdateMaterialBlock();
}

void EpicShader::SetSpecular(float inSpecular) 
{ 
    specular = inSpecular; 
    UpdateMaterialBlock();
}

void EpicShader::SetMetallic(float inMetallic)
{
	metallic = inMetallic;
	UpdateMaterialBlock();
}

void EpicShader::SetAmbient(glm::vec4 inAmbient) 
{ 
    ambient = inAmbient; 
    UpdateMaterialBlock();
}

void EpicShader::SetTexture(TextureSlots::Type slot, std::shared_ptr<class Texture> inputTexture)
{
    textureSlotMapping[slot] = std::move(inputTexture);
}

void EpicShader::SetMaxDisplacement(float input)
{
    maxDisplacement = input;
}

void EpicShader::LoadMaterialFromAssimp(std::shared_ptr<aiMaterial> assimpMaterial)
{
    if (!assimpMaterial) {
        return;
    }

    assimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, &roughness, nullptr);
    assimpMaterial->Get(AI_MATKEY_COLOR_SPECULAR, &specular, nullptr);
    assimpMaterial->Get(AI_MATKEY_SHININESS, &metallic, nullptr);
    assimpMaterial->Get(AI_MATKEY_COLOR_AMBIENT, glm::value_ptr(ambient), nullptr);

    if (assimpMaterial->GetTextureCount(aiTextureType_DIFFUSE)) {
        aiString aiDiffusePath;
        assimpMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &aiDiffusePath);
        std::string diffusePath(aiDiffusePath.C_Str());
        SetTexture(TextureSlots::DIFFUSE, TextureLoader::LoadTexture(diffusePath));
    }

    if (assimpMaterial->GetTextureCount(aiTextureType_SPECULAR)) {
        aiString aiSpecularPath;
        assimpMaterial->GetTexture(aiTextureType_SPECULAR, 0, &aiSpecularPath);
        std::string specularPath(aiSpecularPath.C_Str());
        SetTexture(TextureSlots::SPECULAR, TextureLoader::LoadTexture(specularPath));
    }

    UpdateMaterialBlock();
}