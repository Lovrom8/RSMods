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
        private ConcurrentQueue<Tuple<int, TwitchReward>> remainingRewards;
        private int retryInterval = 1; // In seconds

        private string ipAdr = "127.0.0.1";
        private int port = 45659;

        public EffectServerTCP()
        {
            StartServer();
        }

        private void StartServer()
        {
            tcpListener = new TcpListener(IPAddress.Parse(ipAdr), port);

            /*tcpListenerThread = new Thread(new ThreadStart(ListenForIncomingResponse));
            tcpListenerThread.IsBackground = true;
            tcpListenerThread.Start();*/

            cts = new CancellationTokenSource();
            HandleRemainingEffects(cts.Token);

            usedRewards = new Dictionary<int, TwitchReward>();
            remainingRewards = new ConcurrentQueue<Tuple<int, TwitchReward>>();
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
                    Tuple<int, TwitchReward> currentReward;
                    if (remainingRewards.TryDequeue(out currentReward))
                        SendEffectToTheGame(currentReward.Item2, false);

                    await Task.Delay(TimeSpan.FromSeconds(retryInterval), ct);
                }
            }, ct);
        }

        private async void SendMessageToGame(string message)
        {
            //if (connectedTcpClient == null)
            //    return;

            try
            {
                /* if (!connectedTcpClient.Client.Connected)
                 {
                     TwitchSettings.Get.AddToLog("Not connected :( Pleaase restart the game!");
                     return;
                 }*/

                while (true)
                {
                    tcpListener.Start();
                    WinMsgUtil.SendMsgToRS("Reconnect to CC");
                    await Task.Delay(500);

                    using (connectedTcpClient = tcpListener.AcceptTcpClient())
                    {
                        try
                        {
                            using (NetworkStream stream = connectedTcpClient.GetStream())
                            {
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

                                        if (response.status == 3) // && !remainingRewards.ContainsKey(response.id))// If retry code was returned, put it in the queue
                                            remainingRewards.Enqueue(new Tuple<int, TwitchReward>(response.id, usedRewards[response.id]));

                                        // We'd already dequeued the current effect, so no need to remove it if it goes through
                                        //if (response.status == 0 && remainingRewards.ContainsKey(response.id)) // If the effect has been executed sucessfully and it had been placed in the queue, remove it
                                        //    remainingRewards.TryRemove(response.id, null);

                                        tcpListener.Stop();
                                    }
                                }
                            }
                        }
                        catch (IOException ioex) // If the game (and the connection with it) was closed
                        {
                            TwitchSettings.Get.AddToLog($"IOEx: {ioex.Message}");
                            remainingRewards = new ConcurrentQueue<Tuple<int, TwitchReward>>(); // Clean up after ourselves
                        }
                    }
                }
            }
            catch (SocketException socketException)
            {
                TwitchSettings.Get.AddToLog($"SocketException: {socketException.Message}");
            }
        }

        public void AddEffectToTheQueue(TwitchReward reward)
        {
            remainingRewards.Enqueue(new Tuple<int, TwitchReward>(effectId, reward));
        }

        public void SendEffectToTheGame(TwitchReward reward, bool newEffect = true)
        {
            /* if (connectedTcpClient == null || !connectedTcpClient.Client.Connected)
             {
                 WinMsgUtil.SendMsgToRS("Reconnect to CC");
                 await Task.Delay(2000);
             }

             if (connectedTcpClient == null) // If we are still unable to connect, something is very wrong
             {
                 MessageBox.Show("Unable to connect to the effect server, please restart the game and RSMods!", "Error");
                 return;
             }*/

            if (newEffect)
            {
                Interlocked.Increment(ref effectId);
                usedRewards.Add(effectId, reward);
            }

            Request request = new Request()
            {
                id = effectId,
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

            SendMessageToGame(JsonConvert.SerializeObject(request));
        }
    }
}
