using RocksmithToolkitLib.DLCPackage;
using RocksmithToolkitLib.Extensions;
using RSMods.Rocksmith;
using RSMods.Util;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime;
using System.Threading.Tasks;

namespace RSMods.Core
{
    /// <summary>A result-screen voice line the user can replace: its display label and the WEM file it maps to.</summary>
    public sealed class SoundPackVoiceLine(string label, string wem)
    {
        public string Label { get; } = label;
        public string Wem { get; } = wem;
    }

    /// <summary>
    /// UI-agnostic boundary for the SoundPacks feature: unpack/repack <c>audio.psarc</c> (with progress),
    /// replace a result voice line, import/export a <c>.rs_soundpack</c>, reset to the stock pack, and
    /// preview a result VO in the running game. Frontends own the prompts and progress presentation; this
    /// service only performs the operations and throws on failure.
    /// </summary>
    public sealed class SoundPackService
    {
        /// <summary>The game's packed audio archive (<c>&lt;install&gt;\audio.psarc</c>).</summary>
        public string AudioPsarcPath => Path.Combine(GenUtil.GetRSDirectory(), "audio.psarc");

        /// <summary>The RSMods working folder (<c>&lt;install&gt;\RSMods</c>).</summary>
        public string RsModsPath => GenUtil.GetRsModsPath();

        /// <summary>Where <c>audio.psarc</c> is unpacked to.</summary>
        public string UnpackRoot => Path.Combine(RsModsPath, "audio_psarc");

        /// <summary>The platform subfolder inside the unpacked archive that gets repacked.</summary>
        public string UnpackedPcPath => Path.Combine(UnpackRoot, "audio_psarc_RS2014_Pc");

        /// <summary>True once <c>audio.psarc</c> has been unpacked (drives the tab's two-state UI).</summary>
        public bool IsUnpacked => Directory.Exists(UnpackRoot);

        /// <summary>True when the repack source folder is present (missing means a fresh unpack is needed first).</summary>
        public bool IsUnpackedPcPresent => Directory.Exists(UnpackedPcPath);

        /// <summary>The 18 replaceable result voice lines, ordered by score, each with its label and WEM file.</summary>
        public IReadOnlyList<SoundPackVoiceLine> VoiceLines { get; } = BuildVoiceLines();

        /// <summary>The result voice-overs that can be previewed in-game, by display name.</summary>
        public IReadOnlyList<string> ResultVoiceOvers => Dictionaries.ResultVoiceOverDictionary.Keys.ToList();

        /// <summary>Unpacks <c>audio.psarc</c> into <see cref="UnpackRoot"/>, reporting progress.</summary>
        public Task UnpackAsync(IProgress<int> value, IProgress<string> operation) =>
            RunPsarcTaskAsync(() => Packer.Unpack(AudioPsarcPath, UnpackRoot), value, operation);

        /// <summary>Repacks the unpacked folder back into <c>audio.psarc</c>, reporting progress.</summary>
        public Task RepackAsync(IProgress<int> value, IProgress<string> operation) =>
            RunPsarcTaskAsync(() => Packer.Pack(UnpackedPcPath, AudioPsarcPath), value, operation);

        // Bridges the toolkit's static GlobalProgress events to the caller's IProgress for the duration of the
        // work, always detaching the handlers and forcing an LOH-compacting collection afterwards (packing/
        // unpacking allocates very large buffers) — mirroring the old WinForms SoundPacks executor.
        private static async Task RunPsarcTaskAsync(Action work, IProgress<int> value, IProgress<string> operation)
        {
            void OnValue(int v) => value?.Report(v);
            void OnOperation(string o) => operation?.Report(o);

            GlobalProgress.ProgressValueChanged += OnValue;
            GlobalProgress.CurrentOperationChanged += OnOperation;
            try
            {
                await Task.Run(work);
            }
            finally
            {
                GlobalProgress.ProgressValueChanged -= OnValue;
                GlobalProgress.CurrentOperationChanged -= OnOperation;

                GCSettings.LargeObjectHeapCompactionMode = GCLargeObjectHeapCompactionMode.CompactOnce;
                GC.Collect(2, GCCollectionMode.Forced, blocking: true, compacting: true);
            }
        }

        /// <summary>Deletes the unpacked audio folder, returning the tab to its "not unpacked" state.</summary>
        public void RemoveUnpacked()
        {
            if (Directory.Exists(UnpackRoot))
                Directory.Delete(UnpackRoot, true);
        }

        /// <summary>Converts <paramref name="sourceFile"/> to WEM and replaces the given voice line's file.</summary>
        public void ReplaceSound(string voiceLineWem, string sourceFile) =>
            Soundpacks.ConvertSoundAndReplace(
                Soundpacks.SoundPackLocationPrefix + Soundpacks.SoundPackEnglishPrefix + voiceLineWem,
                sourceFile, RsModsPath);

        /// <summary>Imports a <c>.rs_soundpack</c> archive into the unpacked audio folder.</summary>
        public static void Import(string file) => Soundpacks.ImportSoundFile(file);

        /// <summary>Exports the current result voice lines to a <c>.rs_soundpack</c> archive.</summary>
        public static void Export(string file) => Soundpacks.ExportSoundFile(file);

        /// <summary>Restores the stock voice lines from the pack embedded in GUI.Core.</summary>
        public static void ResetToStock() => Soundpacks.Reset();

        /// <summary>Asks the running game to play a result voice-over (no-op if Rocksmith is not running).</summary>
        public void PlayResultVoiceOver(string displayName)
        {
            if (Dictionaries.ResultVoiceOverDictionary.TryGetValue(displayName, out string wwiseEvent))
                WinMsgUtil.SendMsgToRS($"WwiseEvent {wwiseEvent}");
        }

        // Ordered by ascending score, matching the ranges shown on the WinForms tab.
        private static IReadOnlyList<SoundPackVoiceLine> BuildVoiceLines() =>
        [
            new("0–49% · Bad Performance", Soundpacks.VoiceLines.BadPerformance),
            new("50–59% · Disappointing Performance", Soundpacks.VoiceLines.DisappointingPerformance),
            new("60–65% · Subpar Performance", Soundpacks.VoiceLines.SubparPerformance),
            new("66–72% · Could Be Better", Soundpacks.VoiceLines.CouldBeBetter),
            new("73–80% · Decent Performance", Soundpacks.VoiceLines.DecentPerformance),
            new("81% · Alright Performance", Soundpacks.VoiceLines.AlrightPerformance),
            new("82–85% · Excellent Performance", Soundpacks.VoiceLines.ExcellentPerformance),
            new("86–89% · Top Notch Performance", Soundpacks.VoiceLines.TopNotchPerformance),
            new("90–91% · Superb Performance", Soundpacks.VoiceLines.SuperbPerformance),
            new("92% · Dazzling Performance", Soundpacks.VoiceLines.DazzlingPerformance),
            new("93% · You're Gonna Be A Superstar", Soundpacks.VoiceLines.YoureGonnaBeASuperstar),
            new("94% · Wonderful Performance", Soundpacks.VoiceLines.WonderfulPerformance),
            new("95% · Exceptional Performance", Soundpacks.VoiceLines.ExceptionalPerformance),
            new("96% · Amazing Performance", Soundpacks.VoiceLines.AmazingPerformance),
            new("97% · Exemplary Performance", Soundpacks.VoiceLines.ExemplaryPerformance),
            new("98% · Masterful Performance", Soundpacks.VoiceLines.MasterfulPerformance_98),
            new("99% · Masterful Performance", Soundpacks.VoiceLines.MasterfulPerformance_99),
            new("100% · Flawless Performance", Soundpacks.VoiceLines.FlawlessPerformance),
        ];
    }
}
