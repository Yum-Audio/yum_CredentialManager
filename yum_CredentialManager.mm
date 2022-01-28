#include "yum_CredentialManager.h"

/// For examples see:
/// https://es1015.tistory.com/243

#if JUCE_MAC

#define Point CarbonDummyPoint
#define Rectangle CarbonDummyRect
#define Component CarbonDummyComp
#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>
#include <Security/Security.h>
#undef Point
#undef Rectangle
#undef Component

#include <JuceHeader.h>

using namespace YumAudio;

bool AppCredentials::createUsernameAndPasswordEntry (const UsernameAndPassword& creds)
{
    const auto username = creds.first;
    const auto pw = creds.second;
    
    JUCE_AUTORELEASEPOOL
    {
        CFStringRef keys[4];
        keys[0] = kSecClass;
        keys[1] = kSecAttrAccount;
        keys[2] = kSecValueData;
        keys[3] = kSecAttrService;

        CFTypeRef values[4];
        values[0] = kSecClassGenericPassword;
        values[1] = username.toCFString();
        values[2] = pw.toCFString ();
        values[3] = String (ProjectInfo::projectName).toCFString();

        CFDictionaryRef query;
        query = CFDictionaryCreate (kCFAllocatorDefault,
                                    (const void**) keys,
                                    (const void**) values,
                                    4, NULL, NULL);

        return SecItemAdd (query, NULL) == 0;
    }
}

bool AppCredentials::usernameAndPasswordCredentialsExist ()
{
    JUCE_AUTORELEASEPOOL
    {
       //Let's create an empty mutable dictionary:
       NSMutableDictionary *keychainItem = [NSMutableDictionary dictionary];
        
       auto serviceName = [[NSString alloc] initWithUTF8String: ProjectInfo::projectName];
     
       //Populate it with the data and the attributes we want to use.
        
       keychainItem[(__bridge id)kSecClass] = (__bridge id)kSecClassGenericPassword; // We specify what kind of keychain item this is.
       keychainItem[(__bridge id)kSecAttrAccessible] = (__bridge id)kSecAttrAccessibleWhenUnlocked; // This item can only be accessed when the user unlocks the device.
       keychainItem[(__bridge id)kSecAttrService] = serviceName;
        
       //Check if this keychain item exists.
       return SecItemCopyMatching((__bridge CFDictionaryRef)keychainItem, NULL) == noErr;
    }
}

Array<AppCredentials::UsernameAndPassword> AppCredentials::getAllStoredUsernamesAndPasswords (std::function<void ()> onNoneFound)
{
    JUCE_AUTORELEASEPOOL
    {
            NSMutableDictionary *keychainItem = [NSMutableDictionary dictionary];
            auto serviceName = [[NSString alloc] initWithUTF8String: ProjectInfo::projectName];

            //Populate it with the data and the attributes we want to use.
             
            keychainItem[(__bridge id)kSecClass] = (__bridge id)kSecClassGenericPassword; // We specify what kind of keychain item this is.
            keychainItem[(__bridge id)kSecAttrAccessible] = (__bridge id)kSecAttrAccessibleWhenUnlocked; // This item can only be accessed when the user unlocks the device.
            keychainItem[(__bridge id)kSecAttrService] = serviceName;
             
            //Check if this keychain item already exists.
            keychainItem[(__bridge id)kSecReturnData] = (__bridge id)kCFBooleanTrue;
            keychainItem[(__bridge id)kSecReturnAttributes] = (__bridge id)kCFBooleanTrue;
        
            CFDictionaryRef result = nil;
             
            OSStatus sts = SecItemCopyMatching((__bridge CFDictionaryRef)keychainItem, (CFTypeRef *)&result);
             
            NSLog(@"Error Code: %d", (int)sts);
             
            if(sts != noErr)
            {
                if (onNoneFound != nullptr)
                {
                    MessageManager::callAsync (onNoneFound);
                }
                else
                {
                    auto options = MessageBoxOptions ().withTitle ("No saved password found")
                                                       .withMessage("1. Please enter your username and password.\n\n2. Click \"Login\".\n\n3. When prompted, select to save password.")
                                                       .withButton("OK");
                    AlertWindow::showAsync (options, nullptr);
                }

                return {};
            }
         
            NSDictionary *resultDict = (__bridge_transfer NSDictionary *)result;
        
            NSString *usr = resultDict[(__bridge id)kSecAttrAccount];
            
            NSData *pswd = resultDict[(__bridge id)kSecValueData];
            NSString *password = [[NSString alloc] initWithData:pswd encoding:NSUTF8StringEncoding];
         
        Array<UsernameAndPassword> namesAndPasswords;
        namesAndPasswords.add ({[usr UTF8String], [password UTF8String]});
        return namesAndPasswords;
            
    }

    jassertfalse;
    return {};
}

#endif //end JUCE_MAC
