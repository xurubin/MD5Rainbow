@echo off
echo 
echo 1 Core
echo 2 Cores
echo 3 Cores
echo 4 Cores
echo 5 Cores
echo 6 Cores
echo 7 Cores
echo 8 Cores
set /p core=Please select number of cores(1-8):
if "%core%"=="8" goto core8
if "%core%"=="7" goto core7
if "%core%"=="6" goto core6
if "%core%"=="5" goto core5
if "%core%"=="4" goto core4
if "%core%"=="3" goto core3
if "%core%"=="2" goto core2
if "%core%"=="1" goto core1
goto end

:core8
MD5Rainbow lookup real 8
goto end

:core7
MD5Rainbow lookup real 7
goto end

:core6
MD5Rainbow lookup real 6
goto end

:core5
MD5Rainbow lookup real 5
goto end

:core4
MD5Rainbow lookup real 4
goto end

:core3
MD5Rainbow lookup real 3
goto end

:core2
MD5Rainbow lookup real 2
goto end

:core1
MD5Rainbow lookup real 1
goto end


:end