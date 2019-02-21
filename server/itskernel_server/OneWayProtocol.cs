using System;
using System.IO;

namespace itskernel_server
{
    /// <summary>
    /// Implements a UDP-based one-way protocol, where the client only sends and the server only receives data.
    /// </summary>
    internal class OneWayProtocol
    {
        public string ServerIp { get; set; }
        public int ServerPort { get; set; }
        public string RemoteIp { get; set; }
        public int RemotePort { get; set; }
        public string OutDirectory { get; set; }

        public OneWayProtocol(string serverIp, int serverPort, string remoteIp, int remotePort, string outDirectory)
        {
            ServerIp = serverIp;
            ServerPort = serverPort;
            RemoteIp = remoteIp;
            RemotePort = remotePort;
            OutDirectory = outDirectory;
        }

        public void Run()
        {
            // Open UDP socket and wait for incoming data
            UdpServer server = new UdpServer(ServerIp, ServerPort, RemoteIp, RemotePort);
            while(true)
            {
                // Wait for incoming data
                Console.WriteLine("Waiting for data...");

                // Receive next command
                string command = server.ReceiveLine();
                Console.WriteLine($"Received command \"{command}\".");
                switch(command)
                {
                    case "sendout":
                    {
                        // Receive file name
                        Console.WriteLine($"    Receiving file name...");
                        string fileName = server.ReceiveLine();
                        Console.WriteLine($"    File name is \"{ fileName }\"");

                        // Receive file length
                        Console.WriteLine($"    Receiving file size...");
                        int fileLength = int.Parse(server.ReceiveLine());
                        Console.WriteLine($"    File size is \"{ fileLength }\"");

                        // Receive file
                        Console.WriteLine($"    Receiving file...");
                        byte[] file = server.ReceiveBytes(fileLength);

                        // Save file
                        Console.WriteLine($"    Saving file...");
                        File.WriteAllBytes(Path.Combine(OutDirectory, fileName), file);

                        break;
                    }

                    default:
                    {
                        // Ignore
                        Console.WriteLine($"    Invalid command.");
                        break;
                    }
                }
                Console.WriteLine($"    Done.");
            }
        }
    }
}
