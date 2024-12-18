#pragma once

#include "draw/common/vic20.h"

#include "render/next/context/resource.hpp"

namespace sm::draw::next {
    constexpr shared::Vic20Character newChar(std::string_view text) noexcept {
        CTASSERTF(text.size() == 64, "Invalid character data size %zu", text.size());
        // treat `.` as selecting the background colour, and `#` as selecting the foreground colour.
        // all other characters should throw an error.

        uint64_t data = 0;
        for (size_t i = 0; i < text.size(); i++) {
            if (text[i] == '#') {
                data |= (1ull << i);
            } else if (text[i] != '.') {
                CT_NEVER("Invalid character `%c` at index %zu", text[i], i);
            }
        }

        return shared::Vic20Character{ data };
    }

    class Vic20Display final : public render::next::IContextResource {
        using Super = render::next::IContextResource;
        using InfoDeviceBuffer = render::next::MultiBufferResource<shared::Vic20Info>;

        using DeviceScreenBuffer = render::next::MultiBufferResource<shared::Vic20Screen>;
        using DeviceCharacterMap = render::next::MultiBufferResource<shared::Vic20CharacterMap>;

        render::Object<ID3D12RootSignature> mVic20RootSignature;
        render::Object<ID3D12PipelineState> mVic20PipelineState;
        void createComputeShader();


        InfoDeviceBuffer mInfoBuffer;
        void createConstBuffers(UINT length);
        D3D12_GPU_VIRTUAL_ADDRESS getInfoBufferAddress(UINT index) const noexcept;

        DeviceScreenBuffer mScreenBuffer;
        shared::Vic20Screen mScreenData{};
        DeviceCharacterMap mCharacterMap;
        shared::Vic20CharacterMap mCharacterData{};
        void createScreenBuffer(UINT length);


        render::next::TextureResource mTarget;
        size_t mTargetUAVIndex;
        size_t mTargetSRVIndex;
        math::uint2 mTargetSize;
        void createCompositionTarget();


        void resetDisplayData();
        void createDisplayData(render::next::SurfaceInfo info);
    public:
        Vic20Display(render::next::CoreContext& context, math::uint2 resolution) noexcept;

        void record(ID3D12GraphicsCommandList *list, UINT index);
        ID3D12Resource *getTarget() const noexcept { return mTarget.get(); }
        D3D12_GPU_DESCRIPTOR_HANDLE getTargetSrv() const;

        void write(uint8_t x, uint8_t y, uint8_t colour, uint8_t character) noexcept;

        void writeCharacter(uint8_t id, shared::Vic20Character character) noexcept;

        void setCharacterMap(shared::Vic20CharacterMap characterMap);
        void setScreen(draw::shared::Vic20Screen screen);

        // IContextResource
        void reset() noexcept override;
        void create() override;
        // ~IContextResource
    };
}
