using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Text;

namespace itskernel_server
{
    /// <summary>
    /// Wraps a <see cref="UdpClient"/> instance to support advanced receive functions.
    /// </summary>
    internal class UdpServer : UdpClient
    {
        private int _receivedBytesIndex = 0;
        private readonly List<byte> _receivedBytes = new List<byte>();
        private IPEndPoint _remoteEndPoint;

        public UdpServer(string localAddress, int localPort, string remoteAddress, int remotePort)
            : base(new IPEndPoint(IPAddress.Parse(localAddress), localPort))
        {
            _remoteEndPoint = new IPEndPoint(IPAddress.Parse(remoteAddress), remotePort);
        }

        private void Receive()
        {
            // Receive incoming data
            byte[] data = Receive(ref _remoteEndPoint);
            _receivedBytes.AddRange(data);
        }

        public string ReceiveLine()
        {
            // Receive characters until new line is reached
            StringBuilder str = new StringBuilder();
            char curr;
            while(true)
            {
                while(_receivedBytesIndex >= _receivedBytes.Count)
                    Receive();
                curr = (char)_receivedBytes[_receivedBytesIndex++];
                if(curr == '\n')
                    break;
                str.Append(curr);
            }
            return str.ToString();
        }

        public byte[] ReceiveBytes(int length)
        {
            // Wait until there is enough data in receive queue
            while(_receivedBytesIndex + length > _receivedBytes.Count)
                Receive();
            byte[] bytes = new byte[length];
            _receivedBytes.CopyTo(_receivedBytesIndex, bytes, 0, length);
            _receivedBytesIndex += length;
            return bytes;
        }
    }
}
