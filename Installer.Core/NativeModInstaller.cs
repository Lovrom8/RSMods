using System.IO;

namespace RS2014_Mod_Installer.Core
{
    public enum NativeModInstallStatus
    {
        Success,
        GameRunning  // A file could not be written because Rocksmith is open and holds a lock.
    }

    // Deploys the native mod DLL (xinput1_3.dll + .pdb) into the Rocksmith folder. The DLL bytes are
    // supplied by the caller (embedded in the installer resources) so this stays UI- and
    // resource-free and testable. Was DLLStuff.InjectDLL.
    public static class NativeModInstaller
    {
        public static NativeModInstallStatus Install(string rocksmithLocation, byte[] xinput1_3Dll, byte[] xinput1_3Pdb)
        {
            try
            {
                File.WriteAllBytes(Path.Combine(rocksmithLocation, "xinput1_3.dll"), xinput1_3Dll);
                File.WriteAllBytes(Path.Combine(rocksmithLocation, "xinput1_3.pdb"), xinput1_3Pdb);

                // A large D3DX9_42.dll is a leftover injector from older installs; remove it so ours loads.
                string legacyProxy = Path.Combine(rocksmithLocation, "D3DX9_42.dll");
                if (File.Exists(legacyProxy) && new FileInfo(legacyProxy).Length >= 300000)
                    File.Delete(legacyProxy);

                return NativeModInstallStatus.Success;
            }
            catch (IOException)
            {
                return NativeModInstallStatus.GameRunning;
            }
        }
    }
}
