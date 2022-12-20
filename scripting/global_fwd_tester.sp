#include <sdktools>
#include <libmodulemanager>

#pragma semicolon         1
#pragma newdecls          required

#define PLUGIN_VERSION    "1.1.0"


public Plugin myinfo = {
	name        = "LibModuleManagerTester-GlobalFwd",
	author      = "Nergal",
	description = "Plugin that tests LibModuleManager.",
	version     = PLUGIN_VERSION,
	url         = "zzzzzzzzzzzzz"
};


public void OnLibraryAdded(const char[] name) {
	if( StrEqual(name, "LibModuleManager") ) {
		PrintToServer("Library 'LibModuleManager' is loaded!!");
		ManagerID test_id = LibModuleManager_MakePrivateFwdsManager("configs/plugin_manager/private_fwds.cfg");
		LibModuleManager_PrivateFwdHook(test_id, "OnPrivFwdExampleName", OnPrivateFwdTest);
		
		ManagerID mm_id = LibModuleManager_MakeModuleManager("configs/plugin_manager/module_manager.cfg");
		ModuleManager mm;
		LibModuleManager_GetModuleManager(mm_id, mm);
		mm.Print();
		
		Function f = LibModuleManager_GetModuleFunc(mm_id, "global_fwd_tester", "OnGlobalFwdExampleName");
		PrintToServer("f == OnGlobalFwdExampleName: '%i'", f == OnGlobalFwdExampleName);
		
		Handle pl = LibModuleManager_GetModuleHandle(mm_id, "global_fwd_tester");
		Handle me = GetMyHandle();
		PrintToServer("pl == me: '%i'", pl==me);
		
		PawnAwait(AwaitChannel, 0.25, {0}, 0);
	}
}

static SharedMap g_shmap;
public bool AwaitChannel() {
	if( !LibModuleManager_ChannelExists("global_fwd_tester") ) {
		return false;
	}
	g_shmap = SharedMap("global_fwd_tester");
	return true;
}

public Action OnGlobalFwdExampleName(int p1, any p2, const char[] p3, int &p4, float &p5) {
	PrintToServer("Running OnGlobalFwdExampleName");
	PrintToServer("OnGlobalFwdExampleName :: p1: '%i'", p1);
	PrintToServer("OnGlobalFwdExampleName :: p2: '%i'", p2);
	PrintToServer("OnGlobalFwdExampleName :: p3: '%s'", p3);
	PrintToServer("OnGlobalFwdExampleName :: p4: before modifying '%i'", p4);
	p4 = 9;
	PrintToServer("OnGlobalFwdExampleName :: p4: after modifying '%i'", p4);
	PrintToServer("OnGlobalFwdExampleName :: p5: before modifying '%f'", p5);
	p5 = 6.5;
	PrintToServer("OnGlobalFwdExampleName :: p5: after modifying '%f'", p5);
	return Plugin_Changed;
}

public Action OnPrivateFwdTest(int p1, any p2, const char[] p3, int &p4, float &p5) {
	PrintToServer("Running OnPrivateFwdTest");
	PrintToServer("OnPrivateFwdTest :: p1: '%i'", p1);
	PrintToServer("OnPrivateFwdTest :: p2: '%i'", p2);
	PrintToServer("OnPrivateFwdTest :: p3: '%s'", p3);
	PrintToServer("OnPrivateFwdTest :: p4: before modifying '%i'", p4);
	p4 = 9;
	PrintToServer("OnPrivateFwdTest :: p4: after modifying '%i'", p4);
	PrintToServer("OnPrivateFwdTest :: p5: before modifying '%f'", p5);
	p5 = 6.8;
	PrintToServer("OnPrivateFwdTest :: p5: after modifying '%f'", p5);
	return Plugin_Changed;
}
