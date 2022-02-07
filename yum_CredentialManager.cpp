#include "yum_CredentialManager.h"

#if ! RunHeadless
#include "UsernamePasswordUI.cpp"
#endif

#if JUCE_WINDOWS
/// https://stackoverflow.com/questions/9221245/how-do-i-store-and-retrieve-credentials-from-the-windows-vault-credential-manage

#include <windows.h>
#include <wincred.h>
#include <tchar.h>

#include "NativeWindowsHelpers.h"

using namespace YumAudio;

bool AppCredentials::updateEntry(const UsernameAndPassword& creds)
{
    const auto username = creds.first;
    const auto pw = creds.second;

    if (pw.isEmpty() || username.isEmpty()) return false;

    auto createEntry = [&, username, pw]() -> bool
    {
        std::wstring uname (username.toWideCharPointer ());
        const auto password = pw.toRawUTF8 ();
        std::wstring targetName ( (String(ProjectInfo::projectName) + "/" + username).toWideCharPointer());
    
        DWORD cbCreds = 1 + strlen (password);


        CREDENTIALW cred = { 0 };
        cred.Type = CRED_TYPE_GENERIC;
        cred.TargetName = &targetName[0];
        cred.CredentialBlobSize = cbCreds;
        cred.CredentialBlob = (LPBYTE)password;
        cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
        cred.UserName = &uname[0];

        BOOL ok = ::CredWriteW(&cred, 0);
        wprintf(L"CredWrite() - errno %d\n", ok ? 0 : ::GetLastError());

        return ok;
    };

    auto credentialsExist = userCredentialsExist(username);
    if (!credentialsExist)
    {
#if RunHeadless
        return createEntry();
#else
        String credClientName = "Keychain";
#if JUCE_WINDOWS
        credClientName = "Credential Manager";
#endif
        auto o = MessageBoxOptions().withTitle("Save login data in " + credClientName + "? ")
            .withMessage("Do you want to store your login data in " + credClientName + "?")
            .withButton("Yes").withButton("No");

        AlertWindow::showAsync(o, [&, createEntry](int result)
        {
            if (result == 1)
                createEntry();
        });

        return true;
#endif

    }
    else //credentials exist, check if we have to update
    {
        const auto currentlyStoredPassword = getPasswordForUsername(username);
        if (currentlyStoredPassword != pw)
        {
            //update
            auto updatePassword = [&, pw, username, createEntry]() -> bool
            {
                return createEntry();
            };

#if RunHeadless
            return updatePassword();
#else
            auto o = MessageBoxOptions().withTitle("It seems your password has changed")
                .withMessage("Do you want to update your saved login?")
                .withButton("Yes").withButton("No");

            AlertWindow::showAsync(o, [&, updatePassword](int result)
            {
                if (result == 1)
                    updatePassword();
            });

            return true;
#endif

        }

        return false;
    }
}

bool AppCredentials::anyExist()
{
    return ! getAllAvailableEntries ().isEmpty ();
}

bool AppCredentials::userCredentialsExist(const Username& username)
{
    return getPasswordForUsername (username).isNotEmpty ();
}

StringArray AppCredentials::getAllAvailableEntries (const String& filter)
{
    StringArray entries;
    PCREDENTIALA* creds;
    DWORD count;
    std::string targetName ((String(ProjectInfo::projectName) + "/" + filter + "*").toRawUTF8());
    LPCSTR fltr = targetName.c_str();
    BOOL ok { ::CredEnumerateA (fltr, NULL, &count, &creds) };
    if (!ok)
    {
        return {};
    }

    for (int i = 0; i < count; i++)
    {
        auto cred = creds[i];
        auto credTarget = cred->TargetName;

        if (String(credTarget).contains(ProjectInfo::projectName))
            entries.add(cred->UserName);
    }

    CredFree(creds);
  
    return entries;
}

String AppCredentials::getPasswordForUsername (const Username& username)
{
    std::wstring targetName((String(ProjectInfo::projectName) + "/" + username).toWideCharPointer());
    
    PCREDENTIALW pcred;
    BOOL ok = ::CredReadW(&targetName[0], CRED_TYPE_GENERIC, 0, &pcred);
    wprintf(L"CredRead() - errno %d\n", ok ? 0 : ::GetLastError());
    
    if (!ok)
         return {};

    jassert(String(pcred->UserName) == username);

    wprintf(L"Read username = '%s', password='%S' (%d bytes)\n",
        pcred->UserName, (char*)pcred->CredentialBlob, pcred->CredentialBlobSize);
    
    String pw((char*)pcred->CredentialBlob, pcred->CredentialBlobSize);
   
    return pw;
}

Array<UsernameAndPassword> AppCredentials::getAllStoredUsernamesAndPasswords (std::function<bool()> onNoneFound)
{
    Array<UsernameAndPassword> creds;
    const auto entries = getAllAvailableEntries();

    if (entries.isEmpty() && onNoneFound != nullptr)
    {
        auto tryAgain = onNoneFound();
        if (tryAgain)
            return getAllStoredUsernamesAndPasswords(onNoneFound);
    }

    for (auto& e : entries)
    {
        const auto pass = getPasswordForUsername(e);
        creds.add({ e, pass });
    }

    return creds;
}

//========================================================================
//========================================================================
String Certificates::getSignerIdentity (const File& f)
{
    String path = f.getFullPathName ();
    LPTSTR identity = NULL;

    WindowsNative::getSignerIdentity (&path, identity);

    if (identity != NULL)
        return identity;
    else return { "Error, couldn't read identity from signed file: " + f.getFullPathName() };
}

#endif
