//
// Copyright (c) 2004 - InfoMus Lab - DIST - University of Genova
//
// InfoMus Lab (Laboratorio di Informatica Musicale)
// DIST - University of Genova 
//
// http://www.infomus.dist.unige.it
// news://infomus.dist.unige.it
// mailto:staff@infomus.dist.unige.it
//
// Developer: Gualtiero Volpe
// mailto:volpe@infomus.dist.unige.it
//
// Developer: Trey Harrison
// mailto:trey@treyharrison.com
//
// Fixed issues with Visual Studio 2013: Seppo Enarvi
// http://users.marjaniemi.com/seppo/
//
// Last modified: 2014-07-16
//

#include <sstream>

#include "FFGLPluginSDK.h"

// Disable Microsoft Visual Studio warning:
// 'std::basic_string<char,std::char_traits<char>,std::allocator<char>>::copy':
// Function call with parameters that may be unsafe
// - this call relies on the caller to check that the passed values are correct.
#pragma warning(disable:4996)

using namespace std;


// Buffer used by the default implementation of getParameterDisplay
static char s_DisplayValue[5];


////////////////////////////////////////////////////////
// CFreeFrameGLPlugin constructor and destructor
////////////////////////////////////////////////////////

CFreeFrameGLPlugin::CFreeFrameGLPlugin()
: CFFGLPluginManager()
{
}

CFreeFrameGLPlugin::~CFreeFrameGLPlugin()
{
}


////////////////////////////////////////////////////////
// Default implementation of CFreeFrameGLPlugin methods
////////////////////////////////////////////////////////

// Converts a parameter value to a char array for displaying
// on the user interface of the host application.
//
// Fixed buffer overflow by copying only at most 5 characters
// to s_DisplayValue buffer, plus cosmetic changes.
//   -- Seppo Enarvi, 2014-07-16
//
char* CFreeFrameGLPlugin::GetParameterDisplay(DWORD dwIndex)
{
	DWORD dwType = m_pPlugin->GetParamType(dwIndex);
	DWORD dwValue = m_pPlugin->GetParameter(dwIndex);

	if ((dwValue != FF_FAIL) && (dwType != FF_FAIL))
	{
		if (dwType == FF_TYPE_TEXT)
		{
			return reinterpret_cast<char*>(dwValue);
		}
		else
		{
			ostringstream oss;
			oss << *reinterpret_cast<float*>(&dwValue);
			string::size_type num_copied = oss.str().copy(s_DisplayValue, 5);
			if (num_copied < 5)
				s_DisplayValue[4] = '\0';
			return s_DisplayValue;
		}
	}
	return NULL;
}

DWORD CFreeFrameGLPlugin::SetParameter(const SetParameterStruct* pParam)
{
	return FF_FAIL;
}

DWORD CFreeFrameGLPlugin::GetParameter(DWORD dwIndex)
{
	return FF_FAIL;
}

DWORD CFreeFrameGLPlugin::GetInputStatus(DWORD dwIndex)
{
	if (dwIndex >= (DWORD)GetMaxInputs()) return FF_FAIL;
	return FF_INPUT_INUSE;
}
