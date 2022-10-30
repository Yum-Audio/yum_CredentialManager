#pragma once
#include <JuceHeader.h>
#include "Types.h"

/// AppCredentials subclass by class-inline header include

struct UnamePWEditor : public TextEditor
{
    void mouseDown (const MouseEvent& e) override
    {
        jassert (onMouseDown != nullptr);
        if (onMouseDown) onMouseDown ();
        TextEditor::mouseDown (e);
    }
    std::function<void ()> onMouseDown;
};

class UsernamePasswordUI : public juce::Component
{
public:
    UsernamePasswordUI ();
    ~UsernamePasswordUI () override;
    /// if a function is defined, then virtual function loginButtonClicked won't get a call
    std::function<void (const UsernameAndPassword&)> onLoginButtonClicked { nullptr };
    
    /// returns the current contents of the text editors.
    UsernameAndPassword getCurrentEditorCredentials ();
    
    /// after you're finished with the password editor you should reset it for good measure
    void resetPasswordEditor ();
    
    /// pass a raw pointer in here, the UsernamePasswordUI class takes ownership and deletes
    void setPopupLookAndFeel (LookAndFeel_V4* laf);
    
    /// call this after a login attempt has finished, to make the UI elements accessible again
    void reEnableEditors ();
    
    /// publically accessible members for styling, debugging, etc...
    TextButton loginButton { "Login" };
    UnamePWEditor usernameEditor;
    UnamePWEditor passwordEditor;
    
protected:
    /// will only be called if onLoginButtonClicked == nullptr
    virtual void loginButtonClicked (const UsernameAndPassword& credentials);
    
    void resized () override;
    
private:
    juce::Array<juce::Component*> elements
    {
        &usernameEditor,
        &passwordEditor,
        &loginButton
    };
    
    std::unique_ptr<LookAndFeel_V4> popupLaf;
    
    void attemptLogin ();
    void updateCredentialsPopup (Component* source);
    void fillEditorsForKeychainUser (const Username& user);
    void closeCredentialsPopup ();
    
	class ThreadedTask : private Thread
	{
	public:
		ThreadedTask (std::function<void (ThreadedTask*)> task) : Thread ("ThreadedTask"), task (task) { startThread (); }
	
	private:
		std::function<void (ThreadedTask*)> task;
		void run () override
		{
			task (this);
		}
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThreadedTask)

	};
	OwnedArray<ThreadedTask, CriticalSection> tasks;
	
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UsernamePasswordUI)
};

