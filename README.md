# EmbedExeReg2
This is a port from the original EmbedExeReg from x86Matthew (https://www.x86matthew.com/view_post?id=embed_exe_reg). 

EnbedExeReg2 is a tool that embeds a binary file inside a registry file. I rewrote this file because the original code (03-08-2022) was not working for me. I got a few errors so I decided to make a few rewrites to make it work.

So ALL credit goed to x86matthew!! I only made a few minor adjustments to make the code work for me.

How does the code work:

1. The tool creates a .reg file and embeds data to create .bat files and embeds binary data we provide
2. The user receives the .reg file we created with this tool and double-clicks it
3. The .reg file adds 2 keys to the registry (to the Run or RunOnce keys).
4. When the user reboots the first Run(Once) key searches for the original .reg file on disk and copies it to the user's temp folder ad a .bat file
5. The second Run(Once) key copies this .bat file to a second .bat file while removing the top data (registry data)
6. The second .bat file is executed and re-compiles the binary data to a .exe file with a random name and executes this binary

The original code worked with 1 registry key and only 1 .bat file. Ofcourse we can overwrite the existing .bat file but for now I have chosen to create a second one. The .bat file was crashing on my machine on the registry data. Besides that did the original code holds some characters that my compiler was crashing on (^ / not used for a XOR operation). 

See it in action (Dutch): https://www.youtube.com/watch?v=CwWutqb0fkM

I used Visual Studio to compile the C++ code as C++ Console App
*note, when using Visual Studio check the Project Character Set is "Use Multi-Byte Character Set" and not "Use Use Unicode Character Set".


