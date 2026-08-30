using NAudio.Wave;
using RocksmithToolkitLib.Ogg;
using RocksmithToolkitLib.XmlRepository;
using RSMods.Util;
using SevenZip;
using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Windows.Forms;

namespace RSMods.Rocksmith
{
    internal static class Soundpacks
    {
        internal static class VoiceLines
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
    
        private static void SafeDelete(string path)
        {
            if (File.Exists(path)) File.Delete(path);
        }

        private static string ConvertMP3ToWav(string mp3File)
        {
            string wavFile = Path.Combine(Path.GetDirectoryName(mp3File), Path.GetFileNameWithoutExtension(mp3File) + ".wav");

            using (Mp3FileReader mp3FileReader = new(mp3File))
            {
                WaveFileWriter.CreateWaveFile(wavFile, mp3FileReader);
            }

            return wavFile;
        }

        public static string ConvertOGGToWem(string oggFile, string appStartupPath)
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

            if (File.Exists(finalPath)) File.Delete(finalPath);

            // The lib now properly disposes of the OggStream, but let's keep this call here
            GC.Collect();
            GC.WaitForPendingFinalizers();

            File.Move(wemFile, finalPath);

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
            string currentFile = sourceFile;
            string extension = Path.GetExtension(currentFile).ToLower();

            if (extension == ".mp3")
                currentFile = ConvertMP3ToWav(currentFile);

            extension = Path.GetExtension(currentFile).ToLower();

            if (extension == ".ogg")
                currentFile = ConvertOGGToWem(currentFile, rsModsPath);
            else if (extension == ".wav")
                currentFile = ConvertWAVToWem(currentFile, rsModsPath);

            if (currentFile != "null" && File.Exists(currentFile))
            {
                string destination = Path.Combine(rsModsPath, soundToReplace);

                if (File.Exists(destination))
                    File.Delete(destination);

                GC.Collect();
                GC.WaitForPendingFinalizers();

                File.Move(currentFile, destination);
            }
            else
            {
                throw new Exception("Conversion failed. The resulting file path was null or missing.");
            }
        }

        public static void ImportSoundFile(string fileName)
        {
            SevenZipExtractor.SetLibraryPath("7z64.dll");
            using SevenZipExtractor extractor = new(fileName);
            extractor.ExtractArchive(SoundPackLocationPrefix);
        }

        public static void ExportSoundFile(string fileName)
        {
            SevenZipCompressor.SetLibraryPath("7z64.dll");

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
            GenUtil.ExtractEmbeddedResource(Application.StartupPath, Assembly.GetExecutingAssembly(), "RSMods.Resources", ["original.rs_soundpack"]);
            ImportSoundFile("original.rs_soundpack");
            File.Delete("original.rs_soundpack");
        }
    }
}
