// ======================================================================= 
//  TEXAS INSTRUMENTS, INC.                                                
// ----------------------------------------------------------------------- 
//                                                                         
// sfh_AIS_Util.cs -- Implements AIS_Parser class (C#)
// AUTHOR      : Joseph Coombs, Stanley Park                                                                     
// Rev 0.0.1                                                               
//                                                                         
//  USAGE                                                                  
//      Include AIS_Util namespace in your project and use the following
//      constructor:
//                                                                         
//          AIS_Parser parser = new AIS_Parser
//                                  (
//                                      AIS_Parser.AIS_<bootPeripheral>,
//                                      FxnDelegate_msg_log,
//                                      FxnDelegate_<bootPeripheral>_read,
//                                      FxnDelegate_<bootPeripheral>_write
//                                  );
//
//      Call parsing function using the contents of a binary AIS file
//      stored in a byte array:
//
//          parser.boot(ais_file_contents);
//                                                                         
//  DESCRIPTION                                                            
//      Parses a binary AIS file passed in as a byte array.  Uses external
//      functions (passed as delegates) for I/O read and write and message
//      logging.  Performs all host operations as described in "Using the
//      D800K001 Bootloader" application note for I2C slave, SPI slave, and
//      UART boot modes.                                                                   
//                                                                         
// ----------------------------------------------------------------------- 
//            Copyright (c) 2008 Texas Instruments, Incorporated.          
//                           All Rights Reserved.                          
// ======================================================================= 

using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;

using System.IO;
using System.IO.Ports;
using System.Reflection;
using System.Globalization;
using UtilLib;
using UtilLib.CRC;
using UtilLib.IO;
using UtilLib.ConsoleUtility;

namespace AIS_Util
{
    // host function delegates (rx, tx, log)
    //delegate int ReadFxnDelegate(byte[] rcvBuf, int index, int rcvSize, int timeout);
    //delegate int WriteFxnDelegate(byte[] xmtBuf, int index, int xmtSize, int timeout);
    //delegate void LogFxnDelegate(String line);

    class AIS_Parser
    {
        // AIS constants
	    public const int AIS_error =      -1;
	    public const int AIS_inProgress =  0;
	    public const int AIS_complete =    1;

	    public const int AIS_I2C =  0;
        public const int AIS_SPI =  1;
	    public const int AIS_UART = 2;

        private const uint AIS_magicWord =       0x41504954;
        private const uint AIS_XMT_START_WORD =  0x58535441;
        private const uint AIS_RCV_START_WORD =  0x52535454;

        private const uint AIS_OP_bootTable =    0x58535907;
        private const uint AIS_OP_seqReadEn =    0x58535963;
        private const uint AIS_OP_sectionLoad =  0x58535901;
        private const uint AIS_OP_cSectionLoad = 0x58535909;
        private const uint AIS_OP_sectionFill =  0x5853590A;
        private const uint AIS_OP_fxnExec =      0x5853590D;
        private const uint AIS_OP_jump =         0x58535905;
        private const uint AIS_OP_jumpClose =    0x58535906;
        private const uint AIS_OP_crcEn =        0x58535903;
        private const uint AIS_OP_crcDis =       0x58535904;
        private const uint AIS_OP_crcReq =       0x58535902;
        private const uint AIS_OP_readWait =     0x58535914;
        private const uint AIS_OP_startOver =    0x58535908;
        private const uint AIS_OP_pingDevice =   0x5853590B;

        // public data members
        public int posN, ioBits, ioDelay, opcodeDelay, ioTimeout;
        public bool waitBOOTME;

        public SerialPort myUART;

        // private members

        //private int hostDevice;
        //private ReadFxnDelegate readFxn;
        //private WriteFxnDelegate writeFxn;
        //private LogFxnDelegate logFxn;

        public AIS_Parser(SerialPort UART)
        {
            // apply specified params
            //hostDevice = hostType;
            //readFxn = hostReadFxn;
            //writeFxn = hostWriteFxn;
            //logFxn = hostLogFxn;

            // use defaults for others
            myUART = UART;
            posN = 2;
            ioBits = 8;
            ioDelay = 0;
            opcodeDelay = 5;   // 5ms
            ioTimeout = 5000;  // 5s
            waitBOOTME = true;
        }

        // utility functions (were C macros)
        private uint AIS_XMT_START(int bits)
        {
            // return (bits) MSBs or AIS start word
            return AIS_XMT_START_WORD >> (32 - bits);
        }

        private uint AIS_RCV_START(int bits)
        {
            // return (bits) MSBs or AIS start word ACK
            return AIS_RCV_START_WORD >> (32 - bits);
        }

        private uint AIS_opcode2ack(uint opcode)
        {
            // return opcode ACK
            return (opcode & 0xF0FFFFFF) | 0x02000000;
        }

        private int LOCAL_roundUpTo(int num, int mod)
        {
            // round (num) up to nearest multiple of (mod)
            return ( (num + (mod - 1)) / mod * mod);
        }

        private uint LOCAL_b2uint(byte[] ba)
        {
            // convert byte array to uint with little endian order
            return (uint)ba[0] + ((uint)ba[1] << 8) + ((uint)ba[2] << 16) + ((uint)ba[3] << 24);
        }

        private byte[] LOCAL_uint2b(uint ui)
        {
            // convert uint to byte array with little endian order
            byte[] ba = new byte[4];
            ba[0] = (byte)(ui & 0xFF);
            ba[1] = (byte)((ui >> 8) & 0xFF);
            ba[2] = (byte)((ui >> 16) & 0xFF);
            ba[3] = (byte)((ui >> 24) & 0xFF);
            return ba;
        }

        private void LOCAL_delay(int N)
        {
            Thread.Sleep(N);
        }

        // major function
        public int boot(byte[] AIS_Contents)
        {
	        int AIS_Cursor = 0;
            uint command, addr, size, type,
                sleep, data, crc, crcGuess,
                seek, mask, val, args;
            byte[] crcGuessB = new byte[4];
            int i, status;
	        int opsRead = 0;
	        
            // check for magic word first
            command = LOCAL_parseInt(AIS_Contents, ref AIS_Cursor);
            status = (command == AIS_magicWord) ? AIS_inProgress : AIS_error;
            //logFxn(String.Format("(AIS Parse): Read magic word 0x{0:X8}.", command));
            Console.WriteLine("(AIS Parse): Read magic word 0x{0:X8}.", command);

        	// UART only: read "BOOTME "
	        //if (status == AIS_inProgress && hostDevice == AIS_UART && waitBOOTME)
                if (status == AIS_inProgress  && waitBOOTME)
	        {
		        byte[] rcvInit = new byte[8];
                byte[] corInit = { (byte)'B', (byte)'O', (byte)'O', (byte)'T',
                                   (byte)'M', (byte)'E', (byte)' ', (byte)0 };

                // use longer than normal timeout
                //logFxn("(AIS Parse): Waiting for BOOTME...");
                Console.WriteLine("(AIS Parse): Waiting for BOOTME...");
                status = UTIL_uartRead(rcvInit, 0, 8, ioTimeout * 10);

		        // fail on incorrect sequence or IO error
		        for (i = 0; i < 7; i++)
                    if (rcvInit[i] != corInit[i])
                    {
                        Console.WriteLine("(AIS Parse): Read invalid BOOTME string.");
                        status = AIS_error;
                        break;
                    }
	        }

	        while (status == AIS_inProgress)
	        {
		        // perform synchronization on first pass
		        if (opsRead == 0)
		        {
			        // perform SWS
			        Console.WriteLine("(AIS Parse): Performing Start-Word Sync...");
			        status = AIS_SWS();
			        if (status == AIS_error)
				        break;		// fail if SWS fails

			        // perform POS
			        Console.WriteLine("(AIS Parse): Performing Ping Opcode Sync...");
			        status = AIS_POS(AIS_OP_pingDevice);
			        if (status == AIS_error)
				        continue;	// retry SWS if POS fails
		        }

		        // delay; give bootloader a chance to process previous command
		        LOCAL_delay(opcodeDelay);

		        // read a command
		        command = LOCAL_parseInt(AIS_Contents, ref AIS_Cursor);
		        Console.WriteLine( "(AIS Parse): Processing command {0}: 0x{1:X8}.", opsRead++, command );

		        // perform OS
		        Console.WriteLine("(AIS Parse): Performing Opcode Sync...");
		        status = AIS_OS(command);

		        if (status == AIS_error)
			        break;		// fail if OS fails

		        switch(command)
		        {
			        case AIS_OP_bootTable:
				        Console.WriteLine("(AIS Parse): Loading boot table...");
				        // read: type, addr, data, sleep
				        type = LOCAL_parseInt(AIS_Contents, ref AIS_Cursor);
				        addr = LOCAL_parseInt(AIS_Contents, ref AIS_Cursor);
				        data = LOCAL_parseInt(AIS_Contents, ref AIS_Cursor);
				        sleep = LOCAL_parseInt(AIS_Contents, ref AIS_Cursor);
				        // send: type, addr, data, sleep
				        status |= LOCAL_bufWrite(LOCAL_uint2b(type), 0, 4, ioTimeout);
				        status |= LOCAL_bufWrite(LOCAL_uint2b(addr), 0, 4, ioTimeout);
				        status |= LOCAL_bufWrite(LOCAL_uint2b(data), 0, 4, ioTimeout);
				        status |= LOCAL_bufWrite(LOCAL_uint2b(sleep), 0, 4, ioTimeout);
				        break;

			        case AIS_OP_seqReadEn:
				        // no extra IO required
				        Console.WriteLine("(AIS Parse): No slave memory present; Sequential Read Enable has no effect.");
				        break;

			        case AIS_OP_sectionLoad:
                    case AIS_OP_cSectionLoad:
                        Console.WriteLine("(AIS Parse): Loading section...");
				        // send address
				        addr = LOCAL_parseInt(AIS_Contents, ref AIS_Cursor);
				        status |= LOCAL_bufWrite(LOCAL_uint2b(addr), 0, 4, ioTimeout);
				        // send size
				        size = LOCAL_parseInt(AIS_Contents, ref AIS_Cursor);
				        status |= LOCAL_bufWrite(LOCAL_uint2b(size), 0, 4, ioTimeout);
				        // send data
				        status |= LOCAL_bufWrite(AIS_Contents, AIS_Cursor, LOCAL_roundUpTo((int)size, 4), ioTimeout);
                        LOCAL_parseSkip(ref AIS_Cursor, LOCAL_roundUpTo((int)size, 4));
				        Console.WriteLine( String.Format("(AIS Parse): Loaded {0}-byte section to address 0x{1:X8}.", size, addr) );
                        break;

			        case AIS_OP_sectionFill:
				        Console.WriteLine("(AIS Parse): Filling section...");
				        // send address
				        addr = LOCAL_parseInt(AIS_Contents, ref AIS_Cursor);
				        status |= LOCAL_bufWrite(LOCAL_uint2b(addr), 0, 4, ioTimeout);
				        // send size
				        size = LOCAL_parseInt(AIS_Contents, ref AIS_Cursor);
				        status |= LOCAL_bufWrite(LOCAL_uint2b(size), 0, 4, ioTimeout);
				        // send type
				        type = LOCAL_parseInt(AIS_Contents, ref AIS_Cursor);
				        status |= LOCAL_bufWrite(LOCAL_uint2b(type), 0, 4, ioTimeout);
				        // send pattern (data)
				        data = LOCAL_parseInt(AIS_Contents, ref AIS_Cursor);
				        status |= LOCAL_bufWrite(LOCAL_uint2b(data), 0, 4, ioTimeout);
				        Console.WriteLine( "(AIS Parse): Filled {0}-byte section with pattern 0x{1:X8}.", size, data);
				        break;

			        case AIS_OP_crcDis:
				        // no extra IO required
				        Console.WriteLine("(AIS Parse): CRC disabled.");
				        break;

			        case AIS_OP_crcEn:
				        // no extra IO required
				        Console.WriteLine("(AIS Parse): CRC enabled.");
				        break;

			        case AIS_OP_crcReq:
				        Console.WriteLine("(AIS Parse): Requesting CRC...");
				        // read computed CRC
				        status |= LOCAL_bufRead(crcGuessB, 0, 4, ioTimeout);
                        crcGuess = LOCAL_b2uint(crcGuessB);
				        crc = LOCAL_parseInt(AIS_Contents, ref AIS_Cursor);
				        if (crcGuess == crc)
				        {
					        // CRC succeeded.  Skip seek value to reach next opcode
					        Console.WriteLine("(AIS Parse): CRC passed!");
					        LOCAL_parseSkip(ref AIS_Cursor, 4);
				        }
				        else
				        {
					        // CRC error; send startover opcode and seek AIS
					        Console.WriteLine("(AIS Parse): CRC failed!  Sending STARTOVER...");
					        status |= AIS_OS(AIS_OP_startOver);
					        // seek AIS
					        seek = LOCAL_parseInt(AIS_Contents, ref AIS_Cursor);
					        Console.WriteLine( "(AIS Parse): {0}-byte seek applied.", seek) ;
                            LOCAL_parseSkip(ref AIS_Cursor, (int)seek);
				        }
				        break;
        				
			        case AIS_OP_readWait:
                       Console.WriteLine("(AIS Parse): Performing read-wait...");
				        // send address
				        addr = LOCAL_parseInt(AIS_Contents, ref AIS_Cursor);
				        status |= LOCAL_bufWrite(LOCAL_uint2b(addr), 0, 4, ioTimeout);
				        // send mask
				        mask = LOCAL_parseInt(AIS_Contents, ref AIS_Cursor);
				        status |= LOCAL_bufWrite(LOCAL_uint2b(mask), 0, 4, ioTimeout);
				        // send value
				        val = LOCAL_parseInt(AIS_Contents, ref AIS_Cursor);
				        status |= LOCAL_bufWrite(LOCAL_uint2b(val), 0, 4, ioTimeout);
				        break;

			        case AIS_OP_fxnExec:
				        Console.WriteLine("(AIS Parse): Executing function...");
				        // send function number and number of arguments
				        args = LOCAL_parseInt(AIS_Contents, ref AIS_Cursor);
				        status |= LOCAL_bufWrite(LOCAL_uint2b(args), 0, 4, ioTimeout);
				        args = (args & 0xFFFF0000) >> 16;
				        for (i = 0; i < args; i++)
				        {
					        // send arg i
					        val = LOCAL_parseInt(AIS_Contents, ref AIS_Cursor);
					        status |= LOCAL_bufWrite(LOCAL_uint2b(val), 0, 4, ioTimeout);
				        }
				        break;

			        case AIS_OP_jump:
				        Console.WriteLine("(AIS Parse): Performing jump...");
				        // send address
				        addr = LOCAL_parseInt(AIS_Contents, ref AIS_Cursor);
				        status |= LOCAL_bufWrite(LOCAL_uint2b(addr), 0, 4, ioTimeout);
				        Console.WriteLine("(AIS Parse): Jump to address 0x{0:X8}.", addr);
				        break;

			        case AIS_OP_jumpClose:
				        Console.WriteLine("(AIS Parse): Performing jump and close...");
				        // send address
				        addr = LOCAL_parseInt(AIS_Contents, ref AIS_Cursor);
				        status |= LOCAL_bufWrite(LOCAL_uint2b(addr), 0, 4, ioTimeout);
				        // parsing complete
				        status = AIS_complete;
				        Console.WriteLine("(AIS Parse): AIS complete. Jump to address 0x{0:X8}.", addr);
				        break;

			        case AIS_OP_startOver:
				        // control should never pass here; opcode is not present in AIS files
				        break;

			        // Unrecognized opcode
			        default:
				        Console.WriteLine("(AIS Parse): Unhandled opcode (0x{0:X8}).", command);
				        status = AIS_error;
                        break;
		        }
	        }

	        // UART only: read "   DONE"
	        //if (status == AIS_complete && hostDevice == AIS_UART)
                if (status == AIS_complete)
	        {
		        byte[] rcvEnd = new byte[8];
                byte[] corEnd = { (byte)' ', (byte)' ', (byte)' ', (byte)'D',
                                  (byte)'O', (byte)'N', (byte)'E', (byte)0 };

		        Console.WriteLine("(AIS Parse): Waiting for DONE...");
                status = UTIL_uartRead(rcvEnd, 0, 8, ioTimeout);

		        // fail on incorrect sequence or IO error
		        for (i = 0; i < 7; i++)
                    if (rcvEnd[i] != corEnd[i])
                    {
                        Console.WriteLine("(AIS Parse): Read invalid DONE string.");
                        status = AIS_error;
                        break;
                    }

                // success
                status = AIS_complete;
	        }

            if (status == AIS_complete)
	            Console.WriteLine("(AIS Parse): Boot completed successfully.");
            else
                Console.WriteLine("(AIS Parse): Boot aborted.");
	        return status;
        }

        // Sync Functions
        private int AIS_SWS()
        {
            uint rcvWord = 0;
            byte[] rcvWordB = new byte[4];
            uint xmtWord = AIS_XMT_START(ioBits);
            int status = 0;

            while (true)
            {
                // send xmt start
                status |= LOCAL_bufWrite(LOCAL_uint2b(xmtWord), 0, ioBits / 8, ioTimeout);
                if (status < 0)
                {
                    status = 0;
                    LOCAL_delay(opcodeDelay);
                    continue;
                }
                // receive word
                status |= LOCAL_bufRead(rcvWordB, 0, ioBits / 8, ioTimeout);
                rcvWord = LOCAL_b2uint(rcvWordB);

                // fail on IO error
                if (status < 0)
                    return AIS_error;

                // break if word is rcv start
                if (rcvWord == AIS_RCV_START(ioBits))
                    break;
            }

            return AIS_inProgress;
        }

        private int AIS_POS(uint command)
        {
            uint xmtWord = command;
            uint rcvWord;
            byte[] rcvWordB = new byte[4];
            int status, i;

            // 1. send ping
            status = LOCAL_bufWrite(LOCAL_uint2b(xmtWord), 0, 4, ioTimeout);
            // receive pong
            status |= LOCAL_bufRead(rcvWordB, 0, 4, ioTimeout);
            rcvWord = LOCAL_b2uint(rcvWordB);

            // fail on improper response or IO error
            if (rcvWord != AIS_opcode2ack(xmtWord) || status < 0)
                return AIS_error;

            LOCAL_delay(opcodeDelay);

            // 2. send N
            xmtWord = (uint)posN;
            // send ping
            status |= LOCAL_bufWrite(LOCAL_uint2b(xmtWord), 0, 4, ioTimeout);
            // receive pong
            status |= LOCAL_bufRead(rcvWordB, 0, 4, ioTimeout);
            rcvWord = LOCAL_b2uint(rcvWordB);

            // fail on improper response or IO error
            if (rcvWord != posN || status < 0)
                return AIS_error;

            // 3. send/receive numerical sequence
            for (i = 1; i <= posN; i++)
            {
                LOCAL_delay(opcodeDelay);

                xmtWord = (uint)i;
                status |= LOCAL_bufWrite(LOCAL_uint2b(xmtWord), 0, 4, ioTimeout);
                status |= LOCAL_bufRead(rcvWordB, 0, 4, ioTimeout);
                rcvWord = LOCAL_b2uint(rcvWordB);

                // fail on improper response or IO error
                if (rcvWord != xmtWord || status < 0)
                    return AIS_error;
            }

            return AIS_inProgress;
        }

        private int AIS_OS(uint command)
        {
            uint xmtWord = command;
            uint rcvWord = 0;
            byte[] rcvWordB = new byte[4];
            int status = 0;
            int retryCnt = 0;
            int retryCntMax = 10;

            while (true)
            {
                // send ping
                status |= LOCAL_bufWrite(LOCAL_uint2b(xmtWord), 0, 4, ioTimeout);
                // receive pong
                if (status >= 0)
                {
                    status |= LOCAL_bufRead(rcvWordB, 0, 4, ioTimeout);
                    rcvWord = LOCAL_b2uint(rcvWordB);
                }

                // fail on IO error
                if (status < 0)
                {
                    LOCAL_delay(opcodeDelay);
                    if (retryCnt++ >= retryCntMax)
                    {
                        Console.WriteLine("(AIS Parse): Opcode Sync failed after {0} consecutive I/O failures.", retryCnt);
                        return AIS_error;
                    }
                    status = 0;
                    continue;
                }

                // pass on proper response
                if (rcvWord == AIS_opcode2ack(xmtWord))
                {
                    if (retryCnt > 0)
                        Console.WriteLine("(AIS Parse): Opcode Sync passed after {0} consecutive I/O failures.", retryCnt);
                    return AIS_inProgress;
                }
            }
        }

        // read int (4 bytes) and advance cursor
        private uint LOCAL_parseInt(byte[] ais, ref int cursor)
        {
            uint token = 0;

            // assemble 4-byte uint in little-endian order
            for (int i = 0; i < 4; i++)
                token += (uint)(ais[cursor + i] << (i * 8));

            cursor += 4;
            return token;
        }

        // advance cursor arbitrary number of bytes (ignore data)
        private void LOCAL_parseSkip(ref int cursor, int n)
        {
	        cursor += n;
        }

        private int LOCAL_bufRead(byte[] buffer, int index, int bytes, int timeout)
        {
	        int rcvSize = ioBits / 8;
	        int status = 0;

            // check that we can read specified byte count cleanly
            if (bytes % rcvSize != 0)
            {
                Console.WriteLine("(AIS Parse): Cannot read {0} bytes in chunks of {1}!", bytes, rcvSize);
                return AIS_error;
            }

	        // perform IO transaction in N-bit words
	        for (int i = 0; i < bytes / rcvSize; i++)
	        {
                status |= UTIL_uartRead(buffer, index + i * rcvSize, rcvSize, timeout);
		        LOCAL_delay(ioDelay);

		        if (status < 0)
		        {
			        Console.WriteLine("(AIS Parse): I/O Error in read!");
			        break;
		        }
	        }

	        return status;
        }

        private int LOCAL_bufWrite(byte[] buffer, int index, int bytes, int timeout)
        {
	        int xmtSize = ioBits / 8;
	        int status = 0;

            // check that we can write specified byte count cleanly
            if (bytes % xmtSize != 0)
            {
                Console.WriteLine("(AIS Parse): Cannot write {0} bytes in chunks of {1}!", bytes, xmtSize);
                return AIS_error;
            }
        	
	        // perform IO transaction in N-bit words
	        for (int i = 0; i < bytes / xmtSize; i++)
	        {
		        status |= UTIL_uartWrite(buffer, index + i * xmtSize, xmtSize, timeout);
		        LOCAL_delay(ioDelay);

		        if (status < 0)
		        {
			        Console.WriteLine("(AIS Parse): I/O Error in write!");
			        break;
		        }
	        }

	        return status;
        }
        
        
        
       
       
        public void UTIL_log(String text)
        {
            
            Console.WriteLine(text);
        }

        

        private int UTIL_uartRead(byte[] rcvBuf, int index, int rcvSize, int timeout)
        {
            int bytesRead = 0;

            myUART.ReadTimeout = timeout;
            try
            {
                
                while (bytesRead < rcvSize)
                    bytesRead += myUART.Read(rcvBuf, index + bytesRead, rcvSize - bytesRead);
            }
            catch (Exception ex)
            {
                
                Console.WriteLine("(Serial Port): Read error! (" + ex.Message + ")");
                return -1;
            }

            return 0;
        }

        private int UTIL_uartWrite(byte[] xmtBuf, int index, int xmtSize, int timeout)
        {
            myUART.WriteTimeout = timeout;
            try
            {
                myUART.Write(xmtBuf, index, xmtSize);
            }
            catch (Exception ex)
            {
                Console.WriteLine("(Serial Port): Write error! (" + ex.Message + ")");
                return -1;
            }

            return 0;
        }
    }
}
