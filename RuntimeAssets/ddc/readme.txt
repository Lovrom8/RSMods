-------------------------------------------------------------
-- DDC v3.6 (c) 2013-2018, by Chlipouni
-------------------------------------------------------------

DDC is a command line tool conceived to add Dynamic Difficulty Levels on Custom Downloadable Contents.

Installation :
--------------
Just decompress the "zip" file in a folder of your choice.

Use :
-----
- Open a Windows command-line interpreter
- Execute the "ddc.exe" tool with the following parameters :

  C:\ddc_v3.6>ddc.exe
  -----------------------------------------------------------------------
  -- D Y N A M I C   D I F F I C U L T Y   C R E A T O R   v 3.6       --
  -----------------------------------------------------------------------
  ddc.exe <arrangement> [-l <phrase length>] [-s {Y | N}] [-c <config file>] [-m <ramp-up model>] [-p {Y | N}] [-t {Y | N}]
  
  Parameters :
  ------------
    <arrangement>      : XML arrangement input file (mandatory)
    -l <phrase length> : Length of phrases in number of measures (optional; default : 4)
    -s {Y | N}         : Remove sustain for notes with length < 1/4 of measure (optional; default : N)
    -c <config file>   : Configuration file to use for adjustment of internal parameters (optional; default : internal configuration)
    -m <ramp-up model> : XML file with the specific ramp-up model to apply (optional; default : internal ramp-up model)
    -p {Y | N}         : Preserve the XML file name, so existing content is overwritten (optional; default : N)
    -t {Y | N}         : Trace the DDC process and generate log files (optional; default : Y)

  Example :
  ------------
  C:\mySongs>C:\ddc_v3.6\ddc.exe "PART REAL_GUITAR.xml" -l 3 -s Y

Result Files :
--------------
- If "-p" equals "N", the result XML file is named "DDC_<fileName>.xml" otherwise, the input XML file is overwritten
- If "-t" equals "Y", a log file is automatically created as "DDC_<fileName>.log"

Return Error Codes :
--------------------
0 : Ends normally with no error
1 : Ends with system error
2 : Ends with application error
