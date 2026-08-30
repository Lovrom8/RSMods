namespace RSMods.Core
{
    /// <summary>
    /// UI-agnostic host environment: process-level facts and actions that in WinForms live on
    /// <c>System.Windows.Forms.Application</c> (StartupPath, Exit) and have no cross-platform
    /// equivalent. Implemented by a WinForms adapter today, an Avalonia lifetime later.
    /// </summary>
    public interface IAppEnvironment
    {
        /// <summary>The application's base directory (replaces <c>Application.StartupPath</c>).</summary>
        string BaseDirectory { get; }

        /// <summary>Requests an orderly application shutdown (replaces <c>Application.Exit()</c>).</summary>
        void RequestShutdown();
    }
}
