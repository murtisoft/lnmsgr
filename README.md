Notes
===========================
Encryption has been a headache to fix, unfortunately. I hope to fix it eventually, but in the meantime, you should realize messages (including nudges) might fail to encrypt or decrypt, practically "disappearing". Broadcasts, file transfers, public chats or chatrooms are not affected, as they are not encrypted.

I would like to maintain compatibility between this and old versions, while adding more in the future. 

I would also discourage translators from contributing for now, the interface is constantly changing.

Why LAN Messenger? (2026)
============================
* No accounts, no logins.
* Serverless. No need to keep a computer on, just to communicate.
* Complete privacy. Your data stays within your network.
* Lightweight. Stays out of your way.
* Internet independent. Feel the joy of asking a peer: "Is the internet working?"
* Nudging. Annoy your friends shamelessly!
* 1 on 1 Encrypted communications.
* Voice chat. (Eventually)
* Video chat/desktop sharing (Eventually)
* Multi platform (Eventually - older linux & mac versions are compatible.)

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
