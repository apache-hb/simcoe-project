#include "render/editor/features.hpp"

#include "render/render.hpp"

using namespace sm;
using namespace sm::editor;

static auto gSink = logs::get_sink(logs::Category::eRender);

namespace MyGui {
    template<ctu::Reflected T>
    void TextReflect(const char *name, const T &value, int base = 10) {
        using Reflect = ctu::TypeInfo<T>;
        ImGui::Text("%s: ", name);
        ImGui::SameLine();
        ImGui::TextUnformatted(Reflect::to_string(value, base).c_str());
    }

    void TextFeature(const char *name, bool value) {
        ImGui::Text("%s: ", name);
        ImGui::SameLine();
        if (value) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Supported");
        } else {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Not supported");
        }
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
    MyGui::TextFeature("Existing Heaps Supported", feat.ExistingHeapsSupported());
    MyGui::TextReflect<render::HeapSerializationTier>("Heap Serialization Tier", feat.HeapSerializationTier());
    MyGui::TextFeature("Cross Node Atomic Shader Instructions", feat.CrossNodeAtomicShaderInstructions());
    MyGui::TextFeature("Displayable Texture", feat.DisplayableTexture());
    ImGui::Text("Protected Resource Session Type Count: %u", feat.ProtectedResourceSessionTypeCount());

    if (ImGui::TreeNodeEx("ARCHITECTURE1")) {
        MyGui::TextFeature("Tile Based Renderer", feat.TileBasedRenderer());
        MyGui::TextFeature("UMA", feat.UMA());
        MyGui::TextFeature("Cache Coherent UMA", feat.CacheCoherentUMA());
        MyGui::TextFeature("Isolated MMU", feat.IsolatedMMU());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS")) {
        MyGui::TextFeature("Double Precision Float Shader Ops", feat.DoublePrecisionFloatShaderOps());
        MyGui::TextFeature("Output Merger Logic Op", feat.OutputMergerLogicOp());
        MyGui::TextReflect<render::ShaderMinPrecisionSupport>("Min Precision Support", feat.MinPrecisionSupport());
        MyGui::TextReflect<render::TiledResourceTier>("Tiled Resources Tier", feat.TiledResourcesTier());
        MyGui::TextReflect<render::ResourceBindingTier>("Resource Binding Tier", feat.ResourceBindingTier());
        MyGui::TextFeature("PS Specified Stencil Ref", feat.PSSpecifiedStencilRefSupported());
        MyGui::TextFeature("Typed UAV Load Additional Formats", feat.TypedUAVLoadAdditionalFormats());
        MyGui::TextFeature("ROVs Supported", feat.ROVsSupported());
        MyGui::TextReflect<render::ConservativeRasterizationTier>("Conservative Rasterization Tier", feat.ConservativeRasterizationTier());
        MyGui::TextFeature("Standard Swizzle 64KB", feat.StandardSwizzle64KBSupported());
        MyGui::TextFeature("Cross Node Sharing Tier", feat.CrossNodeSharingTier());
        MyGui::TextFeature("VP And RT Array Index From Any Shader Feeding Rasterizer Without GS Emulation", feat.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation());
        MyGui::TextReflect<render::ResourceHeapTier>("Resource Heap Tier", feat.ResourceHeapTier());
        MyGui::TextReflect<render::CrossNodeSharingTier>("Cross Node Sharing Tier", feat.CrossNodeSharingTier());
        ImGui::Text("Max GPU Virtual Address Bits Per Resource: 0x%x", feat.MaxGPUVirtualAddressBitsPerResource());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS1")) {
        MyGui::TextFeature("Wave Ops", feat.WaveOps());
        ImGui::Text("Wave Lane Count Min: %u", feat.WaveLaneCountMin());
        ImGui::Text("Wave Lane Count Max: %u", feat.WaveLaneCountMax());
        ImGui::Text("Total Lane Count: %u", feat.TotalLaneCount());
        MyGui::TextFeature("Expanded Compute Resource States", feat.ExpandedComputeResourceStates());
        MyGui::TextFeature("Int64 Shader Ops", feat.Int64ShaderOps());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS2")) {
        MyGui::TextFeature("Depth Bounds Test Supported", feat.DepthBoundsTestSupported());
        MyGui::TextReflect<render::ProgrammableSamplePositionsTier>("Programmable Sample Positions Tier", feat.ProgrammableSamplePositionsTier());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS3")) {
        MyGui::TextFeature("Copy Queue Timestamp Queries Supported", feat.CopyQueueTimestampQueriesSupported());
        MyGui::TextFeature("Casting Fully Typed Format Supported", feat.CastingFullyTypedFormatSupported());
        MyGui::TextReflect<render::CommandListSupportFlags>("Command List Support Flags", feat.WriteBufferImmediateSupportFlags());
        MyGui::TextReflect<render::ViewInstancingTier>("View Instancing Tier", feat.ViewInstancingTier());
        MyGui::TextFeature("Barycentrics Supported", feat.BarycentricsSupported());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS4")) {
        MyGui::TextFeature("MSAA64KB Aligned Textures", feat.MSAA64KBAlignedTextureSupported());
        MyGui::TextReflect<render::SharedResourceCompatibilityTier>("Shared Resource Compatibility Tier", feat.SharedResourceCompatibilityTier());
        MyGui::TextFeature("Native16BitShaderOps Supported", feat.Native16BitShaderOpsSupported());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS5")) {
        MyGui::TextFeature("SRV Only Tiled Resource Tier3", feat.SRVOnlyTiledResourceTier3());
        MyGui::TextReflect<render::RenderPassTier>("Render Pass Tier", feat.RenderPassesTier());
        MyGui::TextFeature("Raytracing Tier", feat.RaytracingTier());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS6")) {
        MyGui::TextFeature("Additional Shading Rates Supported", feat.AdditionalShadingRatesSupported());
        MyGui::TextFeature("Per Primitive Shading Rate With Viewport Indexing", feat.PerPrimitiveShadingRateSupportedWithViewportIndexing());
        MyGui::TextReflect<render::VariableShadingRateTier>("Variable Shading Rate Tier", feat.VariableShadingRateTier());
        ImGui::Text("Shading Rate Image Tile Size: %u", feat.ShadingRateImageTileSize());
        MyGui::TextFeature("Background Processing Supported", feat.BackgroundProcessingSupported());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS7")) {
        MyGui::TextReflect<render::MeshShaderTier>("Mesh Shader Tier", feat.MeshShaderTier());
        MyGui::TextFeature("Sampler Feedback Tier", feat.SamplerFeedbackTier());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS8")) {
        MyGui::TextFeature("Unaligned Block Texture Supported", feat.UnalignedBlockTexturesSupported());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS9")) {
        MyGui::TextFeature("Mesh Shading Pipeline Stats Supported", feat.MeshShaderPipelineStatsSupported());
        MyGui::TextFeature("Mesh Shader Supports Full Range Render Target Array Index", feat.MeshShaderSupportsFullRangeRenderTargetArrayIndex());
        MyGui::TextFeature("Atomic Int64 On Typed Resources", feat.AtomicInt64OnTypedResourceSupported());
        MyGui::TextFeature("Atomic Int64 On Group Shared", feat.AtomicInt64OnGroupSharedSupported());
        MyGui::TextFeature("Derivatives In Mesh Shaders And Amplification Shaders", feat.DerivativesInMeshAndAmplificationShadersSupported());
        MyGui::TextReflect<render::WaveMMA>("Wave MMA", feat.WaveMMATier());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS10")) {
        MyGui::TextFeature("Variant Rate Shading Sub Combiner", feat.VariableRateShadingSumCombinerSupported());
        MyGui::TextFeature("Mesh Shader Per Primitive Shading Rate", feat.MeshShaderPerPrimitiveShadingRateSupported());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS11")) {
        MyGui::TextFeature("Atomic Int64 On Descriptor Heap Resource Supported", feat.AtomicInt64OnDescriptorHeapResourceSupported());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS12")) {
        MyGui::TextFeature("MS Primitives Pipeline Statistic Includes Culled Primitives", feat.MSPrimitivesPipelineStatisticIncludesCulledPrimitives());
        MyGui::TextFeature("Enhanced Barriers Supported", feat.EnhancedBarriersSupported());
        MyGui::TextFeature("Relaxed Format Casting Supported", feat.RelaxedFormatCastingSupported());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS13")) {
        MyGui::TextFeature("Unrestricted Buffer Texture Copy Pitch Supported", feat.UnrestrictedBufferTextureCopyPitchSupported());
        MyGui::TextFeature("Unrestricted Vertex Element Alignment Supported", feat.UnrestrictedVertexElementAlignmentSupported());
        MyGui::TextFeature("Inverted Viewport Height Flips Y Supported", feat.InvertedViewportHeightFlipsYSupported());
        MyGui::TextFeature("Inverted Viewport Depth Flips Z Supported", feat.InvertedViewportDepthFlipsZSupported());
        MyGui::TextFeature("Texture Copy Between Dimensions Supported", feat.TextureCopyBetweenDimensionsSupported());
        MyGui::TextFeature("Alpha Blend Factor Supported", feat.AlphaBlendFactorSupported());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS14")) {
        MyGui::TextFeature("Advanced Texture Ops Supported", feat.AdvancedTextureOpsSupported());
        MyGui::TextFeature("Writeable MSAA Textures Supported", feat.WriteableMSAATexturesSupported());
        MyGui::TextFeature("Independent Front And Back Stencil Ref Mask Supported", feat.IndependentFrontAndBackStencilRefMaskSupported());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS15")) {
        MyGui::TextFeature("Triangle Fan Supported", feat.TriangleFanSupported());
        MyGui::TextFeature("Dynamic Index Buffer Strip Cut Supported", feat.DynamicIndexBufferStripCutSupported());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS16")) {
        MyGui::TextFeature("Dynamic Depth Bias Supported", feat.DynamicDepthBiasSupported());
        MyGui::TextFeature("GPU Upload Heap Supported", feat.GPUUploadHeapSupported());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS17")) {
        MyGui::TextFeature("Non-Normalized Coordinate Samplers Supported", feat.NonNormalizedCoordinateSamplersSupported());
        MyGui::TextFeature("Manual Write Tracking Resource Supported", feat.ManualWriteTrackingResourceSupported());
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("D3D12_OPTIONS18")) {
        MyGui::TextFeature("Render Passes Valid", feat.RenderPassesValid());
        MyGui::TextFeature("Mismatching Output Dimensions Supported", feat.MismatchingOutputDimensionsSupported());
        ImGui::Text("Supported Sample Counts With No Outputs: %u", feat.SupportedSampleCountsWithNoOutputs());
        MyGui::TextFeature("Point Sampling Addresses Never Round Up", feat.PointSamplingAddressesNeverRoundUp());
        MyGui::TextFeature("Rasterizer Desc2 Supported", feat.RasterizerDesc2Supported());
        MyGui::TextFeature("Narrow Quadrilateral Lines Supported", feat.NarrowQuadrilateralLinesSupported());
        MyGui::TextFeature("Aniso Filter With Point Mip Supported", feat.AnisoFilterWithPointMipSupported());
        ImGui::Text("Max Sampler Descriptor Heap Size: %u", feat.MaxSamplerDescriptorHeapSize());
        ImGui::Text("Max Sampler Descriptor Heap Size With Static Samplers: %u", feat.MaxSamplerDescriptorHeapSizeWithStaticSamplers());
        ImGui::Text("Max View Descriptor Heap Size: %u", feat.MaxViewDescriptorHeapSize());
        MyGui::TextFeature("Compute Only Write Watch Supported", feat.ComputeOnlyWriteWatchSupported());
        ImGui::TreePop();
    }
}

FeatureSupportPanel::FeatureSupportPanel(render::Context &context)
    : IEditorPanel("D3D12 Feature Support")
    , mContext(context)
{ }
