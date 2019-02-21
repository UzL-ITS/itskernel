using System;
using System.IO;

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
                Console.Read();
                return;
            }

            // Make sure the working directory contains the correct folders
            string inDirectory = Path.Combine(workingDirectory, @"in\");
            string outDirectory = Path.Combine(workingDirectory, @"out\");
            if(!Directory.Exists(inDirectory))
                Directory.CreateDirectory(inDirectory);
            if(!Directory.Exists(outDirectory))
                Directory.CreateDirectory(outDirectory);

            // Server parameters
            //string serverIp = RequestUserConfig("server IP", "192.168.20.1"); // VirtualBox
            string serverIp = RequestUserConfig("server IP", "192.168.21.1"); // VMware
            //string serverIp = RequestUserConfig("server IP", "141.83.62.232");

            // Start requested protocol server
            string protocolType = RequestUserConfig("protocol type [tcp, udp]", "tcp");
            if(protocolType == "tcp")
                new TwoWayProtocol(serverIp, SERVER_PORT, inDirectory, outDirectory).Run();
            else if(protocolType == "udp")
            {
                string remoteIp = RequestUserConfig("remote IP", "141.83.62.44");
                new OneWayProtocol(serverIp, SERVER_PORT, remoteIp, SERVER_PORT, inDirectory).Run();
            }
            else
                Console.WriteLine("Unknown protocol type.");
            Console.Read();
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

    }
}
