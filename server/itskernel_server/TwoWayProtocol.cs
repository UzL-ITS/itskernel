using System;
using System.Collections.Generic;
using System.IO;
using System.Net.Sockets;
using System.Text;

namespace itskernel_server
{
    /// <summary>
    /// Implements a TCP-based two-way protocol, where both client and server can send and receive data.
    /// </summary>
    class TwoWayProtocol
    {
        public string ServerIp { get; set; }
        public int ServerPort { get; set; }
        public string InDirectory { get; set; }
        public string OutDirectory { get; set; }

        public TwoWayProtocol(string serverIp, int serverPort, string inDirectory, string outDirectory)
        {
            ServerIp = serverIp;
            ServerPort = serverPort;
            InDirectory = inDirectory;
            OutDirectory = outDirectory;
        }

        public void Run()
        {
            TcpListener server = new TcpListener(System.Net.IPAddress.Parse(ServerIp), ServerPort);
            server.Start();
            while(true)
            {
                // Wait for client
                Console.WriteLine("Waiting for client...");
                using(TcpClient connClient = server.AcceptTcpClient())
                {
                    // Get client network stream
                    Console.WriteLine($"Client {connClient.Client.RemoteEndPoint.ToString()} connected");
                    NetworkStream conn = connClient.GetStream();
                    BinaryReader connReader = new BinaryReader(conn);
                    StreamWriter connWriter = new StreamWriter(conn, Encoding.ASCII)
                    {
                        NewLine = "\n"
                    };

                    // Client command loop
                    bool running = true;
                    while(running && connClient.Connected)
                    {
                        // Receive next command
                        string command = connReader.ReadLine();
                        Console.WriteLine($"Received command \"{command}\".");
                        switch(command)
                        {
                            case "exit":
                            {
                                // Disconnect
                                running = false;

                                break;
                            }

                            case "ls":
                            {
                                // Get files in input directory
                                Console.WriteLine($"    Enumerating files...");
                                string[] inDirFileNames = Directory.GetFiles(InDirectory, "*.*", SearchOption.TopDirectoryOnly);

                                // Send file list
                                Console.WriteLine($"    Sending file list...");
                                connWriter.WriteLine(inDirFileNames.Length);
                                foreach(string fileName in inDirFileNames)
                                    connWriter.WriteLine(Path.GetFileName(fileName));
                                connWriter.Flush();

                                break;
                            }

                            case "sendin":
                            {
                                // Receive file name
                                Console.WriteLine($"    Receiving file name...");
                                string fileName = connReader.ReadLine();

                                // Read entire file into memory
                                Console.WriteLine($"    Reading file \"{ fileName }\"...");
                                byte[] file;
                                if(File.Exists(Path.Combine(InDirectory, fileName)))
                                    file = File.ReadAllBytes(Path.Combine(InDirectory, fileName));
                                else
                                    file = Encoding.ASCII.GetBytes("File does not exist.");

                                // Send file size first
                                Console.WriteLine($"    Sending file size...");
                                connWriter.WriteLine(file.Length);
                                connWriter.Flush();

                                // Send whole file
                                Console.WriteLine($"    Sending file data...");
                                connWriter.BaseStream.Write(file, 0, file.Length);

                                break;
                            }

                            case "sendout":
                            {
                                // Receive file name
                                Console.WriteLine($"    Receiving file name...");
                                string fileName = connReader.ReadLine();
                                Console.WriteLine($"    File name is \"{ fileName }\"");

                                // Receive file length
                                Console.WriteLine($"    Receiving file size...");
                                int fileLength = int.Parse(connReader.ReadLine());
                                Console.WriteLine($"    File size is \"{ fileLength }\"");

                                // Receive file
                                Console.WriteLine($"    Receiving file...");
                                byte[] file = new byte[fileLength];
                                int receivedBytes = 0;
                                DateTime startTime = DateTime.Now;
                                while(receivedBytes < fileLength)
                                {
                                    receivedBytes += connReader.Read(file, receivedBytes, fileLength - receivedBytes);
                                    Console.Write($"\r        {receivedBytes}/{fileLength} ({(100 * receivedBytes) / fileLength} %, {Math.Round(((double)receivedBytes / (DateTime.Now - startTime).Seconds) / 1024)} KB/s on average)");
                                }
                                Console.WriteLine();

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
    }
}
