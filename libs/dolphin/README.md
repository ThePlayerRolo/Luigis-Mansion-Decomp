Code is mainly based on [the SDK 2001 Decomp by doldecomp.][https://github.com/doldecomp/dolsdk2001]

OSExi uses [SMS's EXIBios][https://github.com/doldecomp/sms/blob/6a8600bc70bb9d6ac3fd2840c6faa16029c9a442/src/dolphin/exi/EXIBios.c] for EXISync and carries macros from it
OSExi also includes exi now and uses the definitions from there (which for some reason didn't match the OSExi ones??)
OSSerial's SIEnablePolling is [SMS's variant][https://github.com/doldecomp/sms/blob/main/src/dolphin/si/SIBios.c#L398]

OSReboot is ripped from SMS[https://github.com/doldecomp/sms/blob/main/src/dolphin/os/OSReboot.c#L99] with some modifications somewhat based on [Moddinations decomp][https://github.com/Moddimation/Yasiki/blob/main/decomp/DolphinSDK/src/dolphin/os/OSReboot.c#L89] for __OSReboot
