@echo off

set build="build/Win64"
if not exist "%build%" mkdir %build%
pushd %build%

cmake -G "Visual Studio 15 2017 Win64" "%~dp0" 

popd
