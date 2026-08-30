namespace RSMods.Core
{
    /// <summary>
    /// UI-agnostic application-lifetime action used by shared startup resolution.
    /// </summary>
    public interface IAppEnvironment
    {
        /// <summary>Requests an orderly application shutdown.</summary>
        void RequestShutdown();
    }
}
