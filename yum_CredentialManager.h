/***********************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION

  ID:               yum_CredentialManager
  vendor:           Yum Audio
  version:          1.0.0
  name:             Yum Audio - Credential Manager
  description:      A wrapper for Keychain on macOS and CredentialManager on Windows
  website:          https://yum-audio.com
  license:          MIT

  dependencies:    juce_core
  OSXFrameworks:   Security
  iOSFrameworks:   Security
  linuxLibs:
  mingwLibs:

 END_JUCE_MODULE_DECLARATION
***********************************************************************************/
#pragma once
#include <JuceHeader.h>
#include "Types.h"

/** Config: RunHeadless
          If set to Enabled,  automatic dialogs will be disabled getAllStoredUsernamesAndPasswords (). This allows for usage of the Credential Manager in CommandLine applications and Plug-Ins
*/
#ifndef RunHeadless
#define RunHeadless 0
#endif

namespace YumAudio
{

using namespace juce;

//========================================================================
//========================================================================
struct AppCredentials
{
    /// returns, if item was added to system credential manager successfully
    static bool updateEntry (const UsernameAndPassword& creds);
    
    /// returns if at least one item for the given app exists in system credential manager
    static bool anyExist ();

    /// returns if a given username already has credentials stored in the keychain
    static bool userCredentialsExist (const Username& username);
    
    /// returns a list of all Username/Password items in Keychain/Credential Manager, but only the Usernames.
    /// Use a username to call getPasswordForUsername (const String& username)
    static StringArray getAllAvailableEntries (const String& filter = {});

    /// returns a password to a username that was found via getAllAvailableEntries ()
    static String getPasswordForUsername (const Username& username);
    
    /// returns an arry containing all credentials that could be found for the app
    /// 1st argument is callback on "no item found", if not defined a standard error will popup if no item can be found for the app
    /// callback return defines if password should be read again after callback finished
    /// !!! Depending on the keychain settings, this might require MULTIPLE password entries on Mac. It might be safer to just use getAllAvailableEntries () and call getPasswordForUsername () with a specific entry only
    static Array<UsernameAndPassword> getAllStoredUsernamesAndPasswords (std::function<bool ()> onNoneFound = nullptr);
    
    class UsernamePasswordUI;
    #include "UsernamePasswordUI.h"
    
};

//========================================================================
//========================================================================
struct Certificates
{
    /// on Mac this will return the 10-letter Apple TeamID associated with a certificate (e.g. "SFXXXXXXXT")
    /// on Windows this will return the human readable name of the signer (e.g. "Yum Audio GmbH & Co. KG")
    static String getSignerIdentity (const File& f);

#if JUCE_MAC
    /// returns the app id imprinted on a certificate (e.g. com.YumAudio.Spread)
    static String getAppIdFromSignature (const File& f);
#endif
    
};

};
