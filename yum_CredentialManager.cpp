#include "yum_CredentialManager.h"

#if JUCE_WINDOWS
/// https://stackoverflow.com/questions/9221245/how-do-i-store-and-retrieve-credentials-from-the-windows-vault-credential-manage

#include <windows.h>
#include <wincred.h>
#include <tchar.h>

using namespace YumAudio;

bool AppCredentials::createUsernameAndPasswordEntry (const AppCredentials::UsernameAndPassword& creds)
{
    std::wstring username (creds.first.toWideCharPointer ());
    const auto password = creds.second.toRawUTF8 ();
    std::wstring targetName ( (String(ProjectInfo::projectName) + "/account").toWideCharPointer () );
  
    DWORD cbCreds = 1 + strlen (password);


    CREDENTIALW cred = { 0 };
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = &targetName[0];
    cred.CredentialBlobSize = cbCreds;
    cred.CredentialBlob = (LPBYTE)password;
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    cred.UserName = &username[0];

    BOOL ok = ::CredWriteW(&cred, 0);
    wprintf(L"CredWrite() - errno %d\n", ok ? 0 : ::GetLastError());

    return ok;
}

bool AppCredentials::usernameAndPasswordCredentialsExist()
{
    std::wstring targetName((String(ProjectInfo::projectName) + "/account").toWideCharPointer());

    PCREDENTIALW pcred;
    BOOL ok = ::CredReadW (&targetName[0], CRED_TYPE_GENERIC, 0, &pcred);
    wprintf(L"CredRead() - errno %d\n", ok ? 0 : ::GetLastError());

    ::CredFree(pcred);

    return ok;
}

Array<AppCredentials::UsernameAndPassword> AppCredentials::getAllStoredUsernamesAndPasswords(std::function<bool()> onNoneFound)
{
    Array < AppCredentials::UsernameAndPassword > usernamesAndPasswords;

    std::wstring targetName((String(ProjectInfo::projectName) + "/account").toWideCharPointer());

    PCREDENTIALW pcred;
    BOOL ok = ::CredReadW(&targetName[0], CRED_TYPE_GENERIC, 0, &pcred);
    wprintf(L"CredRead() - errno %d\n", ok ? 0 : ::GetLastError());
  
    if (!ok && onNoneFound != nullptr)
    {
        auto tryAgain = onNoneFound();
        if (!tryAgain)
        {
            return {};
        }
        else
        {
            ok = ::CredReadW(&targetName[0], CRED_TYPE_GENERIC, 0, &pcred);
        }
    }

    wprintf(L"Read username = '%s', password='%S' (%d bytes)\n",
        pcred->UserName, (char*)pcred->CredentialBlob, pcred->CredentialBlobSize);

    String pw((char*)pcred->CredentialBlob, pcred->CredentialBlobSize);
    String username(pcred->UserName);

    /// TODO: How to retrieve all entries for a given app?

    usernamesAndPasswords.add({ username, pw });
    // must free memory allocated by CredRead()!
    ::CredFree(pcred);

    return usernamesAndPasswords;
}

//========================================================================
//========================================================================
String Certificates::getAppIdFromSignature (const File& f)
{
    jassertfalse;//tbd
}

#endif
