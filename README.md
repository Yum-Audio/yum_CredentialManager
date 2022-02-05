# yum_CredentialManager
Yum Audio Credential Manager

Please note, that this code is not yet production-ready. This is a work in development and everything is subject to changes. 

A JUCE module wrapping different tools/APIs for easy App Credential management.  
- Keychain on macOS (/iOS untested) 
- CredentialManager on Windows 
- Crypt32 certificate reading on Windows
- SecCodeCopySigningInformation certificate reading on Mac


### Username/Password Credential storage/retrieval:
- Currently only a single password per application is supported. 

### Certificates:
- getAppIdFromSignature () is currently Mac only

## TODO: 
- Implement retrieval of multiple passwords on both platforms. "getAllStoredUsernamesAndPasswords ()" only retrieves one entry on both platforms currently.
- Clean up, especially the Obj-C file for Mac
- Add a standard alert for Username/Password retrieval on Windows if "onNoneFound == nullptr", like in Mac implementation
- Android & Linux support
- Certificate::getAppIdFromSignature () for Windows
