using System;
using System.Windows.Forms;
using RSMods.Core;

namespace RSMods.WinForms
{
    /// <summary>WinForms implementation of <see cref="IAppEnvironment"/>.</summary>
    public class WinFormsAppEnvironment : IAppEnvironment
    {
        public string BaseDirectory => Application.StartupPath;

        public void RequestShutdown()
        {
            // During startup resolution the request can arrive before Application.Run has started a message
            // loop, where Application.Exit() is a no-op and the form would still be shown. Terminate the
            // process directly in that case; otherwise shut the running message loop down gracefully.
            if (Application.MessageLoop)
                Application.Exit();
            else
                Environment.Exit(0);
        }
    }
}
