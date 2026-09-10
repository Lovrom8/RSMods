#pragma once

// All interceptors MUST classify meshes through these helpers and query CRCs through ctx.StageCRC; never inline the
// raw stride/prim numbers or call D3D::CRCForTexture directly.
//
// Priority ladder (for reference):
//   -10  CustomHighwayColorsMod  (highway textures; runs before note cluster)
//    10  RainbowNotesStems        (stems / bend-slide indicators)
//    20  ExtendedRangeNotes       (note heads, tails, stems)
//    30  TwitchNotes              (remove / transparent / solid overlay)
//    40  RainbowNotesHeadsTails   (heads + tails, runs last in cluster)

#include "../Framework/DrawRegistry.hpp"
#include "ExtendedRangeMode.hpp"
#include "../D3D/D3DHelper.hpp"
#include "../D3D/D3D.hpp"

namespace DrawMesh {

    // --- Note geometry classifiers ---

    inline bool IsNoteHead(const Framework::DrawContext& ctx) {
        return IsToBeRemoved(sevenstring, ctx.mesh) || IsExtraRemoved(noteModifiers, ctx.thicc);
    }

    // Note stem, accent, bend indicator, or slide indicator.
    inline bool IsNoteStemOrAccent(const Framework::DrawContext& ctx) {
        return (ctx.mesh.Stride == 32 && ctx.mesh.PrimCount == 2 && ctx.mesh.NumVertices == 4) ||
               (ctx.mesh.Stride == 32 && ctx.mesh.PrimCount == 4 && ctx.mesh.NumVertices == 6);
    }

    inline bool IsNoteTail(const Framework::DrawContext& ctx) {
        return ctx.mesh.Stride == 12;
    }

    // --- CRC helpers ---
    // Use ctx.StageCRC(stage) to compare -> never call D3D::CRCForTexture directly.

    inline bool IsNoteStemCrc(Framework::DrawContext& ctx) {
        auto crc1 = ctx.StageCRC(1);
        return crc1.has_value() && (*crc1 == crcStemsAccents || *crc1 == crcBendSlideIndicators);
    }

    /// Generic: query stage CRC from ctx and test against a specific value.
    /// Returns true when the CRC was successfully computed and equals tagCrc.
    inline bool StageMatches(Framework::DrawContext& ctx, DWORD stage, DWORD tagCrc) {
        auto crc = ctx.StageCRC(stage);
        return crc.has_value() && *crc == tagCrc;
    }

}

// Use these instead of the raw globals in interceptor code.
namespace DrawMesh::Crc {
    inline DWORD StemsAccents()        { return ::crcStemsAccents; }
    inline DWORD BendSlideIndicators() { return ::crcBendSlideIndicators; }
    inline DWORD NoteLanes()           { return ::crcNoteLanes; }
    inline DWORD NotewayFretNumbers()  { return ::crcNotewayFretNumbers; }
    inline DWORD NotewayGutters()      { return ::crcNotewayGutters; }
    inline DWORD SkylinePurple()       { return ::crcSkylinePurple; }
    inline DWORD SkylineOrange()       { return ::crcSkylineOrange; }
    inline DWORD SkylineBackground()   { return ::crcSkylineBackground; }
    inline DWORD SkylineShadow()       { return ::crcSkylineShadow; }
    inline DWORD Headstock0()          { return ::crcHeadstock0; }
    inline DWORD Headstock1()          { return ::crcHeadstock1; }
    inline DWORD Headstock2()          { return ::crcHeadstock2; }
    inline DWORD Headstock3()          { return ::crcHeadstock3; }
    inline DWORD Headstock4()          { return ::crcHeadstock4; }
}
