# yum_CredentialManager
Yum Audio Credential Manager

A JUCE module wrapping Keychain on macOS and CredentialManager on Windows for easy App Credential management. 

Currently only a single password per application is supported. 

# TODO: 
- Implement retrieval of multiple passwords on both platforms. "getAllStoredUsernamesAndPasswords ()" only retrieves one entry on both platforms currently.
- Clean up, especially the Obj-C file for Mac
- Add a standard alert on Windows if "onNoneFound == nullptr", like in mac implementation
