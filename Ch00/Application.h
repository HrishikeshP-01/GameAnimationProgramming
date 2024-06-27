#pragma once
/**
* ifndef is a #include guards - prevents multiple inclusion of the same header file
* It checks if a unique application (in this case _H_APPLICATION_) is defined
* If it's not defined, it defines it & continues with the rest of the page
* If it's defined the ifdef fails resulting in a blank file
* This is useful when we include this file multiple times (mainly while using inheritance)
* The ifndef for the 1st file defines it but the next time it encounters the ifndef for this file, it won't define it
* This prevents the double declaration of any identifiers like types, enums & static variables
*/
#ifndef _H_APPLICATION_
#define _H_APPLICATION_

class Application
{
private:
	Application(const Application&);
	Application& operator=(const Application&);
public:
	inline Application() {}
	inline virtual ~Application() {}
	inline virtual void Initialize() {}
	inline virtual void Update(float inDeltaTime) {}
	inline virtual void Render(float inAspectRation) {}
	inline virtual void Shutdown() {}
};

#endif