@echo off
setlocal
:: KeiluVisionBuilder.cmd
:: Written by Flemming Steffensen, 2019.
:: Free for use and abuse by anyone.
:: ======================
:: Configuration

set WaitForLicenseTimeout=60
set BuildAttemptsMax=10

set "ProjectFileName=Template.uvprojx"
set "ProjectPath=D:\SourceCode\_Avery_M031\M031BSP_private_test\SampleCode\Template\Keil\"
set "Compiler=C:\Keil_v5\UV4\UV4.exe"
set "OutFolder=obj\"
:: ======================
:: Do not edit below this line
set BuildAttempt=0

pushd %ProjectPath%

:PerformBuild
echo:
echo:Performing Keil Build...

if exist build.log                del build.log
if exist obj\*.build_log.htm  del %OutFolder%*.build_log.htm

start /wait %Compiler% -j0 -b %ProjectFileName% -o build.log
set ReportedError=%ERRORLEVEL%

:: Scan build.log to determine if the license is locked.
find /c "Error: *** All Flex licenses are in use! ***" build.log  >nul
if %errorlevel% equ 0 (
    set /a BuildAttempt=%BuildAttempt% + 1 
    echo:Error: *** All Flex licenses are in use!
    if %BuildAttempt% equ %BuildAttemptsMax% goto NoLicenseAvailable
    echo:Retrying ^(%BuildAttempt% of %BuildAttemptsMax%^) in %WaitForLicenseTimeout% seconds
    waitfor SignalNeverComming /T %WaitForLicenseTimeout% >nul 2>&1
    goto PerformBuild
) 
:: Scan alternative build.log to determine if the license is locked.
find /c "Failed to check out a license" %OutFolder%*.build_log.htm >nul
if %errorlevel% equ 0 (
    set /a BuildAttempt=%BuildAttempt% + 1 
    echo:Error: Failed to check out a license
    if %BuildAttempt% equ %BuildAttemptsMax% goto NoLicenseAvailable
    echo:Retrying ^(%BuildAttempt% of %BuildAttemptsMax%^) in %WaitForLicenseTimeout% seconds
    waitfor SignalNeverComming /T %WaitForLicenseTimeout% >nul 2>&1
    goto PerformBuild
)
goto NoLicenseProblem


:NoLicenseAvailable
echo:Error: After %BuildAttempt% attempts, the Flex license still appear to be unavailable. Failing the build!
echo:
popd
exit /b 1

:NoLicenseProblem
:: Parse exit codes
set KnownErrors=0 1 2 3 11 12 13 15 20 41

echo:Keil compiler exited with error code %ReportedError%

for %%a in (%KnownErrors%) do (
   if [%ReportedError%] equ [%%a] goto Error%ReportedError%
)
goto UnknownError

:Error0
   echo Compilation successful
   goto ExitButContinueJob

:Error1
   echo Warnings were found
   goto ExitButContinueJob

:Error2 
   echo Errors were found
   goto ExitCritical

:Error3
   echo Error 3 = Fatal Errors
   goto ExitCritical

:Error11
   echo Error 11 = Cannot open project file for writing
   goto ExitCritical

:Error12
   echo Error 12 = Device with given name in not found in database
   goto ExitCritical

:Error13
   echo Error 13 = Error writing project file
   goto ExitCritical

:Error15
   echo Error 15 = Error reading import XML file
   goto ExitCritical

:Error20
   echo Error 20 = Can not convert the project file.
   goto ExitCritical

:Error41
   echo Error 41 = Can not create the logfile requested using the -l switch.
   goto ExitCritical

:UnknownError
   echo Error %ReportedError% = Unknown error 
   goto ExitCritical

:ExitCritical
echo:
if [%ReportedError%] neq 0 exit /b %ReportedError% 

:ExitButContinueJob
echo:
popd
::exit /b 0