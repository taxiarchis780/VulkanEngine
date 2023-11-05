@echo off
setlocal enabledelayedexpansion

cd F:\VulkanProject\VulkanProj\VulkanProject\shaderSrc\

for %%a in (*.vert) do (
	set str=%%a
	set "str=!str:.=_!"
	echo !str!
	for /f %%b in ('F:\VulkanSDK\1.3.250.0\Bin\glslc.exe --target-env^=vulkan1.2 --target-spv^=spv1.5 F:\VulkanProject\VulkanProj\VulkanProject\shaderSrc\%%a -o F:\VulkanProject\VulkanProj\VulkanProject\x64\Release\res\shaders\!str!.spv') do (
    		set "result=%%b"
    		echo Result: !result!
	)
	echo.
)

for %%a in (*.frag) do (
	set str=%%a
	set "str=!str:.=_!"
	echo !str!
	for /f %%b in ('F:\VulkanSDK\1.3.250.0\Bin\glslc.exe --target-env^=vulkan1.2 --target-spv^=spv1.5 F:\VulkanProject\VulkanProj\VulkanProject\shaderSrc\%%a -o F:\VulkanProject\VulkanProj\VulkanProject\x64\Release\res\shaders\!str!.spv') do (
    		set "result=%%b"
    		echo Result: !result!
	)
	echo.
)
endlocal


