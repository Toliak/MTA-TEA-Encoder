# MTA-TEA-Encoder
*compiled via CLion*

-----
## Encoding files with [Tiny Encoding Algorythm from Multy Theft Auto](https://wiki.multitheftauto.com/wiki/TeaEncode)
Open any file with this programm (or just drag'n'drop it) to encode it.
Programm will create MD5-named file in the same folder.

- You can decode it on client or server side:
```
key = "sample key"
base64Decode(teaDecode(base64Encode(crypt), key))
```

- it can be used for binary files (base64 encoding before TEA encoding allows to encode '\0')