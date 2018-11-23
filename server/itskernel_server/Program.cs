﻿using System;
using System.IO;
using System.Net.Sockets;
using System.Text;

namespace itskernel_server
{
    internal class Program
    {
        /// <summary>
        /// The TCP port the server listens on.
        /// </summary>
        private const int SERVER_PORT = 17571;

        /// <summary>
        /// Application entry point.
        /// </summary>
        /// <param name="args">Application arguments. Unused here.</param>
        private static void Main(string[] args)
        {
            // Get working directory
            string workingDirectory = RequestUserConfig("working directory", @"R:\itskernel\");
            if(!Directory.Exists(workingDirectory))
            {
                Console.WriteLine("The given working directory does not exist.");
                return;
            }

            // Make sure the working directory contains the correct folders
            string inDirectory = Path.Combine(workingDirectory, @"in\");
            string outDirectory = Path.Combine(workingDirectory, @"out\");
            if(!Directory.Exists(inDirectory))
                Directory.CreateDirectory(inDirectory);
            if(!Directory.Exists(outDirectory))
                Directory.CreateDirectory(outDirectory);

            // Start server
            //string serverIp = RequestUserConfig("server IP", "192.168.20.1"); // VirtualBox
            //string serverIp = RequestUserConfig("server IP", "192.168.21.1"); // VMware
            string serverIp = RequestUserConfig("server IP", "141.83.62.232");
            TcpListener server = new TcpListener(System.Net.IPAddress.Parse(serverIp), SERVER_PORT);
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
                        string command = ReadLine(connReader);
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
                                string[] inDirFileNames = Directory.GetFiles(inDirectory, "*.*", SearchOption.TopDirectoryOnly);

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
                                string fileName = ReadLine(connReader);

                                // Read entire file into memory
                                Console.WriteLine($"    Reading file \"{ fileName }\"...");
                                byte[] file;
                                if(File.Exists(Path.Combine(inDirectory, fileName)))
                                    file = File.ReadAllBytes(Path.Combine(inDirectory, fileName));
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
                                string fileName = ReadLine(connReader);
                                Console.WriteLine($"    File name is \"{ fileName }\"");

                                // Receive file length
                                Console.WriteLine($"    Receiving file size...");
                                int fileLength = int.Parse(ReadLine(connReader));
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
                                File.WriteAllBytes(Path.Combine(outDirectory, fileName), file);

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

        /// <summary>
        /// Requests the user to type the value for a given configuration item.
        /// </summary>
        /// <param name="name">The display name of the requested configuration item.</param>
        /// <param name="defaultValue">The default value that is returned when the user simply presses ENTER.</param>
        /// <returns></returns>
        private static string RequestUserConfig(string name, string defaultValue)
        {
            // Ask for input
            Console.Write($"Enter {name} (default is \"{defaultValue}\"): ");
            string newValue = Console.ReadLine();
            if(string.IsNullOrWhiteSpace(newValue))
                return defaultValue;
            else
                return newValue;
        }

        /// <summary>
        /// Reads a line from the given stream reader.
        /// </summary>
        /// <param name="reader">The reader to read the line from.</param>
        /// <returns></returns>
        private static string ReadLine(BinaryReader reader)
        {
            StringBuilder str = new StringBuilder();
            char curr;
            while((curr = (char)reader.ReadByte()) != '\n')
                str.Append(curr);
            return str.ToString();
        }

    }
}
