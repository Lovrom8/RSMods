using System;

namespace RS2014_Mod_Installer.Payload
{
    // A payload extracted and verified by ConfiguratorPayloadInstaller.Stage, not yet deployed. Lets the caller check
    // the payload is good before it changes anything else in the Rocksmith folder.
    public sealed class StagedConfiguratorPayload : IDisposable
    {
        private readonly string _installFolder;
        private readonly string _stagingFolder;
        private readonly PayloadManifest _manifest;

        internal StagedConfiguratorPayload(string installFolder, string stagingFolder, PayloadManifest manifest)
        {
            _installFolder = installFolder;
            _stagingFolder = stagingFolder;
            _manifest = manifest;
        }

        /// <summary>Deploys the staged files over the install.</summary>
        /// <exception cref="PayloadInstallException">On any failure; the prior install is left usable.</exception>
        public void Deploy(Action<string> progress = null) =>
            ConfiguratorPayloadInstaller.DeployStaged(_stagingFolder, _installFolder, _manifest, progress);

        public void Dispose() => ConfiguratorPayloadInstaller.DeleteStaging(_stagingFolder);
    }
}
