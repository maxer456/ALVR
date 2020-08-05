//===================== Copyright (c) Valve Corporation. All Rights Reserved. ======================
//
// Example OpenVR driver for demonstrating IVRVirtualDisplay interface.
//
//==================================================================================================

#include "bindings.h"

#if _WIN32

#include <windows.h>
#include "openvr_driver.h"
#include "sharedstate.h"
#include "OvrHMD.h"
#include "driverlog.h"

HINSTANCE g_hInstance;


static void load_debug_privilege(void)
{
	const DWORD flags = TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY;
	TOKEN_PRIVILEGES tp;
	HANDLE token;
	LUID val;

	if (!OpenProcessToken(GetCurrentProcess(), flags, &token)) {
		return;
	}

	if (!!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &val)) {
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = val;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		AdjustTokenPrivileges(token, false, &tp, sizeof(tp), NULL,
			NULL);
	}

	if (!!LookupPrivilegeValue(NULL, SE_INC_BASE_PRIORITY_NAME, &val)) {
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = val;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		if (!AdjustTokenPrivileges(token, false, &tp, sizeof(tp), NULL, NULL)) {
			LogDriver("[GPU PRIO FIX] Could not set privilege to increase GPU priority");
		}
	}

	LogDriver("[GPU PRIO FIX] Succeeded to set some sort of priority.");

	CloseHandle(token);
}

//-----------------------------------------------------------------------------
// Purpose: Server interface implementation.
//-----------------------------------------------------------------------------
class CServerDriver_DisplayRedirect : public vr::IServerTrackedDeviceProvider
{
public:
	CServerDriver_DisplayRedirect()
		: m_pRemoteHmd( NULL )
	{}

	virtual vr::EVRInitError Init( vr::IVRDriverContext *pContext ) override;
	virtual void Cleanup() override;
	virtual const char * const *GetInterfaceVersions() override
		{ return vr::k_InterfaceVersions;  }
	virtual const char *GetTrackedDeviceDriverVersion()
		{ return vr::ITrackedDeviceServerDriver_Version; }
	virtual void RunFrame();
	virtual bool ShouldBlockStandbyMode() override { return false; }
	virtual void EnterStandby() override {}
	virtual void LeaveStandby() override {}

private:
	std::shared_ptr<OvrHmd> m_pRemoteHmd;
	std::shared_ptr<IPCMutex> m_mutex; 
};

vr::EVRInitError CServerDriver_DisplayRedirect::Init( vr::IVRDriverContext *pContext )
{
	VR_INIT_SERVER_DRIVER_CONTEXT( pContext );
	InitDriverLog(vr::VRDriverLog());
	

	m_mutex = std::make_shared<IPCMutex>(APP_MUTEX_NAME, true);
	if (m_mutex->AlreadyExist()) {
		// Duplicate driver installation.
		FatalLog("ALVR Server driver is installed on multiple locations. This causes some issues.\r\n"
			"Please check the installed driver list on About tab and uninstall old drivers.");
		return vr::VRInitError_Driver_Failed;
	}

	//create listener
	// m_Listener = std::make_shared<ClientConnection>();

	//init listener
	// if (!m_Listener->Startup())
	// {
	// 	return vr::VRInitError_Driver_Failed;
	// }

	//create new virtuall hmd
	// m_pRemoteHmd = std::make_shared<OvrHmd>(m_Listener);

	// Launcher is running. Enable driver.
	// m_pRemoteHmd->Enable();

	return vr::VRInitError_None;
}

void CServerDriver_DisplayRedirect::Cleanup()
{
	// m_Listener.reset();
	m_pRemoteHmd.reset();
	m_mutex.reset();

	CleanupDriverLog();

	VR_CLEANUP_SERVER_DRIVER_CONTEXT();
}

void CServerDriver_DisplayRedirect::RunFrame()
{
}

CServerDriver_DisplayRedirect g_serverDriverDisplayRedirect;


BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		g_hInstance = hInstance;
	}

	return TRUE;
}

#endif

// bindigs for Rust

const uint8_t *FRAME_RENDER_VS_CSO_PTR;
uint32_t FRAME_RENDER_VS_CSO_LEN;
const uint8_t *FRAME_RENDER_PS_CSO_PTR;
uint32_t FRAME_RENDER_PS_CSO_LEN;
const uint8_t *QUAD_SHADER_CSO_PTR;
uint32_t QUAD_SHADER_CSO_LEN;
const uint8_t *COMPRESS_SLICES_CSO_PTR;
uint32_t COMPRESS_SLICES_CSO_LEN;
const uint8_t *COLOR_CORRECTION_CSO_PTR;
uint32_t COLOR_CORRECTION_CSO_LEN;

const char *g_alvrDir;

void (*LogError)(const char *stringPtr);
void (*LogWarn)(const char *stringPtr);
void (*LogInfo)(const char *stringPtr);
void (*LogDebug)(const char *stringPtr);
void (*MaybeLaunchWebServer)();
void (*MaybeKillWebServer)();

void *CppEntryPoint(const char *pInterfaceName, int *pReturnCode)
{
#if _WIN32
	Settings::Instance().Load();

	load_debug_privilege();

	LogDriver("HmdDriverFactory %hs (%hs)", pInterfaceName, vr::IServerTrackedDeviceProvider_Version);
	if (0 == strcmp(vr::IServerTrackedDeviceProvider_Version, pInterfaceName))
	{
		LogDriver("HmdDriverFactory server return");
		return &g_serverDriverDisplayRedirect;
	}

	if (pReturnCode)
		*pReturnCode = vr::VRInitError_Init_InterfaceNotFound;
#endif

	return nullptr;
}