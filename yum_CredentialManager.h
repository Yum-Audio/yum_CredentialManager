/***********************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION

  ID:                  yum_CredentialManager
  vendor:           Yum Audio
  version:          1.0.0
  name:             Yum Audio - Credential Manager
  description:    A wrapper for Keychain on macOS and ??? on Windows
  website:         https://yum-audio.com
  license:          MIT

  dependencies:     juce_core
  OSXFrameworks: Security
  iOSFrameworks: Security
  linuxLibs:
  mingwLibs:

 END_JUCE_MODULE_DECLARATION
***********************************************************************************/

#pragma once

#include <JuceHeader.h>

namespace YumAudio
{

using namespace juce;



struct AppCredentials
{
    typedef std::pair<String, String> UsernameAndPassword;
   
    /// returns, if item was added to system credential manager successfully
    static bool createUsernameAndPasswordEntry (const UsernameAndPassword& creds);
    
    /// returns if at least one item for the given app exists in system credential manager
    static bool usernameAndPasswordCredentialsExist ();
    
    /// returns an arry containing all credentials that could be found for the app
    /// 1st argument is callback on "no item found", if not defined a standard error will popup if no item can be found for the app
    ///  callback return defines if password should be read again after callback finished
    static Array<UsernameAndPassword> getAllStoredUsernamesAndPasswords (std::function<bool ()> onNoneFound = nullptr);
};

};
