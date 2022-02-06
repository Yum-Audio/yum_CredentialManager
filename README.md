# yum_CredentialManager
Yum Audio Credential Manager

A JUCE module wrapping different tools/APIs for easy App Credential management.  
- Keychain on macOS (/iOS untested) 
- CredentialManager on Windows 
- Crypt32 certificate reading on Windows
- SecCodeCopySigningInformation certificate reading on Mac


**Please note, that this code is not yet production-ready. This is a work in development and everything is subject to changes.** 


### Certificates:
- getAppIdFromSignature () is currently Mac only

## TODO: 
- Clean up
- DRY up code between Mac/Windows, there are several identical functions in the .mm/.cpp file respectively, that get managed by OS defines. Only OS calls need different implementations, all juce calls can get combined
- Android & Linux support
- Certificate::getAppIdFromSignature () for Windows
