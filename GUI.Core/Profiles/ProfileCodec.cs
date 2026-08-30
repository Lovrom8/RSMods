using MiscUtil.Conversion;
using MiscUtil.IO;
using System;
using System.IO;
using System.Security.Cryptography;
using System.Text;
using zlib;

namespace RSMods
{
    /// <summary>A decoded Rocksmith profile together with the user id required to encrypt it again.</summary>
    public sealed class DecodedProfile
    {
        internal DecodedProfile(string json, byte[] userId)
        {
            Json = json;
            UserId = userId;
        }

        public string Json { get; }
        internal byte[] UserId { get; }
    }

    /// <summary>
    /// Encodes and decodes Rocksmith's EVAS profile envelope. This service owns no selected-profile
    /// state, so inspecting another profile cannot accidentally change the user id used for saving.
    /// </summary>
    public sealed class ProfileCodec
    {
        private static readonly byte[] PcSaveKey =
        [
            0x72, 0x8B, 0x36, 0x9E, 0x24, 0xED, 0x01, 0x34,
            0x76, 0x85, 0x11, 0x02, 0x18, 0x12, 0xAF, 0xC0,
            0xA3, 0xC2, 0x5D, 0x02, 0x06, 0x5F, 0x16, 0x6B,
            0x4B, 0xCC, 0x58, 0xCD, 0x26, 0x44, 0xF2, 0x9E
        ];

        private static readonly byte[] SaveHeader =
        [
            0x45, 0x56, 0x41, 0x53,
            0x01, 0x00, 0x00, 0x00
        ];

        private static readonly byte[] EndOfSaveHeader =
        [
            0x00, 0x00, 0x10, 0x01
        ];

        public DecodedProfile Decode(string path, bool dumpToFile = false, string dumpFile = "profileDump.json")
        {
            using Stream input = File.OpenRead(path);
            using var output = new MemoryStream();

            byte[] userId = DecodeEnvelope(input, output);
            using var reader = new StreamReader(output, Encoding.UTF8, true, 1024, leaveOpen: true);
            string json = reader.ReadToEnd();

            if (dumpToFile)
                File.WriteAllText(dumpFile, json);

            return new DecodedProfile(json, userId);
        }

        public void Encode(string profileJson, string fileName, DecodedProfile source)
        {
            if (source == null)
                throw new ArgumentNullException(nameof(source));

            Encode(profileJson, fileName, source.UserId);
        }

        internal void Encode(string profileJson, string fileName, byte[] userId)
        {
            if (userId == null || userId.Length != 4)
                throw new InvalidDataException("Rocksmith profile user id must contain four bytes.");

            if (profileJson.EndsWith("\r\n", StringComparison.Ordinal))
                profileJson = profileJson.Remove(profileJson.LastIndexOf("\r\n", StringComparison.Ordinal));

            byte[] profileBytes = Encoding.UTF8.GetBytes(profileJson);

            using var encrypted = new MemoryStream();
            using var compressed = new MemoryStream();
            using var writer = new EndianBinaryWriter(EndianBitConverter.Little, encrypted);

            writer.Write(SaveHeader, 0, SaveHeader.Length);
            writer.Write(userId, 0, userId.Length);
            writer.Write(EndOfSaveHeader, 0, EndOfSaveHeader.Length);
            writer.Write((uint)profileBytes.Length);

            Zip(profileBytes, compressed, profileBytes.Length);
            Encrypt(compressed, encrypted, PcSaveKey);
            writer.Write(encrypted.ToArray());
            writer.Flush();
            writer.BaseStream.Seek(0, SeekOrigin.Begin);

            using Stream destination = File.Create(fileName);
            writer.BaseStream.CopyTo(destination);
        }

        private static byte[] DecodeEnvelope(Stream input, Stream output)
        {
            var littleEndian = EndianBitConverter.Little;
            var bigEndian = EndianBitConverter.Big;

            input.Position = 0;
            using var decrypted = new MemoryStream();
            using var reader = new EndianBinaryReader(littleEndian, input);
            using var decryptedReader = new EndianBinaryReader(bigEndian, decrypted);

            reader.ReadBytes(8); // EVAS + version
            byte[] userId = reader.ReadBytes(4);
            reader.ReadBytes(4);
            reader.ReadUInt32(); // uncompressed length
            Decrypt(reader.BaseStream, decrypted, PcSaveKey);

            ushort zlibHeader = decryptedReader.ReadUInt16();
            decryptedReader.BaseStream.Position -= sizeof(ushort);
            if (zlibHeader != 30938)
                throw new InvalidDataException("Profile payload does not contain the expected zlib header.");

            Unzip(decryptedReader.BaseStream, output);
            return userId;
        }

        private static void InitializeCipher(SymmetricAlgorithm cipher, byte[] key, CipherMode mode)
        {
            cipher.Padding = PaddingMode.None;
            cipher.Mode = mode;
            cipher.BlockSize = 128;
            cipher.IV = new byte[16];
            cipher.Key = key;
        }

        private static void Transform(
            Stream input,
            Stream output,
            ICryptoTransform transform,
            long length,
            bool padToBufferBoundary)
        {
            var buffer = new byte[512];
            int remainder = (int)(length % buffer.Length);
            int padding = padToBufferBoundary && remainder != 0 ? buffer.Length - remainder : 0;
            var coder = new CryptoStream(output, transform, CryptoStreamMode.Write);
            long totalRead = 0;

            while (totalRead < length)
            {
                int size = (int)Math.Min(length - totalRead, buffer.Length);
                int bytesRead = input.Read(buffer, 0, size);
                if (bytesRead == 0)
                    break;

                coder.Write(buffer, 0, bytesRead);
                totalRead += bytesRead;
            }

            if (padding > 0 && padding < buffer.Length)
                coder.Write(new byte[padding], 0, padding);

            coder.FlushFinalBlock();
            output.Flush();

            if (output.CanSeek)
                output.Seek(0, SeekOrigin.Begin);
        }

        private static void Decrypt(Stream input, Stream output, byte[] key)
        {
#pragma warning disable SYSLIB0022 // Preserve the format's Rijndael implementation across net48/net8.
            using var cipher = new RijndaelManaged();
#pragma warning restore SYSLIB0022
            InitializeCipher(cipher, key, CipherMode.ECB);
            // The EVAS header has already been consumed; only the remaining encrypted payload
            // participates in block decryption. Using the full file length pads the ciphertext
            // with header-sized garbage and can produce an invalid final block.
            Transform(
                input,
                output,
                cipher.CreateDecryptor(),
                input.Length - input.Position,
                padToBufferBoundary: false);
        }

        private static void Encrypt(Stream input, Stream output, byte[] key)
        {
#pragma warning disable SYSLIB0022 // Preserve the format's Rijndael implementation across net48/net8.
            using var cipher = new RijndaelManaged();
#pragma warning restore SYSLIB0022
            InitializeCipher(cipher, key, CipherMode.ECB);
            Transform(input, output, cipher.CreateEncryptor(), input.Length, padToBufferBoundary: true);
        }

        private static void Unzip(Stream input, Stream output)
        {
            var buffer = new byte[65536];
            var zlibInput = new ZInputStream(input);
            int length;

            while ((length = zlibInput.read(buffer, 0, buffer.Length)) > 0)
                output.Write(buffer, 0, length);

            zlibInput.Close();
            output.Position = 0;
            output.Flush();
        }

        private static long Zip(byte[] bytes, Stream output, long plainLength)
        {
            using var input = new MemoryStream(bytes);
            var buffer = new byte[65536];
            var zlibOutput = new ZOutputStream(output, 9);
            long totalRead = 0;

            while (totalRead < plainLength)
            {
                int size = (int)Math.Min(plainLength - totalRead, buffer.Length);
                int bytesRead = input.Read(buffer, 0, size);
                if (bytesRead == 0)
                    break;

                zlibOutput.Write(buffer, 0, bytesRead);
                totalRead += bytesRead;
            }

            zlibOutput.finish();
            output.Flush();
            if (output.CanSeek)
                output.Position = 0;

            return zlibOutput.TotalOut;
        }
    }
}
