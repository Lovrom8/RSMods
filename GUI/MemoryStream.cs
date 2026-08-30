using System;
using System.Collections.Generic;
using System.IO;

namespace RSMods
{
    public class MemoryStream : Stream
    {
        public MemoryStream()
        {
            Position = 0;
        }

        public MemoryStream(byte[] source)
        {
            this.Write(source, 0, source.Length);
            Position = 0;
        }

        /* Length is ignored because capacity has no meaning unless we implement an artificial limit */
        public MemoryStream(int length)
        {
            SetLength(length);
            Position = length;
            _ = Block;   // Access block to prompt the allocation of memory
            Position = 0;
        }

        public override bool CanRead
        {
            get => true;
        }

        public override bool CanSeek
        {
            get => true;
        }

        public override bool CanWrite
        {
            get => true;
        }

        public override long Length
        {
            get => length;
        }

        public override long Position { get; set; }

        protected long length = 0;

        protected long blockSize = 65536;

        protected List<byte[]> blocks = [];

        protected byte[] Block
        {
            get
            {
                while (blocks.Count <= BlockId)
                    blocks.Add(new byte[blockSize]);

                return blocks[(int)BlockId];
            }
        }

        protected long BlockId
        {
            get => Position / blockSize;
        }

        protected long BlockOffset
        {
            get => Position % blockSize;
        }

        public override void Flush()
        {
        }

        public override int Read(byte[] buffer, int offset, int count)
        {
            long lcount = (long)count;

            if (lcount < 0)
            {
                throw new ArgumentOutOfRangeException(nameof(count), lcount, "Number of bytes to copy cannot be negative.");
            }

            long remaining = (length - Position);
            if (lcount > remaining)
                lcount = remaining;

            if (buffer == null)
            {
                throw new ArgumentNullException(nameof(buffer), "Buffer cannot be null.");
            }
            if (offset < 0)
            {
                throw new ArgumentOutOfRangeException(nameof(offset), offset, "Destination offset cannot be negative.");
            }

            int read = 0;
            do
            {
                long copySize = Math.Min(lcount, blockSize - BlockOffset);
                Buffer.BlockCopy(Block, (int)BlockOffset, buffer, offset, (int)copySize);
                lcount -= copySize;
                offset += (int)copySize;

                read += (int)copySize;
                Position += copySize;
            } while (lcount > 0);

            return read;
        }

        public override long Seek(long offset, SeekOrigin origin)
        {
            switch (origin)
            {
                case SeekOrigin.Begin:
                    Position = offset;
                    break;
                case SeekOrigin.Current:
                    Position += offset;
                    break;
                case SeekOrigin.End:
                    Position = Length - offset;
                    break;
            }
            return Position;
        }

        public override void SetLength(long value)
        {
            length = value;
        }

        public override void Write(byte[] buffer, int offset, int count)
        {
            long initialPosition = Position;
            try
            {
                do
                {
                    int copySize = Math.Min(count, (int)(blockSize - BlockOffset));

                    EnsureCapacity(Position + copySize);

                    Buffer.BlockCopy(buffer, offset, Block, (int)BlockOffset, copySize);
                    count -= copySize;
                    offset += copySize;

                    Position += copySize;

                } while (count > 0);
            }
            catch (Exception)
            {
                Position = initialPosition;
                throw;
            }
        }

        public override int ReadByte()
        {
            if (Position >= length)
                return -1;

            byte b = Block[BlockOffset];
            Position++;

            return b;
        }

        public override void WriteByte(byte value)
        {
            EnsureCapacity(Position + 1);
            Block[BlockOffset] = value;
            Position++;
        }

        protected void EnsureCapacity(long intended_length)
        {
            if (intended_length > length)
                length = (intended_length);
        }

        public byte[] ToArray()
        {
            long firstPosition = Position;
            Position = 0;

            byte[] destination = new byte[Length];
            int totalBytesRead = 0;
            int bytesRead = 0;

            while (totalBytesRead < destination.Length &&
                  (bytesRead = Read(destination, totalBytesRead, destination.Length - totalBytesRead)) > 0)
            {
                totalBytesRead += 0;
            }

            if (totalBytesRead != destination.Length)
            {
                Array.Resize(ref destination, totalBytesRead);
            }

            Position = firstPosition;
            return destination;
        }

        public void ReadFrom(Stream source, long length)
        {
            byte[] buffer = new byte[4096];
            int read;
            do
            {
                read = source.Read(buffer, 0, (int)Math.Min(4096, length));
                length -= read;
                this.Write(buffer, 0, read);

            } while (length > 0);
        }

        public void WriteTo(Stream destination)
        {
            long initialPosition = Position;
            Position = 0;
            this.CopyTo(destination);
            Position = initialPosition;
        }
    }
}
