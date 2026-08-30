using System.Reflection;
using RS2014_Mod_Installer.Payload;

namespace RS2014_Mod_Installer;

internal sealed class EmbeddedInstallerResources
{
    private readonly Assembly _assembly = typeof(EmbeddedInstallerResources).Assembly;

    public PayloadManifest ReadManifest()
    {
        using Stream stream = Open("RSModsGUI.manifest.json");
        using var reader = new StreamReader(stream);
        return PayloadManifest.Parse(reader.ReadToEnd());
    }

    public Stream OpenPayload() => Open("RSModsGUI.zip");
    public byte[] ReadNativeDll() => ReadBytes("xinput1_3.dll");
    public byte[] ReadNativePdb() => ReadBytes("xinput1_3.pdb");

    private Stream Open(string logicalName) =>
        _assembly.GetManifestResourceStream(logicalName)
        ?? throw new PayloadInstallException(
            PayloadInstallFailure.CorruptPayload,
            $"The installer is missing its embedded resource ({logicalName}).");

    private byte[] ReadBytes(string logicalName)
    {
        using Stream stream = Open(logicalName);

        using var buffer = new MemoryStream();
        stream.CopyTo(buffer);

        return buffer.ToArray();
    }
}
