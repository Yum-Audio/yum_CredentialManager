#include "yum_CredentialManager.h"

/// For Keychain examples see:
/// https://es1015.tistory.com/243

#if JUCE_MAC || JUCE_IOS

#define Point CarbonDummyPoint
#define Rectangle CarbonDummyRect
#define Component CarbonDummyComp
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFUtilities.h>
#include <Foundation/Foundation.h>
#include <Security/Security.h>
#include <Security/SecCode.h>
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

Array<AppCredentials::UsernameAndPassword> AppCredentials::getAllStoredUsernamesAndPasswords (std::function<bool ()> onNoneFound)
{
    JUCE_AUTORELEASEPOOL
    {
            NSMutableDictionary *keychainItem = [NSMutableDictionary dictionary];
            auto serviceName = [[NSString alloc] initWithUTF8String: ProjectInfo::projectName];

            //Populate it with the data and the attributes we want to use.
             
            keychainItem[(__bridge id)kSecClass] = (__bridge id)kSecClassGenericPassword; // We specify what kind of keychain item this is.
//            keychainItem[(__bridge id)kSecAttrAccessible] = (__bridge id)kSecAttrAccessibleWhenUnlocked; // This item can only be accessed when the user unlocks the device.
        keychainItem[(__bridge id)kSecAttrAccessible] = (__bridge id)kSecAttrAccessibleAfterFirstUnlock;
            keychainItem[(__bridge id)kSecAttrService] = serviceName;
             
            //Check if this keychain item already exists.
//
            keychainItem[(__bridge id)kSecReturnPersistentRef] = (__bridge id)kCFBooleanTrue;
            keychainItem[(__bridge id)kSecReturnData] = (__bridge id)kCFBooleanTrue;
            keychainItem[(__bridge id)kSecReturnAttributes] = (__bridge id)kCFBooleanTrue;
     
            //@TODO: this still needs figuring out... how to get all results for the given app?
            //For now we're only fetching the first result it can find. This is fine for a single app user in most cases
            keychainItem[(__bridge id)kSecMatchLimitAll] = (__bridge id)kCFBooleanTrue;
        
//            CFMutableArrayRef result;
        
            CFDictionaryRef result = nil;
            OSStatus sts = SecItemCopyMatching((__bridge CFDictionaryRef)keychainItem, (CFTypeRef *)&result);
             
            NSLog(@"Error Code: %d", (int)sts);
             
            if(sts != noErr)
            {
                if (onNoneFound != nullptr)
                {
                    bool tryAgain = onNoneFound ();
                    if (tryAgain)
                    {
                        result=nil;
                        OSStatus rtr = SecItemCopyMatching((__bridge CFDictionaryRef)keychainItem, (CFTypeRef *)&result);
                         
                        NSLog(@"Retry Error Code: %d", (int)rtr);
                         
                        if(rtr == noErr)
                        {
                            
                        }
                        else
                        {
                            return {};
                        }
                    }
                    else
                    {
                        return {};
                    }
                }
                else
                {
#if ! RunHeadless
                    auto options = MessageBoxOptions ().withTitle ("No saved password found")
                                                       .withMessage("1. Please enter your username and password.\n\n2. Click \"Login\".\n\n3. When prompted, select to save password.")
                                                       .withButton("OK");
                    AlertWindow::showAsync (options, nullptr);
#endif
                    return {};
                }

           
            }
        
        Array<UsernameAndPassword> namesAndPasswords;
        
//        CFIndex c = CFArrayGetCount (result);
//        DBG ("Num entries: " << c );
//
//        for (CFIndex i = 0; i < c; i++)
//        {
//            auto resultDict = (CFDictionaryRef) CFArrayGetValueAtIndex (result, i);
////            NSString *user = resultDict[(__bridge id)kSecAttrAccount];
////            NSString *password = [[NSString alloc] initWithData:resultDict[(__bridge id)kSecValueData]
////                                                       encoding:NSUTF8StringEncoding];
////            namesAndPasswords.add ({[user UTF8String], [password UTF8String]});
//        }
//
//

            NSDictionary *resultDict = (NSDictionary *)result;
            NSString *user = resultDict[(__bridge id)kSecAttrAccount];
            NSString *password = [[NSString alloc] initWithData:resultDict[(__bridge id)kSecValueData]
                                                       encoding:NSUTF8StringEncoding];

            namesAndPasswords.add ({[user UTF8String], [password UTF8String]});
            
            return namesAndPasswords;
            
    }

    jassertfalse;
    return {};
}

//========================================================================
//========================================================================


String Certificates::getAppIdFromSignature (const File& f)
{
    String appId;
    JUCE_AUTORELEASEPOOL
    {
        SecStaticCodeRef code;
        CFURLRef url = (CFURLRef)[NSURL fileURLWithPath:[[NSString alloc] initWithUTF8String:f.getFullPathName().toUTF8()]];
        auto result = SecStaticCodeCreateWithPath (url, kSecCSDefaultFlags, &code);
        
        if (result == noErr)
        {
            CFDictionaryRef info;
            auto infoResult = SecCodeCopySigningInformation (code, kSecCSRequirementInformation, &info);
            
            if (infoResult == noErr)
            {
                appId = [[(id)info objectForKey:@"identifier"] UTF8String];
            }
            else
            {
                appId = "Error getting certificate info";
            }
        }
        else
        {
            appId = "Error getting static code reference";
        }
    }

    return appId;
}

String Certificates::getSignerIdentity (const File& f)
{
    String signerIdentity;
    JUCE_AUTORELEASEPOOL
    {
        SecStaticCodeRef code;
        CFURLRef url = (CFURLRef)[NSURL fileURLWithPath:[[NSString alloc] initWithUTF8String:f.getFullPathName().toUTF8()]];
        auto result = SecStaticCodeCreateWithPath (url, kSecCSDefaultFlags, &code);
        
        if (result == noErr)
        {
            CFDictionaryRef info;
            auto infoResult = SecCodeCopySigningInformation (code, kSecCSSigningInformation, &info);
            
            if (infoResult == noErr)
            {
                signerIdentity = [[(id)info objectForKey:@"teamid"] UTF8String];
            }
            else
            {
                signerIdentity = "Error getting certificate info";
            }
        }
        else
        {
            signerIdentity = "Error getting static code reference";
        }
    }

    return signerIdentity;
}

#endif //end JUCE_MAC
