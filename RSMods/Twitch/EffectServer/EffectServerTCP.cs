using Newtonsoft.Json;
using RSMods.Util;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace RSMods.Twitch.EffectServer
{
    class EffectServerTCP
    {
        private TcpListener tcpListener;
        private Thread tcpListenerThread;
        private CancellationTokenSource cts;
        private TcpClient connectedTcpClient;

        private int effectId = 0;
        private Dictionary<int, TwitchReward> usedRewards;
        private Dictionary<int, TwitchReward> remainingRewards;
        private int retryInterval = 5; // In seconds

        private string ipAdr = "127.0.0.1";
        private int port = 45659;

        public EffectServerTCP()
        {
            StartServer();
        }

        private void StartServer()
        {
            tcpListenerThread = new Thread(new ThreadStart(ListenForIncomingResponse));
            tcpListenerThread.IsBackground = true;
            tcpListenerThread.Start();

            cts = new CancellationTokenSource();
            HandleRemainingEffects(cts.Token);

            usedRewards = new Dictionary<int, TwitchReward>();
            remainingRewards = new Dictionary<int, TwitchReward>();

            TwitchSettings.Get.AddToLog("Started the effect server");
        }

        private void ListenForIncomingResponse()
        {
            try
            {
                tcpListener = new TcpListener(IPAddress.Parse(ipAdr), port);
                tcpListener.Start();

                Debug.Write("Server is listening");
                Byte[] bytes = new Byte[1024];

                while (true)
                {
                    using (connectedTcpClient = tcpListener.AcceptTcpClient())
                    {
                        using (NetworkStream stream = connectedTcpClient.GetStream())
                        {
                            int length;

                            while ((length = stream.Read(bytes, 0, bytes.Length)) != 0)
                            {
                                var incomingData = new byte[length];
                                Array.Copy(bytes, 0, incomingData, 0, length);

                                string clientMessage = Encoding.ASCII.GetString(incomingData);
                                Debug.Write($"Recived message from the game: {clientMessage}");

                                if (clientMessage != "\0")
                                {
                                    var response = JsonConvert.DeserializeObject<Response>(clientMessage);

                                    if (response.status == 3 && !remainingRewards.ContainsKey(response.id)) // If retry code was returned, put it in the queue
                                        remainingRewards.Add(response.id, usedRewards[response.id]);

                                    if (response.status == 0 && remainingRewards.ContainsKey(response.id)) // If the effect has been executed sucessfully and it had been placed in the queue, remove it
                                        remainingRewards.Remove(response.id);
                                }
                            }
                        }
                    }
                }
            }
            catch (SocketException socketException)
            {
                Debug.Write($"SocketException: {socketException.Message}");
            }
            catch (IOException) // If the game (and the connection with it) was closed, restart the server
            {
                tcpListener.Stop();
                connectedTcpClient.Close();

                StartServer();
            }
        }

        private void HandleRemainingEffects(CancellationToken ct)
        {
            Task.Run(async () =>
            {
                while (!ct.IsCancellationRequested)
                {
                    foreach (var reward in remainingRewards)
                        AddEffectToQueue(reward.Value, false);

                    await Task.Delay(TimeSpan.FromSeconds(retryInterval), ct);
                }
            }, ct);
        }

        private void SendMessageToGame(string message)
        {
            if (connectedTcpClient == null)
                return;

            try
            {
                NetworkStream stream = connectedTcpClient.GetStream();
                if (!stream.CanWrite)
                    return;

                var serverMessageAsByteArray = new List<byte>();
                serverMessageAsByteArray.AddRange(Encoding.ASCII.GetBytes(message));
                serverMessageAsByteArray.Add((byte)0);

                stream.Write(serverMessageAsByteArray.ToArray(), 0, serverMessageAsByteArray.Count());

                Debug.Write($"Sent a message to the game: {message}");
            }
            catch (SocketException socketException)
            {
                Debug.Write($"Socket exception: {socketException}");
            }
        }

        public async void AddEffectToQueue(TwitchReward reward, bool newEffect = true)
        {
            if (connectedTcpClient == null)
                WinMsgUtil.SendMsgToRS("Reconnect to CC");

            await Task.Delay(2000);

            if (connectedTcpClient == null) // If we are still unable to connect, something is very wrong
            {
                MessageBox.Show("Unable to connect to the effect server, please restart the game and RSMods!", "Error");
                return;
            }

            NetworkStream stream = connectedTcpClient.GetStream();
            if (!stream.CanWrite)
                StartServer();

            if (newEffect)
            {
                effectId++;
                usedRewards.Add(effectId, reward);
            }

            Request request = new Request()
            {
                id = effectId,
                code = reward.InternalMsgEnable.ToLower(),
                //code = "solidcustomrgb",
                type = 1,
                viewer = "rsmods"
            };
            request.parameters = new List<object>();

            if (reward.AdditionalMsg != null && reward.AdditionalMsg != "Random" && reward.AdditionalMsg.Length == 6) // If it's a solid color effect
            {
                request.parameters.Add(Convert.ToInt32(reward.AdditionalMsg.Substring(0, 2), 16));
                request.parameters.Add(Convert.ToInt32(reward.AdditionalMsg.Substring(2, 2), 16));
                request.parameters.Add(Convert.ToInt32(reward.AdditionalMsg.Substring(4, 2), 16));
            }
            request.parameters.Add(new Dictionary<string, int>() { { "duration", reward.Length } });

            SendMessageToGame(JsonConvert.SerializeObject(request));
        }
    }
}
