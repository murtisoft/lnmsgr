Notes
===========================
After the recent two crash fixes, I am hoping that this is reliable enough to actually use, now.

I would like to maintain compatibility between this and old versions, while adding more in the future. 

I would also discourage translators from contributing for now, the interface is constantly changing.

How to compile LAN Messenger on Windows
============================
This project was compiled on Windows 11 with;

* Qt Creator 18.0.1   (C:/Qt/)
* Qt 6.10.1 MinGW 64 bit
* Win64OpenSSL-3.6.0  (C:/Qt/Tools/OpenSSL-Win64)
	* For compiled binaries: slproweb.com/products/Win32OpenSSL.html
* Libopus (C:/Qt/Tools/opus-1.6)
	* Compile the project into (/build/release)

Footnotes
============================
Most of the old project was borked so I started by removing some dead weight.
Obviously, we will need some of those back (Like the setup scripts.)
