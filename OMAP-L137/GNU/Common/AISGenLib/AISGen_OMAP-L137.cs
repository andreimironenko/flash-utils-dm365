 /* --------------------------------------------------------------------------
    FILE        : AISGen_OMAP-L137.cs
    PROJECT     : TI Booting and Flashing Utilities
    AUTHOR      : Daniel Allred
    DESC        : Concrete AISGen class implemenatation for OMAP-L137
 ----------------------------------------------------------------------------- */

using System;
using System.Text;
using System.Text.RegularExpressions;
using System.IO;
using System.IO.Ports;
using System.Reflection;
using System.Threading;
using System.Globalization;
using System.Collections;
using UtilLib.IO;
using UtilLib.CRC;

namespace AISGenLib
{
  /// <summary>
  /// AISGen class that is specific to the device (inherits from abtract base class AISGen)
  /// </summary>
  public class AISGen_OMAP_L137:AISGen
  {
    /// <summary>
    /// String definitions for built-in ROM functions
    /// </summary>
    public struct ROMFunctionNames
    {
      public const String PLLConfig           = "PLLConfig";
      public const String PeriphClockConfig   = "PeriphClockConfig";
      public const String EMIF3CConfigSDRAM   = "EMIF3CConfigSDRAM";
      public const String EMIF25ConfigSDRAM   = "EMIF25ConfigSDRAM";
      public const String EMIF25ConfigAsync   = "EMIF25ConfigAsync";
      public const String PLLandClockConfig   = "PLLandClockConfig";
      public const String PSCConfig           = "PSCConfig";
      public const String PINMUXConfig        = "PinMuxConfig";
      public const String FastBoot            = "FastBoot";
      public const String IOPUConfig          = "IOPUConfig";
      public const String MPUConfig           = "MPUConfig";
      public const String TAPSConfig          = "TAPSConfig";
    }
    
    /// <summary>
    /// The public constructor for the OMAP_L137 device AIS generator.
    /// The constructor is where the device differentiation is defined.
    /// </summary>
    public AISGen_OMAP_L137()
    {
      // Define the device name - used for default file names
      devNameShort = "OMAP-L137";
      devNameLong = "OMAPL137";

      // Define the device caches (they are considered internal memories since 
      // bootrom turns off caching) -  two identical sets since the memory map 
      // has the caches at two locations
      Cache = new CacheInfo[6];
      Cache[0].level = CacheLevel.L2;
      Cache[0].type = CacheType.Program | CacheType.Data;
      Cache[0].startAddr = 0x00800000;
      Cache[0].size = 0x40000;
            
      Cache[1].level = CacheLevel.L1;
      Cache[1].type = CacheType.Program;
      Cache[1].startAddr = 0x00E08000;
      Cache[1].size = 0x8000;
            
      Cache[2].level = CacheLevel.L1;
      Cache[2].type = CacheType.Data;
      Cache[2].startAddr = 0x00F10000;
      Cache[2].size = 0x8000;

      Cache[3].level = CacheLevel.L2;
      Cache[3].type = CacheType.Program | CacheType.Data;
      Cache[3].startAddr = 0x10800000;
      Cache[3].size = 0x40000;

      Cache[4].level = CacheLevel.L1;
      Cache[4].type = CacheType.Program;
      Cache[4].startAddr = 0x10E08000;
      Cache[4].size = 0x8000;

      Cache[5].level = CacheLevel.L1;
      Cache[5].type = CacheType.Data;
      Cache[5].startAddr = 0x10F10000;
      Cache[5].size = 0x8000;

      // Define the IDMA channels for internal memory transfers
      IDMA = new IDMARegisters[2];
      IDMA[0] = new IDMARegisters(0, 0x01820000);
      IDMA[1] = new IDMARegisters(1, 0x01820100);
            
      // Define OMAP-L138 ROM boot loader functions
      ROMFunc = new ROMFunction[12];
      UInt32 i = 0;
      
      ROMFunc[i].funcName = ROMFunctionNames.PLLConfig;
      ROMFunc[i].iniSectionName = "PLLCONFIG";
      ROMFunc[i].numParams = 2;
      ROMFunc[i].paramNames = new String[2] { "PLLCFG0", "PLLCFG1" };
      i++;
      
      ROMFunc[i].funcName = ROMFunctionNames.PeriphClockConfig;
      ROMFunc[i].iniSectionName = "PERIPHCLKCFG";
      ROMFunc[i].numParams = 1;
      ROMFunc[i].paramNames = new String[1] { "PERIPHCLKCFG" };
      i++;

      ROMFunc[i].funcName = ROMFunctionNames.EMIF3CConfigSDRAM;
      ROMFunc[i].iniSectionName = "EMIF3SDRAM";
      ROMFunc[i].numParams = 4;
      ROMFunc[i].paramNames = new String[4] { "SDCR", "SDTIMR", "SDTIMR2", "SDRCR" };
      i++;

      ROMFunc[i].funcName = ROMFunctionNames.EMIF25ConfigSDRAM;
      ROMFunc[i].iniSectionName = "EMIF25SDRAM";
      ROMFunc[i].numParams = 4;
      ROMFunc[i].paramNames = new String[4] { "SDBCR", "SDTIMR", "SDRSRPDEXIT", "SDRCR"};
      i++;
      
      ROMFunc[i].funcName = ROMFunctionNames.EMIF25ConfigAsync;
      ROMFunc[i].iniSectionName = "EMIF25ASYNC";
      ROMFunc[i].numParams = 4;
      ROMFunc[i].paramNames = new String[4] { "A1CR", "A2CR", "A3CR", "A4CR" };
      i++;
      
      ROMFunc[i].funcName = ROMFunctionNames.PLLandClockConfig;
      ROMFunc[i].iniSectionName = "PLLANDCLOCKCONFIG";
      ROMFunc[i].numParams = 3;
      ROMFunc[i].paramNames = new String[3] { "PLLCFG0", "PLLCFG1", "PERIPHCLKCFG" };
      i++;
      
      ROMFunc[i].funcName = ROMFunctionNames.PSCConfig;
      ROMFunc[i].iniSectionName = "PSCCONFIG";
      ROMFunc[i].numParams = 1;
      ROMFunc[i].paramNames = new String[1] { "LPSCCFG" };
      i++;

      ROMFunc[i].funcName = ROMFunctionNames.PINMUXConfig;
      ROMFunc[i].iniSectionName = "PINMUX";
      ROMFunc[i].numParams = 3;
      ROMFunc[i].paramNames = new String[3] { "REGNUM", "MASK", "VALUE" };
      i++;
      
      ROMFunc[i].funcName = ROMFunctionNames.FastBoot;
      ROMFunc[i].iniSectionName = "FASTBOOT";
      ROMFunc[i].numParams = 0;
      ROMFunc[i].paramNames = null;
      i++;
      
      ROMFunc[i].funcName = ROMFunctionNames.IOPUConfig;
      ROMFunc[i].iniSectionName = "IOPUCONFIG";
      ROMFunc[i].numParams = 2;
      ROMFunc[i].paramNames = new String[2] { "IOPUSELECT", "MPPAVALUE" };
      i++;
      
      ROMFunc[i].funcName = ROMFunctionNames.MPUConfig;
      ROMFunc[i].iniSectionName = "MPUCONFIG";
      ROMFunc[i].numParams = 4;
      ROMFunc[i].paramNames = new String[4] { "MPUSELECT", "STARTADDR" ,"ENDADDR" ,"MPPAVALUE" };
      i++;
      
      ROMFunc[i].funcName = ROMFunctionNames.TAPSConfig;
      ROMFunc[i].iniSectionName = "TAPSCONFIG";
      ROMFunc[i].numParams = 1;
      ROMFunc[i].paramNames = new String[1] { "TAPSCFG" };
      i++;
      
      // Configuration info for the AISExtras functions (provided in AISExtraFileName COFF file)
      AISExtraFileName = null;

      AISExtraFunc = null;

      // OMAP-L138 is little endian
      devEndian = Endian.LittleEndian;
      
      // OMAP-L138 AIS data is little endian;
      devAISEndian = Endian.LittleEndian;

      // Default settings for UARTSendDONE function
      UARTSendDONEAddr = 0x0;
      SendUARTSendDONE = false;

      // Default boot mode (can be changed from INI file) for this device
      bootMode = BootModes.NONE;

      // Create default CRC object for this device
      devCRC = new CRC32(0x04C11DB7, 0x00000000, 0x00000000, false, 1, UtilLib.CRC.CRCType.INCREMENTAL, UtilLib.CRC.CRCCalcMethod.BITWISE);
      
      crcType = CRCCheckType.NO_CRC;
    } 
  }
} //end of AISGenLib namespace
