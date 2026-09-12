#pragma once

// All interceptors MUST classify meshes through these helpers and compare CRCs through StageMatches /
// StageMatchesAny; never inline the raw stride/prim numbers or call D3D::CRCForTexture directly. The CRC
// tag values live in namespace D3D::Crc (see D3D.hpp).
//
// Priority ladder (for reference):
//   -10  CustomHighwayColorsMod  (highway textures; runs before note cluster)
//    10  RainbowNotesStems        (stems / bend-slide indicators)
//    20  ExtendedRangeNotes       (note heads, tails, stems)
//    30  TwitchNotes              (remove / transparent / solid overlay)
//    40  RainbowNotesHeadsTails   (heads + tails, runs last in cluster)

#include <initializer_list>
#include "../Framework/DrawRegistry.hpp"
#include "ExtendedRangeMode.hpp"
#include "../D3D/D3DHelper.hpp"
#include "../D3D/D3D.hpp"

namespace DrawMesh {

    // --- Mesh geometry constants ---
    // Vertex strides that identify note geometry; use these instead of raw literals.
    inline constexpr DWORD kNoteStemStride = 32;
    inline constexpr DWORD kNoteTailStride = 12;

    // --- Note geometry classifiers ---

    inline bool IsNoteHead(const Framework::DrawContext& ctx) {
        return IsToBeRemoved(sevenstring, ctx.mesh) || IsExtraRemoved(noteModifiers, ctx.thicc);
    }

    // Note stem, accent, bend indicator, or slide indicator.
    inline bool IsNoteStemOrAccent(const Framework::DrawContext& ctx) {
        return (ctx.mesh.Stride == kNoteStemStride && ctx.mesh.PrimCount == 2 && ctx.mesh.NumVertices == 4) ||
               (ctx.mesh.Stride == kNoteStemStride && ctx.mesh.PrimCount == 4 && ctx.mesh.NumVertices == 6);
    }

    inline bool IsNoteTail(const Framework::DrawContext& ctx) {
        return ctx.mesh.Stride == kNoteTailStride;
    }

    // --- CRC helpers ---
    // Query the stage CRC through ctx and compare against tag values from D3D::Crc.

    inline bool StageMatches(Framework::DrawContext& ctx, DWORD stage, DWORD tagCrc) {
        auto crc = ctx.StageCRC(stage);
        return crc.has_value() && *crc == tagCrc;
    }

    inline bool StageMatchesAny(Framework::DrawContext& ctx, DWORD stage, std::initializer_list<DWORD> tagCrcs) {
        auto crc = ctx.StageCRC(stage);
        if (!crc.has_value()) return false;
        for (DWORD tag : tagCrcs) {
            if (*crc == tag) return true;
        }
        return false;
    }

    inline bool IsNoteStemCrc(Framework::DrawContext& ctx) {
        return StageMatchesAny(ctx, 1, { D3D::Crc::StemsAccents, D3D::Crc::BendSlideIndicators });
    }
}
