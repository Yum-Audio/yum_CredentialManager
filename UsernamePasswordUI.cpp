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
		updateCredentialsPopup(&usernameEditor);
        
    };
    auto credPopupFromPassword = [&]()
    {
		updateCredentialsPopup(&passwordEditor);
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
    reEnableEditors ();
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

void UsernamePasswordUI::reEnableEditors ()
{
    usernameEditor.setEnabled (true);
    passwordEditor.setEnabled (true);
    loginButton.setEnabled (true);
}

void UsernamePasswordUI::attemptLogin ()
{
    usernameEditor.setEnabled (false);
    passwordEditor.setEnabled (false);
    loginButton.setEnabled (false);
    
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
    
	tasks.add (new ThreadedTask ([&, uname = usernameEditor.getText (), source](ThreadedTask* task)
	{
		const auto entries = AppCredentials::getAllAvailableEntries (uname);

		MessageManager::callAsync ([&, entries, task, source] ()
		{
			if ( ! entries.isEmpty () )
			{
				PopupMenu m;
				
				m.setLookAndFeel (popupLaf.get ());
				
				for (auto& e : entries)
					m.addItem (e, [&, e] () { fillEditorsForKeychainUser (e); });
				
				const auto options = PopupMenu::Options ()
									.withTargetComponent (source)
									.withPreferredPopupDirection (PopupMenu::Options::PopupDirection::downwards)
									.withMinimumWidth (source->getWidth());
				m.showMenuAsync (options);
			}

			tasks.removeObject (task);
		});
	}));
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

}
