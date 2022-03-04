#include "UsernamePasswordUI.h"

#include "yum_CredentialManager.h"

using namespace YumAudio;

typedef AppCredentials::UsernamePasswordUI UsernamePasswordUI;

UsernamePasswordUI::UsernamePasswordUI ()
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
        closeCredentialsPopup ();
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

UsernamePasswordUI::~UsernamePasswordUI ()
{
    closeCredentialsPopup ();
}

UsernameAndPassword UsernamePasswordUI::getCurrentEditorCredentials ()
{
    return { usernameEditor.getText (), passwordEditor.getText () };
}

void UsernamePasswordUI::resetPasswordEditor ()
{
    passwordEditor.setText ("", dontSendNotification);
}

void UsernamePasswordUI::setPopupLookAndFeel (LookAndFeel_V4* laf)
{
    closeCredentialsPopup ();
    popupLaf.reset (laf);
}

void UsernamePasswordUI::loginButtonClicked (const UsernameAndPassword& credentials)
{
    /// You'll have to either make a subclass of AppCredentials::UsernamePasswordUI and overwrite this method
    /// >>> OR <<<
    /// define a callback function in public member std::function<void (const UsernameAndPassword&)> onLoginButtonClicked
    ///
    /// if a callback is defined this virtual function will never get a call
    jassertfalse;
}

void UsernamePasswordUI::resized ()
{
    auto area = getLocalBounds ();
    auto elementHeight = area.getHeight () / elements.size ();
    
    for (auto& e : elements)
        e->setBounds (area.removeFromTop (elementHeight).reduced (2));
}

void UsernamePasswordUI::attemptLogin ()
{
    closeCredentialsPopup ();
    
    const auto cred = getCurrentEditorCredentials ();
    if (cred.first.isEmpty () || cred.second.isEmpty ()) return;
    
    if (onLoginButtonClicked != nullptr)
        onLoginButtonClicked (cred);
    else loginButtonClicked (cred);
}

void UsernamePasswordUI::updateCredentialsPopup (Component* source)
{
    closeCredentialsPopup ();
    
    auto entries = AppCredentials::getAllAvailableEntries (usernameEditor.getText ());

    if ( ! entries.isEmpty () )
    {
        credentialsPopup = std::make_unique<PopupMenu>();
        
        if (popupLaf != nullptr)
            credentialsPopup->setLookAndFeel (popupLaf.get ());
        
        for (auto& e : entries)
            credentialsPopup->addItem (e, [&, e] () { fillEditorsForKeychainUser (e); });
        
        const auto options = PopupMenu::Options ()
                             .withTargetComponent (source)
                             .withPreferredPopupDirection (PopupMenu::Options::PopupDirection::downwards)
                              .withMinimumWidth (source->getWidth());
        credentialsPopup->showMenuAsync (options);
    }
}

void UsernamePasswordUI::fillEditorsForKeychainUser (const Username& user)
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

void UsernamePasswordUI::closeCredentialsPopup ()
{
    PopupMenu::dismissAllActiveMenus ();
    if (credentialsPopup)
        credentialsPopup->setLookAndFeel (nullptr);
    credentialsPopup.reset ();
}
