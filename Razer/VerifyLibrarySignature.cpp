#include "stdafx.h"
#include "VerifyLibrarySignature.h"
#include <psapi.h>
#include <wintrust.h>
#include <Softpub.h>

#pragma comment (lib, "wintrust")
#pragma comment (lib, "Psapi")

namespace ChromaSDK
{

	// Source: https://docs.microsoft.com/en-us/windows/desktop/seccrypto/example-c-program--verifying-the-signature-of-a-pe-file
	BOOL VerifyLibrarySignature(const TCHAR* szModuleName, HMODULE hModule)
	{
		DWORD dwLastError;

		TCHAR szFilePath[MAX_PATH];
		if (GetModuleFileNameEx(GetCurrentProcess(),
			hModule,
			szFilePath,
			MAX_PATH) > 0)
		{
			WINTRUST_FILE_INFO FileData;
			memset(&FileData, 0, sizeof(FileData));

			FileData.cbStruct = sizeof(WINTRUST_FILE_INFO);
			FileData.pcwszFilePath = szFilePath;
			FileData.hFile = NULL;
			FileData.pgKnownSubject = NULL;

			/*
			WVTPolicyGUID specifies the policy to apply on the file
			WINTRUST_ACTION_GENERIC_VERIFY_V2 policy checks:

			1) The certificate used to sign the file chains up to a root
			certificate located in the trusted root certificate store. This
			implies that the identity of the publisher has been verified by
			a certification authority.

			2) In cases where user interface is displayed (which this example
			does not do), WinVerifyTrust will check for whether the
			end entity certificate is stored in the trusted publisher store,
			implying that the user trusts content from this publisher.

			3) The end entity certificate has sufficient permission to sign
			code, as indicated by the presence of a code signing EKU or no
			EKU.
			*/

			GUID WVTPolicyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;

			WINTRUST_DATA WinTrustData;
			memset(&WinTrustData, 0, sizeof(WinTrustData));

			WinTrustData.cbStruct = sizeof(WinTrustData);
			WinTrustData.pPolicyCallbackData = NULL;
			WinTrustData.pSIPClientData = NULL;
			WinTrustData.dwUIChoice = WTD_UI_NONE;
			WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
			WinTrustData.dwUnionChoice = WTD_CHOICE_FILE;
			WinTrustData.dwStateAction = WTD_STATEACTION_VERIFY;
			WinTrustData.hWVTStateData = NULL;
			WinTrustData.pwszURLReference = NULL;
			WinTrustData.dwUIContext = 0;
			WinTrustData.pFile = &FileData;

			// WinVerifyTrust verifies signatures as specified by the GUID 
			// and Wintrust_Data.
			LONG lStatus = WinVerifyTrust(
				NULL,
				&WVTPolicyGUID,
				&WinTrustData);

			switch (lStatus)
			{
			case ERROR_SUCCESS:
				/*
				Signed file:
					- Hash that represents the subject is trusted.

					- Trusted publisher without any verification errors.

					- UI was disabled in dwUIChoice. No publisher or
						time stamp chain errors.

					- UI was enabled in dwUIChoice and the user clicked
						"Yes" when asked to install and run the signed
						subject.
				*/
				break;
			case TRUST_E_NOSIGNATURE:
				// The file was not signed or had a signature 
				// that was not valid.

				// Get the reason for no signature.
				dwLastError = GetLastError();
				if (TRUST_E_NOSIGNATURE == dwLastError ||
					TRUST_E_SUBJECT_FORM_UNKNOWN == dwLastError ||
					TRUST_E_PROVIDER_UNKNOWN == dwLastError)
				{
					// The file was not signed.
				}
				else
				{
					// The signature was not valid or there was an error 
					// opening the file.
				}

				return FALSE;

			case TRUST_E_EXPLICIT_DISTRUST:
				// The hash that represents the subject or the publisher 
				// is not allowed by the admin or user.
				return FALSE;

			case TRUST_E_SUBJECT_NOT_TRUSTED:
				// The user clicked "No" when asked to install and run.
				return FALSE;

			case CRYPT_E_SECURITY_SETTINGS:
				/*
				The hash that represents the subject or the publisher
				was not explicitly trusted by the admin and the
				admin policy has disabled user trust. No signature,
				publisher or time stamp errors.
				*/
				return FALSE;

			default:
				// The UI was disabled in dwUIChoice or the admin policy 
				// has disabled user trust. lStatus contains the 
				// publisher or time stamp chain error.
				dwLastError = GetLastError();
				return FALSE;
			}

			// Any hWVTStateData must be released by a call with close.
			WinTrustData.dwStateAction = WTD_STATEACTION_CLOSE;

			lStatus = WinVerifyTrust(
				NULL,
				&WVTPolicyGUID,
				&WinTrustData);

			// Verify the path of the module
			// Below are valid paths
			// Windows/System32
			// Windows/SysWOW64
			// Program Files/Razer Chroma SDK/bin
			// Program Files (x86)/Razer Chroma SDK/bin

			DWORD dwLength = 0;
			TCHAR szFileName[MAX_PATH];

			TCHAR szSystemRootPath[MAX_PATH] = L"";
			dwLength = GetEnvironmentVariable(L"SystemRoot",
				szSystemRootPath,
				MAX_PATH);

			if (dwLength > 0)
			{
				_tcscpy_s(szFileName, dwLength + 1, szSystemRootPath);

				_tcscat_s(szFileName, MAX_PATH, L"\\System32\\");
				_tcscat_s(szFileName, MAX_PATH, szModuleName);

				if (_tcsicmp(szFilePath, szFileName) == 0)
				{
					return TRUE;
				}
			}

			TCHAR szProgramFilesPath[MAX_PATH] = L"";
			dwLength = GetEnvironmentVariable(L"ProgramFiles",
				szProgramFilesPath,
				MAX_PATH);

			if (dwLength > 0)
			{
				_tcscpy_s(szFileName, dwLength + 1, szProgramFilesPath);

				_tcscat_s(szFileName, MAX_PATH, L"\\Razer Chroma SDK\\bin\\");
				_tcscat_s(szFileName, MAX_PATH, szModuleName);

				if (_tcsicmp(szFilePath, szFileName) == 0)
				{
					return TRUE;
				}
			}
		}

		return FALSE;
	}

}