#include "UnifiedMaterialConstants.h"

namespace AstralEngine {

    UnifiedMaterialUBO MaterialTemplate::applyTemplate(const UnifiedMaterialUBO& instanceParams) const {
        UnifiedMaterialUBO result = instanceParams;
        
        // Apply locked parameters from template
        if (isParameterLocked(LOCK_BASE_COLOR)) {
            result.baseColor = defaultParameters.baseColor;
        }
        
        if (isParameterLocked(LOCK_METALLIC)) {
            result.metallic = defaultParameters.metallic;
        }
        
        if (isParameterLocked(LOCK_ROUGHNESS)) {
            result.roughness = defaultParameters.roughness;
        }
        
        if (isParameterLocked(LOCK_NORMAL_SCALE)) {
            result.getNormalScale() = defaultParameters.getNormalScale();
        }
        
        if (isParameterLocked(LOCK_EMISSIVE)) {
            result.emissiveColor = defaultParameters.emissiveColor;
        }
        
        if (isParameterLocked(LOCK_TEXTURE_TILING)) {
            result.baseColorTilingOffset = defaultParameters.baseColorTilingOffset;
            result.normalTilingOffset = defaultParameters.normalTilingOffset;
            result.metallicRoughnessTilingOffset = defaultParameters.metallicRoughnessTilingOffset;
            result.emissiveTilingOffset = defaultParameters.emissiveTilingOffset;
        }
        
        if (isParameterLocked(LOCK_ANISOTROPY)) {
            result.anisotropyParams = defaultParameters.anisotropyParams;
        }
        
        if (isParameterLocked(LOCK_CLEARCOAT)) {
            result.getClearcoat() = defaultParameters.getClearcoat();
            result.getClearcoatRoughness() = defaultParameters.getClearcoatRoughness();
            result.clearcoatNormalScale = defaultParameters.clearcoatNormalScale;
        }
        
        if (isParameterLocked(LOCK_SHEEN)) {
            result.sheenParams = defaultParameters.sheenParams;
            result.sheenColor = defaultParameters.sheenColor;
        }
        
        if (isParameterLocked(LOCK_TRANSMISSION)) {
            result.transmissionParams = defaultParameters.transmissionParams;
            result.volumeParams = defaultParameters.volumeParams;
            result.attenuationColor = defaultParameters.attenuationColor;
            result.attenuationDistance = defaultParameters.attenuationDistance;
            result.volumeAttenuationColor = defaultParameters.volumeAttenuationColor;
            result.volumeAttenuationDistance = defaultParameters.volumeAttenuationDistance;
        }
        
        if (isParameterLocked(LOCK_TEXTURE_FLAGS)) {
            result.materialFlags.x = defaultParameters.materialFlags.x;
        }
        
        if (isParameterLocked(LOCK_FEATURE_FLAGS)) {
            result.materialFlags.y = defaultParameters.materialFlags.y;
        }
        
        if (isParameterLocked(LOCK_ALPHA_MODE)) {
            result.materialFlags.z = defaultParameters.materialFlags.z;
        }
        
        if (isParameterLocked(LOCK_RENDERING_FLAGS)) {
            result.materialFlags.w = defaultParameters.materialFlags.w;
        }
        
        return result;
    }

} // namespace AstralEngine
