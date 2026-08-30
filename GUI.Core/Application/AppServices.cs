namespace RSMods.Core
{
    /// <summary>
    /// Composition root for the UI-agnostic services. The host sets these once at startup
    /// (WinForms today, an Avalonia lifetime later). Consumers that cannot take constructor or
    /// parameter injection may read from here; prefer explicit injection where practical.
    /// </summary>
    public static class AppServices
    {
        public static IDialogService Dialogs { get; private set; }
        public static IAppEnvironment Environment { get; private set; }

        public static void Initialize(IDialogService dialogs, IAppEnvironment environment)
        {
            Dialogs = dialogs;
            Environment = environment;
        }
    }
}
