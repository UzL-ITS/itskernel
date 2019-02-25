using System;
using System.Collections.Generic;
using System.Linq;
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
        private readonly Dictionary<uint, Block> _receivedBlocks = new Dictionary<uint, Block>();
        private uint _nextRetrievableBlockId = 0;

        private readonly TimeSpan _receiveMinDelay = TimeSpan.FromSeconds(3);
        private readonly TimeSpan _receiveMaxDelay = TimeSpan.FromSeconds(60);
        private IPEndPoint _remoteEndPoint;

        /// <summary>
        /// Creates a new UDP server instance at the given local and remote ports.
        /// </summary>
        public UdpServer(string localAddress, int localPort, string remoteAddress, int remotePort)
            : base(new IPEndPoint(IPAddress.Parse(localAddress), localPort))
        {
            _remoteEndPoint = new IPEndPoint(IPAddress.Parse(remoteAddress), remotePort);
        }

        /// <summary>
        /// Receives and parses one chunk.
        /// Assumes that an UDP datagram contains exactly one chunk.
        /// </summary>
        private void ReceiveChunk()
        {
            // Receive incoming data
            byte[] data = Receive(ref _remoteEndPoint);

            // Extract header
            uint blockId = BitConverter.ToUInt32(data, 0);
            uint chunkCount = BitConverter.ToUInt32(data, 4);
            uint chunkId = BitConverter.ToUInt32(data, 8);
            ushort chunkLength = BitConverter.ToUInt16(data, 12);

            // Retrieve block object
            if(!_receivedBlocks.ContainsKey(blockId))
                _receivedBlocks.Add(blockId, new Block
                {
                    ChunkCount = chunkCount,
                    Chunks = new Dictionary<uint, Chunk>()
                });
            var block = _receivedBlocks[blockId];
            if(block.ChunkCount != chunkCount)
                throw new Exception("Chunk count data of received chunk and already known block differ.");

            // Chunk already received?
            if(block.Chunks.ContainsKey(chunkId))
            {
                Console.WriteLine($"Received chunk #{chunkId} another time, skipping...");
                return;
            }

            // Store chunk
            byte[] payload = new byte[chunkLength];
            Array.Copy(data, 14, payload, 0, chunkLength);
            var chunk = new Chunk
            {
                ChunkLength = chunkLength,
                Data = payload
            };
            block.Chunks.Add(chunkId, chunk);
        }

        public void SkipBlocks(uint count)
        {
            _nextRetrievableBlockId += count;
        }

        /// <summary>
        /// Waits until the next block is received, or throws an exception if that wasn't possible within the given time frame.
        /// </summary>
        private Block ReceiveBlock()
        {
            // Give some time for receiving something
            DateTime receiveStartTime = DateTime.Now;
            Console.WriteLine("Receive loop start");
            while(true)
            {
                if(Available > 0)
                    ReceiveChunk();
                else if(DateTime.Now > receiveStartTime + _receiveMinDelay)
                    break;
                if(DateTime.Now > receiveStartTime + _receiveMaxDelay)
                    break;
            }
            Console.WriteLine("Receive loop end");

            // Next block available?
            if(!_receivedBlocks.ContainsKey(_nextRetrievableBlockId))
                throw new PartialBlockException("There is no next block available.");

            // Get next block
            var block = _receivedBlocks[_nextRetrievableBlockId];
            if(block.Chunks.Count != block.ChunkCount)
                throw new PartialBlockException("The next block has only been received partially.");
            return block;
        }

        public string ReceiveLine()
        {
            // Give some time for receiving everything
            var block = ReceiveBlock();

            // Convert block into char array
            StringBuilder str = new StringBuilder();
            char curr;
            for(uint i = 0; i < block.ChunkCount; ++i)
            {
                var chunk = block.Chunks[i];
                for(int index = 0; index < chunk.ChunkLength; ++index)
                {
                    curr = (char)chunk.Data[index];
                    if(curr != '\n')
                        str.Append(curr);
                }
            }

            // Receiving done
            ++_nextRetrievableBlockId;
            return str.ToString();
        }

        public byte[] ReceiveBytes(int length)
        {
            // Give some time for receiving everything
            var block = ReceiveBlock();

            // Valid block?
            if(length != block.Chunks.Sum(c => c.Value.ChunkLength))
                throw new Exception("The given block size and the chunk lengths differ.");

            // Copy block contents
            byte[] data = new byte[length];
            int offset = 0;
            for(uint i = 0; i < block.ChunkCount; ++i)
            {
                var chunk = block.Chunks[i];
                Array.Copy(chunk.Data, 0, data, offset, chunk.ChunkLength);
                offset += chunk.ChunkLength;
            }

            // Receiving done
            ++_nextRetrievableBlockId;
            return data;
        }

        /// <summary>
        /// Defines one UDP transmission block.
        /// </summary>
        private class Block
        {
            public uint ChunkCount { get; set; }
            public Dictionary<uint, Chunk> Chunks { get; set; }
        }

        /// <summary>
        /// Defines one UDP transmission chunk.
        /// </summary>
        private class Chunk
        {
            public ushort ChunkLength { get; set; }
            public byte[] Data { get; set; }
        }

        /// <summary>
        /// Thrown when a block is not or only partially received.
        /// </summary>
        public class PartialBlockException : Exception
        {
            public PartialBlockException(string message)
                : base(message)
            {
                Console.WriteLine("--- PartialBlockException: " + message);
            }
        }
    }
}
