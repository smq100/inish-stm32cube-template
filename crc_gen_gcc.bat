@echo off
setlocal
pushd "%~dp0"
echo CD=%CD%

ECHO -------------------------------------
ECHO Computing CRC
REM Batch script for generating CRC in SW4STM32 project
REM Must be placed at SW4STM32/<TARGET> folder

REM Path configuration
SET SREC_PATH=.
SET TARGET_NAME=.\Debug\inish-stm32cube-template
SET BYTE_SWAP=1
SET COMPARE_HEX=0
SET CRC_ADDR_FROM_MAP=1
REM Not used when CRC_ADDR_FROM_MAP=1
SET CRC_ADDR=0x08009000

REM Derived configuration
SET MAP_FILE=.\Debug\inish-stm32cube-template.map
SET INPUT_BIN=%TARGET_NAME%.bin -binary
SET INPUT_HEX=%TARGET_NAME%.hex -intel
SET OUTPUT_HEX=%TARGET_NAME%_CRC.hex -intel
SET TMP_FILE=.\Debug\crc_tmp_file.txt

IF NOT "%CRC_ADDR_FROM_MAP%"=="1" goto:end_of_map_extraction
REM Extract CRC address from MAP file
REM -----------------------------------------------------------
ECHO Extracting CRC address from MAP file
REM Prefer the _Check_Sum symbol address. Some map files show .check_sum
REM start before alignment, so using section start can place CRC incorrectly.
SET "CRC_ADDR="
for /f "tokens=1" %%a in ('findstr /r /c:"0x[0-9A-Fa-f][0-9A-Fa-f]*[ ]*PROVIDE[ ]*([_]Check_Sum[ ]*=[ ]*[.])" "%MAP_FILE%"') do (
  set "CRC_ADDR=%%a"
  goto:crc_addr_found
)

REM Fallback for older map formats if _Check_Sum symbol is not found.
for /f "tokens=2" %%a in ('findstr /r /c:"^[.]check_sum[ ]*0x[0-9A-Fa-f][0-9A-Fa-f]*" "%MAP_FILE%"') do (
  set "CRC_ADDR=%%a"
  goto:crc_addr_found
)
:crc_addr_found
if "%CRC_ADDR%"=="" (
  ECHO ERROR: Could not extract CRC address from %MAP_FILE%
  ECHO Expected a line like: .check_sum      0x0800xxxx
  exit /b 1
)
REM -----------------------------------------------------------
REM End of CRC address extraction
:end_of_map_extraction
REM Convert BIN to HEX with offset
%SREC_PATH%\srec_cat.exe ^
  %INPUT_BIN% ^
  -offset 0x08000000 ^
  -o %INPUT_HEX%

REM Compute CRC and store it to new HEX file
ECHO CRC address: %CRC_ADDR%
if "%BYTE_SWAP%"=="1" (
REM ECHO to see what is going on
ECHO %SREC_PATH%\srec_cat.exe ^
  %INPUT_HEX% ^
  -crop 0x08000000 %CRC_ADDR% ^
  -byte_swap 4 ^
  -stm32-b-e %CRC_ADDR% ^
  -byte_swap 4 ^
  -o %TMP_FILE% -intel
%SREC_PATH%\srec_cat.exe ^
  %INPUT_HEX% ^
  -crop 0x08000000 %CRC_ADDR% ^
  -byte_swap 4 ^
  -stm32-b-e %CRC_ADDR% ^
  -byte_swap 4 ^
  -o %TMP_FILE% -intel
) else (
REM ECHO to see what is going on
ECHO %SREC_PATH%\srec_cat.exe ^
  %INPUT_HEX% ^
  -crop 0x08000000 %CRC_ADDR% ^
  -stm32-l-e %CRC_ADDR% ^
  -o %TMP_FILE% -intel
%SREC_PATH%\srec_cat.exe ^
  %INPUT_HEX% ^
  -crop 0x08000000 %CRC_ADDR% ^
  -stm32-l-e %CRC_ADDR% ^
  -o %TMP_FILE% -intel
)
ECHO %SREC_PATH%\srec_cat.exe ^
  %INPUT_HEX% -exclude -within %TMP_FILE% -intel ^
  %TMP_FILE% -intel ^
  -o %OUTPUT_HEX%
%SREC_PATH%\srec_cat.exe ^
  %INPUT_HEX% -exclude -within %TMP_FILE% -intel ^
  %TMP_FILE% -intel ^
  -o %OUTPUT_HEX%
REM Delete temporary file
DEL %TMP_FILE%
ECHO Modified HEX file with CRC stored at %OUTPUT_HEX%

REM Compare input HEX file with output HEX file
if %COMPARE_HEX%==1 (
ECHO Comparing %INPUT_HEX% with %OUTPUT_HEX%
%SREC_PATH%\srec_cmp.exe ^
%INPUT_HEX% %OUTPUT_HEX% -v
)

ECHO Post-build finished. ERRORLEVEL=%ERRORLEVEL%
ECHO -------------------------------------

popd
exit /b 0
