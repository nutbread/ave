@echo off

:: Clear
cls

:: Get variables
call vars.bat

:: Check if setup needs to be performed
if "%AVEXTENSIONS_SETUP_ARCHITECTURE%"=="" (
	:: It does
	call %SETENV% %SETENV_X64_FLAG%
	set AVEXTENSIONS_SETUP_ARCHITECTURE=x64

	:: Reset the color
	color
) else (
	:: Error messages
	if "%AVEXTENSIONS_SETUP_ARCHITECTURE%"=="x86" (
		echo Setup has been performed for x86
		echo To target x64, open a new terminal and run `setup64.bat`
	) else (
		echo Setup has already been performed
	)
)
