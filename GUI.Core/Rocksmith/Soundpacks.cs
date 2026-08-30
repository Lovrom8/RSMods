using NAudio.Wave;
using RocksmithToolkitLib.Ogg;
using RSMods.Core;
using RSMods.Util;
using SevenZip;
using System;
using System.Collections.Generic;
using System.IO;

namespace RSMods.Rocksmith
{
    /// <summary>
    /// SoundPack conversion and packaging: swaps Rocksmith's result-screen voice lines by converting a
    /// user-supplied audio file to WEM and dropping it into the unpacked <c>audio.psarc</c>, plus
    /// import/export of a <c>.rs_soundpack</c> archive and a reset to the stock voice lines.
    /// </summary>
    public static class Soundpacks
    {
        public static class VoiceLines
        {
            public const string BadPerformance = "2066953778.wem";
            public const string DisappointingPerformance = "2067218742.wem";
            public const string SubparPerformance = "2066826048.wem";
            public const string CouldBeBetter = "2068001585.wem";
            public const string DecentPerformance = "2068133176.wem";
            public const string AlrightPerformance = "2068002869.wem";
            public const string ExcellentPerformance = "2067285052.wem";
            public const string TopNotchPerformance = "2067281979.wem";
            public const string SuperbPerformance = "2067350856.wem";
            public const string DazzlingPerformance = "2068132687.wem";
            public const string YoureGonnaBeASuperstar = "2068199486.wem";
            public const string WonderfulPerformance = "2067154245.wem";
            public const string ExceptionalPerformance = "2067153482.wem";
            public const string AmazingPerformance = "2067871807.wem";
            public const string ExemplaryPerformance = "2067022644.wem";
            public const string MasterfulPerformance_98 = "2068137287.wem";
            public const string MasterfulPerformance_99 = "2067870540.wem";
            public const string FlawlessPerformance = "2068002100.wem";
        }

        public const string SoundPackLocationPrefix = "audio_psarc\\audio_psarc_RS2014_Pc\\audio\\windows\\";
        public const string SoundPackEnglishPrefix = "english(us)\\";

        // The 7z native library and the stock pack resolve against the application's base directory, not the
        // current working directory. This matters for Avalonia, whose CWD is not the RSMods folder.
        private static string SevenZipLibraryPath => Path.Combine(AppContext.BaseDirectory, "7z64.dll");

        private static void SafeDelete(string path)
        {
            if (File.Exists(path)) File.Delete(path);
        }

        private static string ConvertMP3ToWav(string mp3File)
        {
            string wavFile = Path.Combine(Path.GetDirectoryName(mp3File), Path.GetFileNameWithoutExtension(mp3File) + ".wav");

            using (var mp3FileReader = new Mp3FileReaderBase(mp3File, waveFormat => new AcmMp3FrameDecompressor(waveFormat)))
            {
                WaveFileWriter.CreateWaveFile(wavFile, mp3FileReader);
            }

            return wavFile;
        }

        private static string ConvertOGGToWem(string oggFile, string appStartupPath)
        {
            string oggFileName = Path.GetFileNameWithoutExtension(oggFile);
            string directory = Path.GetDirectoryName(oggFile);
            string oggFullPath = Path.Combine(directory, oggFileName);
            string oggPreviewName = oggFullPath + "_preview";

            string wemFile = OggFile.Convert2Wem(oggFile);

            SafeDelete(oggFullPath + ".wav");
            SafeDelete(oggPreviewName + ".ogg");
            SafeDelete(oggPreviewName + ".wav");
            SafeDelete(oggPreviewName + ".wem");

            string destWemName = Path.GetFileNameWithoutExtension(wemFile) + ".wem";
            string finalPath = Path.Combine(appStartupPath, destWemName);

            if (!string.Equals(Path.GetFullPath(wemFile), Path.GetFullPath(finalPath), StringComparison.OrdinalIgnoreCase))
                File.Move(wemFile, finalPath, true);

            return finalPath;
        }

        private static string ConvertWAVToWem(string wavFile, string appStartupPath)
        {
            string previewWav = Path.Combine(Path.GetDirectoryName(wavFile), Path.GetFileNameWithoutExtension(wavFile) + "_preview.wav");

            SafeDelete(previewWav);
            File.Copy(wavFile, previewWav);

            string outputWem = Path.Combine(appStartupPath, Path.GetFileNameWithoutExtension(wavFile) + ".wem");
            Wwise.Wav2Wem(wavFile, outputWem, 4);

            SafeDelete(previewWav);
            SafeDelete(Path.Combine(appStartupPath, Path.GetFileNameWithoutExtension(wavFile) + "_preview.wem"));

            return outputWem;
        }

        public static void ConvertSoundAndReplace(string soundToReplace, string sourceFile, string rsModsPath)
        {
            string temporaryFolder = CreateTemporaryFolder();
            try
            {
                string workingSource = Path.Combine(temporaryFolder, Path.GetFileName(sourceFile));
                File.Copy(sourceFile, workingSource);

                string convertedFile = Path.GetExtension(workingSource).ToLowerInvariant() switch
                {
                    ".mp3" => ConvertWAVToWem(ConvertMP3ToWav(workingSource), temporaryFolder),
                    ".ogg" => ConvertOGGToWem(workingSource, temporaryFolder),
                    ".wav" => ConvertWAVToWem(workingSource, temporaryFolder),
                    ".wem" => workingSource,
                    _ => throw new NotSupportedException("Sound files must use the MP3, OGG, WAV, or WEM format.")
                };

                if (!File.Exists(convertedFile))
                    throw new InvalidOperationException("Conversion failed because the converted audio file was not created.");

                string destination = Path.Combine(rsModsPath, soundToReplace);
                File.Copy(convertedFile, destination, true);
            }
            finally
            {
                DeleteTemporaryFolder(temporaryFolder);
            }
        }

        public static void ImportSoundFile(string fileName)
        {
            SevenZipExtractor.SetLibraryPath(SevenZipLibraryPath);
            using SevenZipExtractor extractor = new(fileName);

            // Extract under the RSMods folder (where the unpacked audio.psarc lives and where Export reads
            // from), not the current working directory. The original WinForms code extracted CWD-relative,
            // which only lined up because that build ran from the RSMods folder.
            extractor.ExtractArchive(Path.Combine(GenUtil.GetRsModsPath(), SoundPackLocationPrefix));
        }

        public static void ExportSoundFile(string fileName)
        {
            SevenZipCompressor.SetLibraryPath(SevenZipLibraryPath);

            SevenZipCompressor compressor = new()
            {
                CompressionMethod = CompressionMethod.Deflate,
                CompressionLevel = CompressionLevel.Normal,
                CompressionMode = CompressionMode.Create,
                DirectoryStructure = true,
                PreserveDirectoryRoot = false,
                ArchiveFormat = OutArchiveFormat.Zip
            };

            Dictionary<string, string> exportedFiles = [];

            string[] voiceLines = {
                VoiceLines.BadPerformance, VoiceLines.DisappointingPerformance, VoiceLines.SubparPerformance,
                VoiceLines.CouldBeBetter, VoiceLines.DecentPerformance, VoiceLines.AlrightPerformance,
                VoiceLines.ExcellentPerformance, VoiceLines.TopNotchPerformance, VoiceLines.SuperbPerformance,
                VoiceLines.DazzlingPerformance, VoiceLines.YoureGonnaBeASuperstar, VoiceLines.WonderfulPerformance,
                VoiceLines.ExceptionalPerformance, VoiceLines.AmazingPerformance, VoiceLines.ExemplaryPerformance,
                VoiceLines.MasterfulPerformance_98, VoiceLines.MasterfulPerformance_99, VoiceLines.FlawlessPerformance
            };

            foreach (var voiceLine in voiceLines)
            {
                string zipPath = SoundPackEnglishPrefix + voiceLine;
                string physicalPath = Path.Combine(GenUtil.GetRsModsPath(), SoundPackLocationPrefix, SoundPackEnglishPrefix, voiceLine);

                if (File.Exists(physicalPath))
                {
                    exportedFiles.Add(zipPath, physicalPath);
                }
            }

            if (exportedFiles.Count == 0)
            {
                throw new Exception("No unpacked audio files were found to export. Make sure you have unpacked the audio first!");
            }

            compressor.CompressFileDictionary(exportedFiles, fileName);
        }

        public static void Reset()
        {
            string temporaryFolder = CreateTemporaryFolder();
            try
            {
                GenUtil.ExtractEmbeddedResource(temporaryFolder, typeof(Soundpacks).Assembly, "RSMods.Core.Resources", ["original.rs_soundpack"]);
                ImportSoundFile(Path.Combine(temporaryFolder, "original.rs_soundpack"));
            }
            finally
            {
                DeleteTemporaryFolder(temporaryFolder);
            }
        }

        private static string CreateTemporaryFolder()
        {
            string path = Path.Combine(Path.GetTempPath(), "RSMods", "SoundPacks", Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(path);
            return path;
        }

        private static void DeleteTemporaryFolder(string path)
        {
            try
            {
                if (Directory.Exists(path))
                    Directory.Delete(path, true);
            }
            catch (IOException)
            {
                // Cleanup must not hide the conversion/import result.
            }
            catch (UnauthorizedAccessException)
            {
                // Cleanup must not hide the conversion/import result.
            }
        }
    }
}
