#include "render/editor/features.hpp"

#include "render/render.hpp"

using namespace sm;
using namespace sm::editor;

namespace MyGui {
    template<ctu::Reflected T>
    void TextReflect(const char *name, const T &value, int base = 10) {
        using Reflect = ctu::TypeInfo<T>;
        ImGui::Text("%s: ", name);
        ImGui::SameLine();
        ImGui::TextUnformatted(Reflect::to_string(value, base).c_str());
    }
}

void FeatureSupportPanel::draw_feature(const char *name, bool value) {
    ImGui::Text("%s: ", name);
    ImGui::SameLine();
    if (value) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "true");
    } else {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "false");
    }
}

void FeatureSupportPanel::draw_content() {
    const auto& feat = mContext.mFeatureSupport;

    MyGui::TextReflect<render::FeatureLevel>("Feature Level", feat.MaxSupportedFeatureLevel());
    MyGui::TextReflect<render::ShaderModel>("Shader Model", feat.HighestShaderModel());
    MyGui::TextReflect<render::RootSignatureVersion>("Root Signature Version", feat.HighestRootSignatureVersion());
    MyGui::TextReflect<render::ProtectedResourceSessionSupport>("Protected Resource Session Support", feat.ProtectedResourceSessionSupport());
    MyGui::TextReflect<render::ShaderCacheSupportFlags>("Shader Cache Support Flags", feat.ShaderCacheSupportFlags());
    ImGui::Text("Max GPU Virtual Address Bits: 0x%x", feat.MaxGPUVirtualAddressBitsPerProcess());
    draw_feature("Existing Heaps Supported", feat.ExistingHeapsSupported());
    MyGui::TextReflect<render::HeapSerializationTier>("Heap Serialization Tier", feat.HeapSerializationTier());
    draw_feature("Cross Node Atomic Shader Instructions", feat.CrossNodeAtomicShaderInstructions());
    draw_feature("Displayable Texture", feat.DisplayableTexture());
    ImGui::Text("Protected Resource Session Type Count: %u", feat.ProtectedResourceSessionTypeCount());

    if (ImGui::TreeNodeEx("ARCHITECTURE1")) {
        draw_feature("Tile Based Renderer", feat.TileBasedRenderer());
        draw_feature("UMA", feat.UMA());
        draw_feature("Cache Coherent UMA", feat.CacheCoherentUMA());
        draw_feature("Isolated MMU", feat.IsolatedMMU());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS")) {
        draw_feature("Double Precision Float Shader Ops", feat.DoublePrecisionFloatShaderOps());
        draw_feature("Output Merger Logic Op", feat.OutputMergerLogicOp());
        MyGui::TextReflect<render::ShaderMinPrecisionSupport>("Min Precision Support", feat.MinPrecisionSupport());
        MyGui::TextReflect<render::TiledResourceTier>("Tiled Resources Tier", feat.TiledResourcesTier());
        MyGui::TextReflect<render::ResourceBindingTier>("Resource Binding Tier", feat.ResourceBindingTier());
        draw_feature("PS Specified Stencil Ref", feat.PSSpecifiedStencilRefSupported());
        draw_feature("Typed UAV Load Additional Formats", feat.TypedUAVLoadAdditionalFormats());
        draw_feature("ROVs Supported", feat.ROVsSupported());
        MyGui::TextReflect<render::ConservativeRasterizationTier>("Conservative Rasterization Tier", feat.ConservativeRasterizationTier());
        draw_feature("Standard Swizzle 64KB", feat.StandardSwizzle64KBSupported());
        draw_feature("Cross Node Sharing Tier", feat.CrossNodeSharingTier());
        draw_feature("VP And RT Array Index From Any Shader Feeding Rasterizer Without GS Emulation", feat.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation());
        MyGui::TextReflect<render::ResourceHeapTier>("Resource Heap Tier", feat.ResourceHeapTier());
        MyGui::TextReflect<render::CrossNodeSharingTier>("Cross Node Sharing Tier", feat.CrossNodeSharingTier());
        ImGui::Text("Max GPU Virtual Address Bits Per Resource: 0x%x", feat.MaxGPUVirtualAddressBitsPerResource());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS1")) {
        draw_feature("Wave Ops", feat.WaveOps());
        ImGui::Text("Wave Lane Count Min: %u", feat.WaveLaneCountMin());
        ImGui::Text("Wave Lane Count Max: %u", feat.WaveLaneCountMax());
        ImGui::Text("Total Lane Count: %u", feat.TotalLaneCount());
        draw_feature("Expanded Compute Resource States", feat.ExpandedComputeResourceStates());
        draw_feature("Int64 Shader Ops", feat.Int64ShaderOps());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS2")) {
        draw_feature("Depth Bounds Test Supported", feat.DepthBoundsTestSupported());
        MyGui::TextReflect<render::ProgrammableSamplePositionsTier>("Programmable Sample Positions Tier", feat.ProgrammableSamplePositionsTier());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS3")) {
        draw_feature("Copy Queue Timestamp Queries Supported", feat.CopyQueueTimestampQueriesSupported());
        draw_feature("Casting Fully Typed Format Supported", feat.CastingFullyTypedFormatSupported());
        MyGui::TextReflect<render::CommandListSupportFlags>("Command List Support Flags", feat.WriteBufferImmediateSupportFlags());
        MyGui::TextReflect<render::ViewInstancingTier>("View Instancing Tier", feat.ViewInstancingTier());
        draw_feature("Barycentrics Supported", feat.BarycentricsSupported());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS4")) {
        draw_feature("MSAA64KB Aligned Textures", feat.MSAA64KBAlignedTextureSupported());
        MyGui::TextReflect<render::SharedResourceCompatibilityTier>("Shared Resource Compatibility Tier", feat.SharedResourceCompatibilityTier());
        draw_feature("Native16BitShaderOps Supported", feat.Native16BitShaderOpsSupported());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS5")) {
        draw_feature("SRV Only Tiled Resource Tier3", feat.SRVOnlyTiledResourceTier3());
        MyGui::TextReflect<render::RenderPassTier>("Render Pass Tier", feat.RenderPassesTier());
        draw_feature("Raytracing Tier", feat.RaytracingTier());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS6")) {
        draw_feature("Additional Shading Rates Supported", feat.AdditionalShadingRatesSupported());
        draw_feature("Per Primitive Shading Rate With Viewport Indexing", feat.PerPrimitiveShadingRateSupportedWithViewportIndexing());
        MyGui::TextReflect<render::VariableShadingRateTier>("Variable Shading Rate Tier", feat.VariableShadingRateTier());
        ImGui::Text("Shading Rate Image Tile Size: %u", feat.ShadingRateImageTileSize());
        draw_feature("Background Processing Supported", feat.BackgroundProcessingSupported());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS7")) {
        MyGui::TextReflect<render::MeshShaderTier>("Mesh Shader Tier", feat.MeshShaderTier());
        draw_feature("Sampler Feedback Tier", feat.SamplerFeedbackTier());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS8")) {
        draw_feature("Unaligned Block Texture Supported", feat.UnalignedBlockTexturesSupported());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS9")) {
        draw_feature("Mesh Shading Pipeline Stats Supported", feat.MeshShaderPipelineStatsSupported());
        draw_feature("Mesh Shader Supports Full Range Render Target Array Index", feat.MeshShaderSupportsFullRangeRenderTargetArrayIndex());
        draw_feature("Atomic Int64 On Typed Resources", feat.AtomicInt64OnTypedResourceSupported());
        draw_feature("Atomic Int64 On Group Shared", feat.AtomicInt64OnGroupSharedSupported());
        draw_feature("Derivatives In Mesh Shaders And Amplification Shaders", feat.DerivativesInMeshAndAmplificationShadersSupported());
        MyGui::TextReflect<render::WaveMMA>("Wave MMA", feat.WaveMMATier());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS10")) {
        draw_feature("Variant Rate Shading Sub Combiner", feat.VariableRateShadingSumCombinerSupported());
        draw_feature("Mesh Shader Per Primitive Shading Rate", feat.MeshShaderPerPrimitiveShadingRateSupported());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS11")) {
        draw_feature("Atomic Int64 On Descriptor Heap Resource Supported", feat.AtomicInt64OnDescriptorHeapResourceSupported());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS12")) {
        draw_feature("MS Primitives Pipeline Statistic Includes Culled Primitives", feat.MSPrimitivesPipelineStatisticIncludesCulledPrimitives());
        draw_feature("Enhanced Barriers Supported", feat.EnhancedBarriersSupported());
        draw_feature("Relaxed Format Casting Supported", feat.RelaxedFormatCastingSupported());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS13")) {
        draw_feature("Unrestricted Buffer Texture Copy Pitch Supported", feat.UnrestrictedBufferTextureCopyPitchSupported());
        draw_feature("Unrestricted Vertex Element Alignment Supported", feat.UnrestrictedVertexElementAlignmentSupported());
        draw_feature("Inverted Viewport Height Flips Y Supported", feat.InvertedViewportHeightFlipsYSupported());
        draw_feature("Inverted Viewport Depth Flips Z Supported", feat.InvertedViewportDepthFlipsZSupported());
        draw_feature("Texture Copy Between Dimensions Supported", feat.TextureCopyBetweenDimensionsSupported());
        draw_feature("Alpha Blend Factor Supported", feat.AlphaBlendFactorSupported());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS14")) {
        draw_feature("Advanced Texture Ops Supported", feat.AdvancedTextureOpsSupported());
        draw_feature("Writeable MSAA Textures Supported", feat.WriteableMSAATexturesSupported());
        draw_feature("Independent Front And Back Stencil Ref Mask Supported", feat.IndependentFrontAndBackStencilRefMaskSupported());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS15")) {
        draw_feature("Triangle Fan Supported", feat.TriangleFanSupported());
        draw_feature("Dynamic Index Buffer Strip Cut Supported", feat.DynamicIndexBufferStripCutSupported());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS16")) {
        draw_feature("Dynamic Depth Bias Supported", feat.DynamicDepthBiasSupported());
        draw_feature("GPU Upload Heap Supported", feat.GPUUploadHeapSupported());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS17")) {
        draw_feature("Non-Normalized Coordinate Samplers Supported", feat.NonNormalizedCoordinateSamplersSupported());
        draw_feature("Manual Write Tracking Resource Supported", feat.ManualWriteTrackingResourceSupported());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS18")) {
        draw_feature("Render Passes Valid", feat.RenderPassesValid());
        draw_feature("Mismatching Output Dimensions Supported", feat.MismatchingOutputDimensionsSupported());
        ImGui::Text("Supported Sample Counts With No Outputs: %u", feat.SupportedSampleCountsWithNoOutputs());
        draw_feature("Point Sampling Addresses Never Round Up", feat.PointSamplingAddressesNeverRoundUp());
        draw_feature("Rasterizer Desc2 Supported", feat.RasterizerDesc2Supported());
        draw_feature("Narrow Quadrilateral Lines Supported", feat.NarrowQuadrilateralLinesSupported());
        draw_feature("Aniso Filter With Point Mip Supported", feat.AnisoFilterWithPointMipSupported());
        ImGui::Text("Max Sampler Descriptor Heap Size: %u", feat.MaxSamplerDescriptorHeapSize());
        ImGui::Text("Max Sampler Descriptor Heap Size With Static Samplers: %u", feat.MaxSamplerDescriptorHeapSizeWithStaticSamplers());
        ImGui::Text("Max View Descriptor Heap Size: %u", feat.MaxViewDescriptorHeapSize());
        draw_feature("Compute Only Write Watch Supported", feat.ComputeOnlyWriteWatchSupported());
        ImGui::TreePop();
    }
}

FeatureSupportPanel::FeatureSupportPanel(render::Context &context)
    : IEditorPanel("D3D12 Feature Support")
    , mContext(context)
{ }
