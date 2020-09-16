using SevenZip;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading;
using System.Windows.Forms;

namespace RSMods.Util
{
    // Originally written by cozy1
    public static class ZipUtilities
    {
        /// <summary>
        /// The lock for SevenZipSharp initialization
        /// </summary>
        private static readonly object zlibLock = new object();

        /// <summary>
        /// Flag if the SevenZipShparp library as initalized.
        /// </summary>
        private static bool zlibInitalized = false;

        /// <summary>
        /// Compresses a directory using the .zip format.
        /// </summary>
        /// <param name="sourceDirectoryPath">The directory to compress.</param>
        /// <param name="archiveName">The name of the archive to be created.</param>
        /// <param name="archiveFormat">Default is .zip</param>
        /// <param name="preserveRoot">Preserve the Root Source Directory Path inside the archive (default = false)</param>
        public static bool ZipDirectory(string sourceDirectory, string archiveName, OutArchiveFormat archiveFormat = OutArchiveFormat.Zip, bool preserveRoot = false)
        {
            SetupZlib();

            try
            {
                SevenZipCompressor compressor = new SevenZipCompressor();

                if (String.IsNullOrEmpty(Path.GetExtension(archiveName)))
                    compressor.ArchiveFormat = archiveFormat;

                if (preserveRoot)
                    compressor.PreserveDirectoryRoot = true;

                compressor.TempFolderPath = Path.GetTempPath();
                compressor.IncludeEmptyDirectories = true;
                compressor.CompressDirectory(sourceDirectory, archiveName, "");
            }
            catch
            {
                return false;
            }

            return true;
        }

        /// <summary>
        /// Compresses a file using the .zip format.
        /// </summary>
        /// <param name="sourceFilePath">The file to compress.</param>
        /// <param name="archiveName">The name of the archive to be created.</param>
        /// <param name="archiveFormat">Default is .zip</param>
        public static bool ZipFile(string sourceFilePath, string archiveName, OutArchiveFormat archiveFormat = OutArchiveFormat.Zip)
        {
            SetupZlib();

            try
            {
                var sourceFilePaths = new string[] { sourceFilePath };
                SevenZipCompressor compressor = new SevenZipCompressor();
                compressor.ArchiveFormat = archiveFormat;
                compressor.CompressFiles(archiveName, sourceFilePaths[0]);
            }
            catch
            {
                return false;
            }

            return true;
        }

        /// <summary>
        /// Compress a single file and append/replace existing compressed file .zip format.
        /// </summary>
        /// <param name="sourceFilePath">The file to compress and inject.</param>
        /// <param name="archiveName">The name (root path) of the archive to be appended/create to.</param>
        /// <param name="compressionMode">Append or Create</param>
        /// <param name="internalArchivePath">internal archive path with file name (default = root level)</param>
        /// <param name="archiveFormat">Default is .zip</param>
        public static bool InjectFile(string sourceFilePath, string archiveName, string internalArchivePath = "", OutArchiveFormat archiveFormat = OutArchiveFormat.Zip, CompressionMode compressionMode = CompressionMode.Append)
        {
            SetupZlib();

            // -- SUPER CRITICAL CODE HERE -- 
            try
            {
                if (String.IsNullOrEmpty(internalArchivePath))
                    internalArchivePath = Path.GetFileName(sourceFilePath);

                // find and delete an existing file from archive
                ReadOnlyCollection<string> ss = null;
                using (SevenZipExtractor extractor = new SevenZipExtractor(archiveName))
                    ss = extractor.ArchiveFileNames;

                Dictionary<int, string> updateFile = new Dictionary<int, string>();
                if (ss != null)
                {
                    for (int ndx = 0; ndx < ss.Count; ndx++)
                        if (ss[ndx].Equals(internalArchivePath)) // Contains(Path.GetFileName(sourceFilePath)))
                        {
                            updateFile.Add(ndx, null);
                            break;
                        }
                }

                SevenZipCompressor compressor = new SevenZipCompressor();
                compressor.ArchiveFormat = archiveFormat;
                compressor.CompressionMode = compressionMode;
                compressor.TempFolderPath = Path.GetTempPath();

                if (updateFile.Count > 1)
                    throw new Exception();
                if (updateFile.Count == 1)
                    compressor.ModifyArchive(archiveName, updateFile);

                // inject source file to archive
                Dictionary<string, string> injector = new Dictionary<string, string>();
                injector.Add(internalArchivePath, sourceFilePath);
                compressor.CompressFileDictionary(injector, archiveName);
            }
            catch (Exception)
            {
                return false;
            }

            return true;
        }

        /// <summary>
        /// Extracts all data from a .zip archive to an output directory path.
        /// </summary>
        /// <param name="zipArchivePath">The file (archive) to extract data from.</param>
        /// <param name="outputDirPath">The directory to put the extracted data into.</param>
        public static bool UnzipDir(string zipArchivePath, string outputDirPath, InArchiveFormat archiveFormat = InArchiveFormat.Zip)
        {
            SetupZlib();

            zipArchivePath = Path.GetFullPath(zipArchivePath);
            outputDirPath = Path.GetFullPath(outputDirPath);

            try
            {
                using (SevenZipExtractor extractor = new SevenZipExtractor(zipArchivePath))
                {
                    extractor.ExtractArchive(outputDirPath);
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine("Error Unzipping: " + ex.Message + Environment.NewLine +
                    "Make sure correct version of 7z.dll is used.");
                return false;
            }

            return true;
        }

        /// <summary>
        /// Extracts a single file from a .zip archive.
        /// </summary>
        /// <param name="targetDir">The directory to put the extracted file in.</param>
        /// <param name="zipFile">The file (archive) to extract data from.</param>
        /// <param name="fileName">The file to extract.</param>
        public static bool ExtractSingleFile(string targetDir, string zipFile, string fileName)
        {
            SetupZlib();
            try
            {
                using (SevenZipExtractor extractor = new SevenZipExtractor(zipFile))
                {
                    var filePath = Path.Combine(targetDir, Path.GetFileName(fileName));
                    using (FileStream fs = new FileStream(filePath, FileMode.Create))
                    {
                        extractor.ExtractFile(fileName, fs);
                    }
                }
            }
            catch
            {
                return false;
            }

            return true;
        }

        /// <summary>
        /// Extracts file names from a .zip archive.
        /// </summary>
        /// <param name="zipArchivePath">The zip archive to extract file names from.</param>
        public static List<string> ExtractFileNames(string zipArchivePath)
        {
            List<string> fileNames = new List<string>();
            SetupZlib();

            try
            {
                using (SevenZipExtractor extractor = new SevenZipExtractor(zipArchivePath))
                {
                    fileNames = extractor.ArchiveFileNames.ToList();
                }
            }
            catch
            {
                return fileNames;
            }

            return fileNames;
        }

        /// <summary>
        /// Setups the zlib library; gets the proper 32 or 64 bit library as a stream from a resource, and loads it.
        /// </summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId = "Zlib", Justification = "Zlib is a spelled correctly")]
        public static void SetupZlib()
        {
            if (zlibInitalized)
            {
                return;
            }

            lock (zlibLock)
            {
                if (zlibInitalized)
                {
                    return;
                }

                //    Stream stream = null;
                //    Assembly asm = Assembly.GetExecutingAssembly();
                //    string libraryPath = string.Empty;

                //    if (IntPtr.Size == 8)
                //    {
                //        libraryPath = Path.Combine(Path.GetDirectoryName(Assembly.GetAssembly(typeof(SevenZipExtractor)).Location), @"7z64.dll");
                //        stream = asm.GetManifestResourceStream("libs.7z64.dll");
                //    }
                //    else
                //    {
                //        libraryPath = Path.Combine(Path.GetDirectoryName(Assembly.GetAssembly(typeof(SevenZipExtractor)).Location), @"7z86.dll");
                //        stream = asm.GetManifestResourceStream("libs.7z86.dll");
                //    }

                //    if (!File.Exists(libraryPath))
                //    {
                //        byte[] myAssembly = new byte[stream.Length];
                //        stream.Read(myAssembly, 0, (int)stream.Length);
                //        File.WriteAllBytes(libraryPath, myAssembly);
                //        stream.Close();
            }

            // "7za.dll" only supports 7Zip archives
            // use "7z.dll" to support many archive formats 
            var libraryPath = Path.Combine(Path.GetDirectoryName(Application.ExecutablePath), "7z.dll");

            SevenZipBase.SetLibraryPath(libraryPath);

            zlibInitalized = true;
        }

        /// <summary>
        /// Try to write a file to a directory to make sure writing is allowed.
        /// </summary>
        /// <param name="directory"> The directory to write in.</param>
        public static bool EnsureWritableDirectory(string directory)
        {
            try
            {
                string testFile = Path.Combine(directory, "temp.txt");
                File.WriteAllText(testFile, @"Testing Writable Directory ...");
                File.Delete(testFile);
            }
            catch
            {
                return false;
            }

            return true;
        }

        public static void DirectoryCopy(string sourceDirName, string destDirName, bool copySubDirs)
        {
            DirectoryInfo dir = new DirectoryInfo(sourceDirName);
            DirectoryInfo[] dirs = dir.GetDirectories();

            // If the source directory does not exist, throw an exception.
            if (!dir.Exists)
            {
                throw new DirectoryNotFoundException("Source directory does not exist or could not be found: " + sourceDirName);
            }

            // If the destination directory does not exist, create it.
            if (!Directory.Exists(destDirName))
            {
                Directory.CreateDirectory(destDirName);
            }

            // Get the file contents of the directory to copy.
            FileInfo[] files = dir.GetFiles();

            foreach (FileInfo file in files)
            {
                // Create the path to the new copy of the file.
                string temppath = Path.Combine(destDirName, file.Name);

                // Copy the file.
                file.CopyTo(temppath, false);
            }

            // If copySubDirs is true, copy the subdirectories.
            if (copySubDirs)
            {
                foreach (DirectoryInfo subdir in dirs)
                {
                    // Create the subdirectory.
                    string temppath = Path.Combine(destDirName, subdir.Name);

                    // Copy the subdirectories.
                    DirectoryCopy(subdir.FullName, temppath, copySubDirs);
                }
            }
        }

        public static void RemoveReadOnlyAttribute(string dir)
        {
            string[] files = Directory.GetFiles(dir, "*.*", SearchOption.AllDirectories);
            foreach (string file in files)
            {
                FileAttributes attribute = File.GetAttributes(file);
                File.SetAttributes(file, attribute & ~FileAttributes.ReadOnly);
            }
        }

        //from: http://stackoverflow.com/questions/329355/cannot-delete-directory-with-directory-deletepath-true
        // delete directories that still may be open or not empty
        public static void DeleteDirectory(string dirPath, bool includeSubDirs = true)
        {
            const int magicDust = 10;
            for (var gnomes = 1; gnomes <= magicDust; gnomes++)
            {
                try
                {
                    Directory.Delete(dirPath, includeSubDirs);
                }
                catch (DirectoryNotFoundException)
                {
                    return; // ok!
                }
                catch (IOException)
                {
                    // System.IO.IOException: The directory is not empty
                    Debug.WriteLine("Gnomes prevent deletion of {0}! Applying magic dust, attempt #{1}.", dirPath, gnomes);
                    Thread.Sleep(50);
                    continue;
                }
                return;
            }
        }

        /// <summary>
        /// Extracts all data from a .rar archive to an output directory path.
        /// </summary>
        /// <param name="rarArchivePath">The file (archive) to extract data from.</param>
        /// <param name="outputDirPath">The directory to put the extracted data into.</param>
        public static bool UnrarDir(string rarArchivePath, string outputDirPath, InArchiveFormat archiveFormat = InArchiveFormat.Rar)
        {
            rarArchivePath = Path.GetFullPath(rarArchivePath);
            outputDirPath = Path.GetFullPath(outputDirPath);

            try
            {
                using (SevenZipExtractor extractor = new SevenZipExtractor(rarArchivePath))
                {
                    extractor.ExtractArchive(outputDirPath);
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine("<Error> UnrarDir - " + ex.Message);
                return false;
            }

            return true;
        }
    }
}
