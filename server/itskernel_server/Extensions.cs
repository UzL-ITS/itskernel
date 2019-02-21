using System;
using System.Collections.Generic;
using System.IO;
using System.Net.Sockets;
using System.Text;

namespace itskernel_server
{
    static class Extensions
    {
        /// <summary>
        /// Reads a line from the given stream reader.
        /// </summary>
        /// <param name="reader">The reader to read the line from.</param>
        /// <returns></returns>
        public static string ReadLine(this BinaryReader reader)
        {
            StringBuilder str = new StringBuilder();
            char curr;
            while((curr = (char)reader.ReadByte()) != '\n')
                str.Append(curr);
            return str.ToString();
        }
    }
}
