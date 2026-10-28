# Patch: Add SongVolume and PreviewVolume properties to Attributes.fs
# These properties exist in Rocksmith manifest JSON but are not exposed by the DLCB library.
#
# Upstream PR iminashi/Rocksmith2014.NET#38 proposed adding these and was CLOSED (not merged):
# the maintainer omits them because official files store volume in the .bnk files, not the
# manifest, so they're "only found in customs (created with the Toolkit)". This patch is therefore
# permanent — main will never carry these. CFSM2's volume repair/normalize target customs, where
# the manifest IS the source of truth, so exposing them here is correct for our use.

param([string]$RepoDir)

$File = Join-Path $RepoDir "src/Rocksmith2014.Common/Manifest/Attributes.fs"
$Content = Get-Content $File -Raw

# Add PreviewVolume after PreviewBankPath
$Content = $Content -replace `
    '(    member val PreviewBankPath: string = null with get, set\r?\n)',
    "`$1`n    // Only found in customs (created with the Toolkit)`n    member val PreviewVolume: Nullable<float32> = Nullable() with get, set`n"

# Add SongVolume after SongPartition
$Content = $Content -replace `
    '(    member val SongPartition: Nullable<int> = Nullable\(\) with get, set\r?\n)',
    "`$1`n    // Only found in customs (created with the Toolkit)`n    member val SongVolume: Nullable<float32> = Nullable() with get, set`n"

Set-Content $File $Content -NoNewline
Write-Host "    Patched: Attributes.fs (added SongVolume, PreviewVolume)"
