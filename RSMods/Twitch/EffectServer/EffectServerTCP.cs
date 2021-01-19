using Newtonsoft.Json;
using RSMods.Util;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace RSMods.Twitch.EffectServer
{
    class EffectServerTCP
    {
        private TcpListener tcpListener;
        private Thread tcpListenerThread;
        private CancellationTokenSource cts;
        private TcpClient connectedTcpClient;
        private TimeSpan retrySpan;
        private NetworkStream stream;

        private Dictionary<int, TwitchReward> usedRewards;
        private ConcurrentQueue<TwitchReward> remainingRewards;
        private int retryInterval = 1; // In seconds
        private bool reconnect = false;

        private string ipAdr = "127.0.0.1";
        private int port = 45659;

        public EffectServerTCP()
        {
            StartServer();
        }

        private void StartConnectionThread()
        {
            tcpListenerThread = new Thread(new ThreadStart(GetConnectionToTheGame));
            tcpListenerThread.IsBackground = true;
            tcpListenerThread.Start();
        }

        private void StartServer()
        {
            StartConnectionThread();

            cts = new CancellationTokenSource();
            retrySpan = TimeSpan.FromSeconds(retryInterval);
            HandleRemainingEffects(cts.Token);

            usedRewards = new Dictionary<int, TwitchReward>();
            remainingRewards = new ConcurrentQueue<TwitchReward>();
            // remainingRewards = new ConcurrentQueue<Tuple<int, TwitchReward>>();
            //remainingRewards = new ConcurrentDictionary<int, TwitchReward>();

            TwitchSettings.Get.AddToLog("Started the effect server");
        }

        /*private void ListenForIncomingResponse()
        {
        }*/

        private void HandleRemainingEffects(CancellationToken ct)
        {
            Task.Run(async () =>
            {
                while (!ct.IsCancellationRequested)
                {
                    TwitchReward currentReward;
                    if (remainingRewards.TryDequeue(out currentReward))
                        SendEffectToTheGame(currentReward, true);

                    //TwitchSettings.Get.AddToLog("Trying...");

                    await Task.Delay(retrySpan, ct);
                }
            }, ct);
        }

        public void StopTCPListener()
        {
            try
            {
                if (tcpListener != null)
                    tcpListener.Stop();
            }
            catch (SocketException) //Likely the error due to it being stopped from another thread
            {
            }
        }

        private void GetConnectionToTheGame()
        {
            try
            {
                if (tcpListener != null && tcpListener.Server.IsBound) //No need to reconnect if we have already connected to the socket
                    return;

                tcpListener = new TcpListener(IPAddress.Parse(ipAdr), port);

                reconnect = false;

                tcpListener.Start();
                connectedTcpClient = tcpListener.AcceptTcpClient();
                TwitchSettings.Get.AddToLog("Connected to the game...");
            }
            catch (SocketException socketException)
            {
                if (!socketException.Message.Contains("WSACancelBlockingCall"))
                    TwitchSettings.Get.AddToLog($"SocketException: {socketException.Message}");
            }
        }

        private async void SendMessageToGame(string message, TwitchReward currentReward)
        {
            if (connectedTcpClient == null)
            {
                WinMsgUtil.SendMsgToRS("Reconnect to CC");
                await Task.Delay(500);
            }

            if (connectedTcpClient == null || !connectedTcpClient.Client.Connected)
            {
                TwitchSettings.Get.AddToLog("Unable to connect to the effect server, please restart RSMods!");
                return;
            }

            await Task.Run(() =>
             {
                 try
                 {
                     if (!connectedTcpClient.Connected)
                         return;

                     stream = connectedTcpClient.GetStream();

                     var serverMessageAsByteArray = new List<byte>();
                     serverMessageAsByteArray.AddRange(Encoding.ASCII.GetBytes(message));
                     serverMessageAsByteArray.Add((byte)0);

                     stream.Write(serverMessageAsByteArray.ToArray(), 0, serverMessageAsByteArray.Count());

                     Debug.Write($"Sent a message to the game: {message}");
                     Debug.Write("Server is listening for the response");

                     Byte[] bytes = new Byte[1024];

                     int length;

                     while ((length = stream.Read(bytes, 0, bytes.Length)) != 0)
                     {
                         var incomingData = new byte[length];
                         Array.Copy(bytes, 0, incomingData, 0, length);

                         string clientMessage = Encoding.ASCII.GetString(incomingData);
                         Debug.Write($"Recieved a message from the game: {clientMessage}");

                         if (clientMessage != "\0")
                         {
                             var response = JsonConvert.DeserializeObject<Response>(clientMessage);

                             if (response.status == 3) // If retry code was returned, put it back in the queue
                             {
                                 remainingRewards.Enqueue(currentReward);
                                 // TwitchSettings.Get.AddToLog($"Requeing {currentReward.Name}");
                             }

                             // We'd already dequeued the current effect, so no need to remove it if it goes through
                             //if (response.status == 0 && remainingRewards.ContainsKey(response.id)) // If the effect has been executed sucessfully and it had been placed in the queue, remove it
                             //    remainingRewards.TryRemove(response.id, null);
                         }
                     }
                 }
                 catch (IOException ioex) // If the game (and the connection with it) was closed
                 {
                     if (ioex.Message.Contains("forcibly"))
                         TwitchSettings.Get.AddToLog($"Looks like the game was closed :(");
                     else
                         TwitchSettings.Get.AddToLog($"IO Exception: {ioex.Message}");

                     remainingRewards = new ConcurrentQueue<TwitchReward>(); // Clean up after ourselves

                     if (!reconnect)
                     {
                         tcpListener.Stop();
                         StartConnectionThread();
                         reconnect = true;
                     }
                 }
                 catch (SocketException socketException)
                 {
                     TwitchSettings.Get.AddToLog($"SocketException: {socketException.Message}");
                 }
             });
        }

        public void AddEffectToTheQueue(TwitchReward reward) => remainingRewards.Enqueue(reward);

        public void SendEffectToTheGame(TwitchReward reward, bool newEffect = true)
        {
            Request request = new Request()
            {
                id = 1,
                code = reward.InternalMsgEnable.ToLower().Replace("enable", "").Trim(),
                type = 1,
                viewer = "rsmods"
            };
            request.parameters = new List<object>();

            if (reward.AdditionalMsg != null && reward.InternalMsgEnable.ToLower().Contains("solidnote")) // If it's a solid color effect
            {
                if (reward.AdditionalMsg == "Random")
                    request.code = "solidrandom";
                else if (reward.AdditionalMsg.Length == 6)
                {
                    request.parameters.Add(Convert.ToInt32(reward.AdditionalMsg.Substring(0, 2), 16));
                    request.parameters.Add(Convert.ToInt32(reward.AdditionalMsg.Substring(2, 2), 16));
                    request.parameters.Add(Convert.ToInt32(reward.AdditionalMsg.Substring(4, 2), 16));
                    request.code = "solidcustomrgb";
                }
            }
            request.parameters.Add(new Dictionary<string, int>() { { "duration", reward.Length } });

            SendMessageToGame(JsonConvert.SerializeObject(request), reward);
        }
    }
}

