<#
.SYNOPSIS
    Creates a new DLL mod from a template and lists it in DLL.vcxproj and DLL.vcxproj.filters.

.DESCRIPTION
    Writes DLL/Mods/<Name>.hpp and .cpp (a mod with one toggle, ready to build) and adds explicit
    project entries, the same ones Visual Studio writes. The project deliberately has no wildcards:
    VS rewrites them when files are added or removed in the IDE.

.EXAMPLE
    powershell Build/New-Mod.ps1 -Name ShowBpm
    Creates ShowBpmMod with the setting key "ShowBpmEnabled".
#>
param (
    [Parameter(Mandatory = $true)]
    [string]$Name
)

$ErrorActionPreference = 'Stop'

if ($Name -notmatch '^[A-Z][A-Za-z0-9]*$') {
    throw "Name must be PascalCase letters and digits, e.g. ShowBpm."
}

$base = $Name -replace 'Mod$', ''
if ($base -eq '') { throw "Name needs more than just 'Mod'." }
$class = "${base}Mod"

$dll = Join-Path (Split-Path $PSScriptRoot -Parent) 'DLL'
$hppPath = Join-Path $dll "Mods\$class.hpp"
$cppPath = Join-Path $dll "Mods\$class.cpp"
foreach ($path in $hppPath, $cppPath) {
    if (Test-Path $path) { throw "$path already exists." }
}

$utf8 = New-Object System.Text.UTF8Encoding($false)

function Write-Lf([string]$path, [string]$text) {
    [System.IO.File]::WriteAllText($path, ($text -replace "`r`n", "`n"), $utf8)
}

# Inserts $block (LF lines) after the last entry matching $pattern, past its closing tag if it has one.
function Add-Entry([string]$text, [string]$pattern, [string]$closing, [string]$block) {
    $lines = [System.Collections.Generic.List[string]]($text -split "`n")
    $at = -1
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($lines[$i] -match $pattern) { $at = $i }
    }
    if ($at -lt 0) { throw "No existing entry matches $pattern; add the mod by hand." }

    if ($lines[$at] -notmatch '/>\s*$') {
        while ($lines[$at] -notmatch $closing) { $at++ }
    }

    $lines.InsertRange($at + 1, [string[]]($block -split "`n"))
    return ($lines -join "`n")
}

Write-Lf $hppPath @"
#pragma once

#include "../Framework/Framework.hpp"

class $class : public Framework::IMod {
public:
	MOD_ID($class)
	Framework::SettingDefs Settings() const override;
	bool IsEnabled(const Framework::ModContext& c) const override;
	void OnSongTick(Framework::ModContext& c) override;
};

"@

$registrar = '_' + $class.Substring(0, 1).ToLower() + $class.Substring(1) + 'Reg'
Write-Lf $cppPath @"
#include "../stdafx.h"
#include "$class.hpp"

using Framework::ModContext;
using Framework::SettingDef;
using Framework::SettingDefs;

namespace {
	// Every mod's settings share one key space, so keys carry the mod's name.
	constexpr char kEnabled[] = "${base}Enabled";
}

SettingDefs ${class}::Settings() const {
	return {
		SettingDef::Toggle(kEnabled, "$base")
			.Hint("TODO: what this does. Shown as the tooltip in the GUI."),
	};
}

bool ${class}::IsEnabled(const ModContext& c) const {
	return c.IsOn(kEnabled);
}

void ${class}::OnSongTick(ModContext&) {
}

static Framework::ModRegistrar<$class> $registrar;

"@

$projectPath = Join-Path $dll 'DLL.vcxproj'
$filtersPath = Join-Path $dll 'DLL.vcxproj.filters'
$project = [System.IO.File]::ReadAllText($projectPath)
$filters = [System.IO.File]::ReadAllText($filtersPath)

$project = Add-Entry $project '^\s*<ClCompile Include="Mods\\([^"\\]+)\.cpp"' '</ClCompile>' @"
    <ClCompile Include="Mods\$class.cpp">
      <PrecompiledHeaderFile>../stdafx.h</PrecompiledHeaderFile>
    </ClCompile>
"@
$project = Add-Entry $project '^\s*<ClInclude Include="Mods\\([^"\\]+)\.hpp"' '</ClInclude>' @"
    <ClInclude Include="Mods\$class.hpp" />
"@
$filters = Add-Entry $filters '^\s*<ClCompile Include="Mods\\([^"\\]+)\.cpp"' '</ClCompile>' @"
    <ClCompile Include="Mods\$class.cpp">
      <Filter>Mods</Filter>
    </ClCompile>
"@
$filters = Add-Entry $filters '^\s*<ClInclude Include="Mods\\([^"\\]+)\.hpp"' '</ClInclude>' @"
    <ClInclude Include="Mods\$class.hpp">
      <Filter>Mods\Headers</Filter>
    </ClInclude>
"@

[System.IO.File]::WriteAllText($projectPath, $project, $utf8)
[System.IO.File]::WriteAllText($filtersPath, $filters, $utf8)

Write-Host "Created DLL\Mods\$class.hpp and .cpp, and listed them in DLL.vcxproj and .filters." -ForegroundColor Green
Write-Host ""
Write-Host "Next:"
Write-Host "  1. Fill in the mod. The README's 'Adding a mod' section has a worked example (hotkey, HUD line)."
Write-Host "  2. Build RSMods.sln (Release|Win32). If Visual Studio has the project open, it will offer to reload it."
Write-Host "  3. Regenerate the manifest so the GUI shows the setting:"
Write-Host "       powershell DLL/Framework/Tests/BuildAndRun.ps1 -DumpManifest"
