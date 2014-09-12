@echo off

:: Clear
cls

:: Get variables
call vars.bat

:: Check if setup needs to be performed
if "%AVEXTENSIONS_SETUP_ARCHITECTURE%"=="" (
	:: It does
	call %SETENV% %SETENV_X86_FLAG%
	set AVEXTENSIONS_SETUP_ARCHITECTURE=x86

	:: Reset the color
	color
) else (
	:: Error messages
	if "%AVEXTENSIONS_SETUP_ARCHITECTURE%"=="x64" (
		echo Setup has been performed for x64
		echo To target x86, open a new terminal and run `setup.bat`
	) else (
		echo Setup has already been performed
	)
)
