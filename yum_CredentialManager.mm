#include "yum_CredentialManager.h"

/// For Keychain examples see:
/// https://es1015.tistory.com/243
/// https://cpp.hotexamples.com/examples/-/-/SecItemCopyMatching/cpp-secitemcopymatching-function-examples.html
///
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
#include "UsernamePasswordUI.cpp"

using namespace YumAudio;

bool AppCredentials::updateKeychainEntry (const UsernameAndPassword& creds)
{
    const auto username = creds.first;
    const auto pw = creds.second;
    
    if (pw.isEmpty () || username.isEmpty ()) return false;
    
    auto createEntry = [&, username, pw]() -> bool
    {
        JUCE_AUTORELEASEPOOL
        {
            const int numOptions = 5;
            
            CFStringRef keys [numOptions];
            keys[0] = kSecClass;
            keys[1] = kSecAttrAccount;
            keys[2] = kSecValueData;
            keys[3] = kSecAttrService;
            keys[4] = kSecAttrAccessible;

            CFTypeRef values [numOptions];
            values[0] = kSecClassGenericPassword;
            values[1] = username.toCFString();
            values[2] = pw.toCFString ();
            values[3] = String (ProjectInfo::projectName).toCFString();
            values[4] = kSecAttrAccessibleAfterFirstUnlock;
            
            CFDictionaryRef query;
            query = CFDictionaryCreate (kCFAllocatorDefault,
                                        (const void**) keys,
                                        (const void**) values,
                                        numOptions, NULL, NULL);

            return SecItemAdd (query, NULL) == 0;
        }
    };
    
    auto credentialsExist = userCredentialsExist (username);
    if ( ! credentialsExist )
    {
#if RunHeadless
        return createEntry ();
#else
        auto o = MessageBoxOptions ().withTitle("Save login data in Keychain?")
                                     .withMessage ("Do you want to store your login data in Keychain?")
                                     .withButton ("Yes").withButton ("No");

        AlertWindow::showAsync (o, [&, createEntry](int result)
        {
            if (result == 1)
                createEntry ();
        });
        
        return true;
#endif

    }
    else //credentials exist, check if we have to update
    {
        const auto currentlyStoredPassword = getPasswordForUsername (username);
        if (currentlyStoredPassword != pw)
        {
            //update
            auto updatePassword = [&, pw, username, createEntry]() -> bool
            {
                JUCE_AUTORELEASEPOOL
                {
                    const auto serviceName = [[NSString alloc] initWithUTF8String: ProjectInfo::projectName];
                    
                    CFMutableDictionaryRef query = CFDictionaryCreateMutable (NULL, 0,
                                                                             &kCFTypeDictionaryKeyCallBacks,
                                                                             &kCFTypeDictionaryValueCallBacks);
                    
                    CFDictionarySetValue (query, kSecClass, kSecClassGenericPassword);
                    CFDictionarySetValue (query, kSecAttrService, serviceName);
                    CFDictionarySetValue (query, kSecAttrAccount, [[NSString alloc] initWithUTF8String:username.toRawUTF8()]);
                    CFDictionarySetValue (query, kSecReturnAttributes, kCFBooleanTrue);
                    CFDictionarySetValue (query, kSecReturnData, kCFBooleanTrue);
                    
                    /// I can't figure out for the life of me how to use SecItemUpdate (),
                    /// so we do it the rough way, deleting the item and recreating it
                    /// i have tried every single example I was able to find, but if you think you can manage
                    /// be my guest and submit a PR using SecItemUpdate () here instead
                    OSStatus status = SecItemDelete((__bridge CFDictionaryRef)query);
                    
                    if (status == errSecSuccess)
                    {
                        DBG("Item deleted... recreating now");
                        return createEntry ();
                    }
                    else
                    {
                        jassertfalse; // error handling?
                        return false;
                    }
                }
            };
            
#if RunHeadless
            return updatePassword ();
#else
            auto o = MessageBoxOptions ().withTitle("It seems your password has changed")
                                         .withMessage ("Do you want to update your saved login?")
                                         .withButton ("Yes").withButton ("No");

            AlertWindow::showAsync (o, [&, updatePassword](int result)
            {
                if (result == 1)
                    updatePassword ();
            });
            
            return true;
#endif

        }
        
        return false;
    }
}

bool AppCredentials::anyCredentialsExist ()
{
    JUCE_AUTORELEASEPOOL
    {
       //Let's create an empty mutable dictionary:
       NSMutableDictionary *keychainItem = [NSMutableDictionary dictionary];
        
       auto serviceName = [[NSString alloc] initWithUTF8String: ProjectInfo::projectName];
     
       //Populate it with the data and the attributes we want to use.
        
       keychainItem[(__bridge id)kSecClass] = (__bridge id)kSecClassGenericPassword; // We specify what kind of keychain item this is.
       keychainItem[(__bridge id)kSecAttrAccessible] = (__bridge id)kSecAttrAccessibleAfterFirstUnlock; // This item can only be accessed when the user unlocks the device.
       keychainItem[(__bridge id)kSecAttrService] = serviceName;
        
       //Check if this keychain item exists.
       return SecItemCopyMatching((__bridge CFDictionaryRef)keychainItem, NULL) == noErr;
    }
}

bool AppCredentials::userCredentialsExist (const Username& username)
{
    const auto allEntriesForUser = getAllAvailableEntries (username);
    return ! allEntriesForUser.isEmpty ();
}

StringArray AppCredentials::getAllAvailableEntries (const String& filter)
{
    JUCE_AUTORELEASEPOOL
    {
        StringArray entries;
        const auto serviceName = [[NSString alloc] initWithUTF8String: ProjectInfo::projectName];
    
        CFMutableDictionaryRef query = CFDictionaryCreateMutable(NULL, 0,
                                                                &kCFTypeDictionaryKeyCallBacks,
                                                                &kCFTypeDictionaryValueCallBacks);
     
        CFDictionarySetValue (query, kSecReturnRef, kCFBooleanTrue);
        CFDictionarySetValue (query, kSecReturnAttributes, kCFBooleanTrue);
        CFDictionarySetValue (query, kSecMatchLimit, kSecMatchLimitAll);
        CFDictionarySetValue (query, kSecAttrService, (__bridge id)serviceName);
        CFDictionarySetValue (query, kSecClass, kSecClassGenericPassword);
        
        CFArrayRef items = NULL;
        
        OSStatus status = SecItemCopyMatching(query, (CFTypeRef *)&items);

        if (status == errSecSuccess)
        {
            const auto count = CFArrayGetCount (items);
            for (int j = 0; j < count; j++)
            {
                CFDictionaryRef properties = (CFDictionaryRef) CFArrayGetValueAtIndex (items, j);
                const String accountName = [(NSString*)CFDictionaryGetValue(properties, kSecAttrAccount) UTF8String];
              
                if (filter.isEmpty () || accountName.contains (filter))
                    entries.add (accountName);
            }
            
            entries.sort (true);
            return entries;
        }
        else if (status == errSecItemNotFound)
        {
            //No password found for service (/app)
            return {};
        }
        else
        {
            jassertfalse; //do we need to handle any errors?
        }
    }
    
    jassertfalse;
    return {};
}

String AppCredentials::getPasswordForUsername (const Username& username)
{
    JUCE_AUTORELEASEPOOL
    {
        const auto serviceName = [[NSString alloc] initWithUTF8String: ProjectInfo::projectName];
        
        CFMutableDictionaryRef query = CFDictionaryCreateMutable(NULL, 0,
                                                                &kCFTypeDictionaryKeyCallBacks,
                                                                &kCFTypeDictionaryValueCallBacks);
        
        CFDictionarySetValue (query, kSecClass, kSecClassGenericPassword);
        CFDictionarySetValue (query, kSecAttrService, serviceName);
        CFDictionarySetValue (query, kSecAttrAccount, [[NSString alloc] initWithUTF8String:username.toRawUTF8()]);
        CFDictionarySetValue (query, kSecReturnAttributes, kCFBooleanTrue);
        CFDictionarySetValue (query, kSecReturnData, kCFBooleanTrue);
        
        CFDictionaryRef result = nil;
        OSStatus status = SecItemCopyMatching((__bridge CFDictionaryRef)query, (CFTypeRef *)&result);
        
        if (status == errSecSuccess)
        {
            const auto resultDict = (NSDictionary *)result;
            const auto password = [[NSString alloc] initWithData:resultDict[(__bridge id)kSecValueData]
                                                        encoding:NSUTF8StringEncoding];
            return [password UTF8String];
        }
        else
        {
            jassertfalse;
            return {};
        }
    }
    
    jassertfalse;
    return {};
}

Array<UsernameAndPassword> AppCredentials::getAllStoredUsernamesAndPasswords (std::function<bool ()> onNoneFound)
{
    JUCE_AUTORELEASEPOOL
    {
      
//        NSMutableDictionary *query = [NSMutableDictionary dictionary];
//        auto serviceName = [[NSString alloc] initWithUTF8String: ProjectInfo::projectName];
//
//            //Populate it with the data and the attributes we want to use.
//
//        query[(__bridge id)kSecClass] = (__bridge id)kSecClassGenericPassword; // We specify what kind of keychain item this is.
//        query[(__bridge id)kSecAttrService] = serviceName;
//        query[(__bridge id)kSecReturnRef] = (__bridge id)kCFBooleanTrue;
//        query[@"kSecMatchLimit"] = (__bridge id)kSecMatchLimitAll;
        
//        query[(__bridge id)kSecReturnRef] = (__bridge id)kCFBooleanTrue;
//            keychainItem[(__bridge id)kSecAttrAccessible] = (__bridge id)kSecAttrAccessibleAfterFirstUnlock; // This item can only be accessed when the user unlocks the device.
//        query[(__bridge id)kSecAttrAccessible] = (__bridge id)kSecAttrAccessibleAfterFirstUnlock;
           
             
            //Check if this keychain item already exists.
//
        
//
//            query[(__bridge id)kSecReturnAttributes] = (__bridge id)kCFBooleanTrue;
     
            //@TODO: this still needs figuring out... how to get all results for the given app?
            //For now we're only fetching the first result it can find. This is fine for a single app user in most cases
      
//        CFArrayRef result = nil;
//
////            CFDictionaryRef result = nil;
//        OSStatus sts = SecItemCopyMatching((__bridge CFDictionaryRef)query, (CFTypeRef *)&result);
//
//        NSLog(@"Error Code: %d", (int)sts);
//
//        if(sts != noErr)
//        {
//            if (onNoneFound != nullptr)
//            {
//                bool tryAgain = onNoneFound ();
//                if (tryAgain)
//                {
//                    result=nil;
//                    OSStatus rtr = SecItemCopyMatching((__bridge CFDictionaryRef)query, (CFTypeRef *)&result);
//
//                    NSLog(@"Retry Error Code: %d", (int)rtr);
//
//                    if(rtr == noErr)
//                    {
//
//                    }
//                    else
//                    {
//                        return {};
//                    }
//                }
//                else
//                {
//                    return {};
//                }
//            }
//            else
//            {
//#if ! RunHeadless
//                auto options = MessageBoxOptions ().withTitle ("No saved password found")
//                                                    .withMessage("1. Please enter your username and password.\n\n2. Click \"Login\".\n\n3. When prompted, select to save password.")
//                                                    .withButton("OK");
//                AlertWindow::showAsync (options, nullptr);
//#endif
//                return {};
//            }
//
//
//        }
//
//        Array<UsernameAndPassword> namesAndPasswords;
//
//        CFIndex c = CFArrayGetCount (result);
//        DBG ("Num entries: " << c );
//
//        for (CFIndex i = 0; i < c; i++)
//        {
//            auto resultDict = CFArrayGetValueAtIndex(result, i);
//
//
//        }
////
//

//            NSDictionary *resultDict = (NSDictionary *)result;
//            NSString *user = resultDict[(__bridge id)kSecAttrAccount];
//            NSString *password = [[NSString alloc] initWithData:resultDict[(__bridge id)kSecValueData]
//                                                       encoding:NSUTF8StringEncoding];
//
//            namesAndPasswords.add ({[user UTF8String], [password UTF8String]});
            
//            return namesAndPasswords;
            
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
