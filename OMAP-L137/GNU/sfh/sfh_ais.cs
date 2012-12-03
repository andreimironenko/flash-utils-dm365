/* --------------------------------------------------------------------------
    FILE        : sfh_ais.cs
    PROJECT     : TI Booting and Flashing Utilities for OMAP-L137
    AUTHOR      : Stanley Park
    DESC        : Host program for flashing via UART
 ----------------------------------------------------------------------------- */

using System;
using System.Text;
using System.IO;
using System.IO.Ports;
using System.Reflection;
using System.Threading;
using System.Globalization;
using UtilLib;
using UtilLib.CRC;
using UtilLib.IO;
using UtilLib.ConsoleUtility;
using AIS_Util;

using System.Windows.Forms;


[assembly: AssemblyTitle("SerialFlasherHost")]
[assembly: AssemblyVersion("1.50.*")]


namespace TIBootAndFlash
{
  /// <summary>
  /// Main program Class
  /// </summary>
  /// 

    

  partial class Program
  {
    //**********************************************************************************
    #region Class variables and members

    /// <summary>
    /// Global main Serial Port Object
    /// </summary>
    public static SerialPort MySP;
            
    /// <summary>
    /// The main thread used to actually execute everything
    /// </summary>
    public static Thread workerThread;

    /// <summary>
    /// Global boolean to indicate successful completion of workerThread
    /// </summary>
    public static Boolean workerThreadSucceeded = false;

    /// <summary>
    /// String to hold the BootImage name
    /// </summary>

    public static String BootImage;
    /// <summary>
    /// String to hold the AppImage name
    /// </summary>
    public static String AppImage;
    //String to designate port number
    public static String PortNum;

    #endregion
    //**********************************************************************************


    //**********************************************************************************
    #region Code for Main thread

    /// <summary>
    /// Help Display Function
    /// </summary>
    private static void DispHelp()
    {
      Console.Write("\n\nUsage: sfh_ais.exe <AISBOOT> <APP> <PORT> \n");
	  Console.Write("<AISBOOT>: AISBootImage (ex. flasher.ais) \n");
      Console.Write("<APP>    : Application image to flash (ex. e3L3.ais) \n");
      Console.Write("<PORT>   : Designated COM port number (ex. COM5) \n");
    }   

    /// <summary>
    /// Parse the command line into the appropriate internal command structure
    /// </summary>
    /// <param name="args">The array of strings passed to the command line.</param>
   
    /// <summary>
    /// Main entry point of application
    /// </summary>
    /// <param name="args">Array of command-line arguments</param>
    /// <returns>Return code: 0 for correct exit, -1 for unexpected exit</returns>
    static Int32 Main(String[] args)
    {

        if (args.Length != 3)
        {
            DispHelp();
            return 0;
        }

        BootImage = args[0]; //set first arg as boot image name
        AppImage = args[1];  //set second arg as application image name
		PortNum = args[2];   //set thrid arg as port 
        try
        {
            Console.WriteLine("Attempting to connect to device " + PortNum + "...");
            MySP = new SerialPort(PortNum, 230400, Parity.None, 8, StopBits.One);
            MySP.Encoding = Encoding.ASCII;
            MySP.Open();
        }
      catch(Exception e)
      {
        if (e is UnauthorizedAccessException)
        {
          Console.WriteLine(e.Message);
          Console.WriteLine("This application failed to open the COM port.");
          Console.WriteLine("Most likely it is in use by some other application.");
          return -1;
        }
        
          Console.WriteLine(e.Message);
        return -1;
      }

      Console.WriteLine("Press any key to end this program at any time.\n");
      
      // Setup the thread that will actually do all the work of interfacing to
      // the Device boot ROM.  Start that thread.


      
      workerThread = new Thread(new ThreadStart(Program.WorkerThreadStart));
     
      workerThread.Start(); 

      
      // Wait for a key to terminate the program
      while ((workerThread.IsAlive) && (!Console.KeyAvailable))
      {
        Thread.Sleep(1000);
      }
                 
      // If a key is pressed then abort the worker thread and close the serial port
      try
      {
        if (Console.KeyAvailable)
        {
          Console.ReadKey();
          Console.WriteLine("Aborting program...");
          workerThread.Abort();
        }
        else if (workerThread.IsAlive)
        {
          Console.WriteLine("Aborting program...");
          workerThread.Abort();
        }
        
        while ((workerThread.ThreadState & ThreadState.Stopped) != ThreadState.Stopped){}
      }
      catch (Exception e)
      {
        Console.WriteLine("Abort thread error...");
        Console.WriteLine(e.GetType());
        Console.WriteLine(e.Message);
      }
      
      if (workerThreadSucceeded)
      {
        return 0;
      }
      else
      {
        Console.WriteLine("\n\nInterfacing to the OMAP-L137 via UART failed." +
            "\nPlease reset or power-cycle the board and try again...");
        return -1;
      }
      
    }

    #endregion
    //**********************************************************************************
      

    //**********************************************************************************
    

    /// <summary>
    /// The main function of the thread where all the cool stuff happens
    /// to interface with the device
    /// </summary>
   
    public static void WorkerThreadStart()
    {
       
      try
      {      
        
        Byte[] imageData, imageData2;
        Byte[] AppSize = new Byte[4];
       
        
          
        // Read the extracted embedded SFT data that we will transmit
        imageData = FileIO.GetFileData(BootImage); //read UART boot image
        imageData2 = FileIO.GetFileData(AppImage); //read Application image

        AppSize = BitConverter.GetBytes(imageData2.Length);//Get App image size


          Console.WriteLine("Entering AIS_Parser");


          AIS_Parser parser = new AIS_Parser(MySP); 
        
          parser.boot(imageData); // Start the ROM booting 

          Console.Write("\n Interfacing with SPIFlasher on Device... \n");

          parser.SPIFlasher(); // Interfacing with SPIFlasher to check the UART connection

        
          parser.ApplicationStart(imageData2, imageData2.Length, AppSize); //Interfacing with SPIFlasher to send the application image


        
          //******************************************************************************************************************
      }
     
      
      catch (Exception e)
      {
        if (e is ThreadAbortException)
        {
          Thread.Sleep(1000);
        }
        else
        {
          Console.WriteLine(e.Message);
        }
        return;
      }
        
      
      workerThreadSucceeded = true;
    }

     //**********************************************************************************

  }

 
}
