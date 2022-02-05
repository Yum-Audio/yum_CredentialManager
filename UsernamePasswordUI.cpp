#include "UsernamePasswordUI.h"

#include "yum_CredentialManager.h"

using namespace YumAudio;

AppCredentials::UsernamePasswordUI::UsernamePasswordUI ()
{
    addAndMakeVisible (loginButton);
    loginButton.onClick = [&]()
    {
        attemptLogin ();
    };
    
    auto credPopupFromUsername = [&]()
    {
        updateCredentialsPopup (&usernameEditor);
    };
    auto credPopupFromPassword = [&]()
    {
        updateCredentialsPopup (&passwordEditor);
    };
    
    auto onEditorLostFocus = [&]()
    {
        credentialsPopup.reset ();
    };
    
    auto onEnter = [&]()
    {
        attemptLogin ();
    };
    
    addAndMakeVisible (usernameEditor);
    usernameEditor.setEscapeAndReturnKeysConsumed(false);
    usernameEditor.setTextToShowWhenEmpty ("Username",
                                           Colours::grey);
    usernameEditor.onTextChange = credPopupFromUsername;
    usernameEditor.onMouseDown = credPopupFromUsername;
    usernameEditor.onFocusLost = onEditorLostFocus;
    usernameEditor.onReturnKey = onEnter;
    
    addAndMakeVisible (passwordEditor);
    passwordEditor.setPasswordCharacter ('*');
    passwordEditor.setEscapeAndReturnKeysConsumed(false);
    passwordEditor.setTextToShowWhenEmpty ("Password",
                                           Colours::grey);
    passwordEditor.onMouseDown = credPopupFromPassword;
    passwordEditor.onFocusLost = onEditorLostFocus;
    passwordEditor.onReturnKey = onEnter;
}

UsernameAndPassword AppCredentials::UsernamePasswordUI::getCurrentEditorCredentials ()
{
    return { usernameEditor.getText (), passwordEditor.getText () };
}

void AppCredentials::UsernamePasswordUI::resetPasswordEditor ()
{
    passwordEditor.setText ("", dontSendNotification);
}

void AppCredentials::UsernamePasswordUI::loginButtonClicked (const UsernameAndPassword& credentials)
{
    /// You'll have to either make a subclass of AppCredentials::UsernamePasswordUI and overwrite this method
    /// >>> OR <<<
    /// define a callback function in public member std::function<void (const UsernameAndPassword&)> onLoginButtonClicked
    ///
    /// if a callback is defined this virtual function will never get a call
    jassertfalse;
}

void AppCredentials::UsernamePasswordUI::resized ()
{
    auto area = getLocalBounds ();
    auto elementHeight = area.getHeight () / elements.size ();
    
    for (auto& e : elements)
        e->setBounds (area.removeFromTop (elementHeight).reduced (2));
}

void AppCredentials::UsernamePasswordUI::attemptLogin ()
{
    const auto cred = getCurrentEditorCredentials ();
    
    if (onLoginButtonClicked != nullptr)
        onLoginButtonClicked (cred);
    else loginButtonClicked (cred);
}

void AppCredentials::UsernamePasswordUI::updateCredentialsPopup (Component* source)
{
    closeCredentialsPopup ();
    
    auto entries = AppCredentials::getAllAvailableEntries (usernameEditor.getText ());

    if ( ! entries.isEmpty () )
    {
        credentialsPopup = std::make_unique<PopupMenu>();
        
        for (auto& e : entries)
            credentialsPopup->addItem (e, [&, e] () { fillEditorsForKeychainUser (e); });
        
        const auto options = PopupMenu::Options ()
                             .withTargetComponent (source)
                             .withPreferredPopupDirection (PopupMenu::Options::PopupDirection::downwards);
        credentialsPopup->showMenuAsync (options);
    }
}

void AppCredentials::UsernamePasswordUI::fillEditorsForKeychainUser (const Username& user)
{
    const Password pass = AppCredentials::getPasswordForUsername (user);
    if (pass.isNotEmpty ())
    {
        usernameEditor.setText (user, dontSendNotification);
        passwordEditor.setText (pass, dontSendNotification);
    }
    
    closeCredentialsPopup ();
    attemptLogin ();
}

void AppCredentials::UsernamePasswordUI::closeCredentialsPopup ()
{
    PopupMenu::dismissAllActiveMenus ();
    credentialsPopup.reset ();
}
