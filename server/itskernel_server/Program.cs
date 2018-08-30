﻿using System;
using System.Collections.Generic;
using System.IO;
using System.Net.Sockets;
using System.Text;

namespace itskernel_server
{
    class Program
    {
        /// <summary>
        /// The TCP port the server listens on.
        /// </summary>
        const int SERVER_PORT = 17571;

        /// <summary>
        /// Application entry point.
        /// </summary>
        /// <param name="args">Application arguments. Unused here.</param>
        static void Main(string[] args)
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
            string serverIp = RequestUserConfig("server IP", "127.0.0.1");
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
                    StreamReader connReader = new StreamReader(conn, Encoding.ASCII);
                    StreamWriter connWriter = new StreamWriter(conn, Encoding.ASCII)
                    {
                        NewLine = "\n"
                    };

                    // Client command loop
                    bool running = true;
                    while(running)
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
                                string[] inDirFileNames = Directory.GetFiles(inDirectory, "*.*", SearchOption.TopDirectoryOnly);

                                // Send file list
                                Console.WriteLine($"    Sending file list...");
                                connWriter.WriteLine(inDirFileNames.Length);
                                foreach(string fileName in inDirFileNames)
                                    connWriter.WriteLine(fileName);
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
                                byte[] file = File.ReadAllBytes(Path.Combine(inDirectory, fileName));

                                // Send file size first
                                Console.WriteLine($"    Sending file size...");
                                connWriter.WriteLine(file.Length);
                                connWriter.Flush();

                                // Send whole file
                                Console.WriteLine($"    Sending file data...");
                                connWriter.BaseStream.Write(file, 0, file.Length);

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
        static string RequestUserConfig(string name, string defaultValue)
        {
            // Ask for input
            Console.Write($"Enter {name} (default is \"{defaultValue}\"): ");
            string newValue = Console.ReadLine();
            if(string.IsNullOrWhiteSpace(newValue))
                return defaultValue;
            else
                return newValue;
        }
    }
}