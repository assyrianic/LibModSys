#include <sdktools>
#include <libmodsys>

#pragma semicolon         1
#pragma newdecls          required

#define PLUGIN_VERSION    "1.0.0"


public Plugin myinfo = {
	name        = "LibModSysTester",
	author      = "Nergal",
	description = "Plugin that tests LibModSys.",
	version     = PLUGIN_VERSION,
	url         = "zzzzzzzzzzzzz"
};


//public void OnPluginStart() {
public void OnMapStart() {
	PrintToServer("==================== LibModSysTester Is Online");
	GlobalFwd fwd_example;
	LibModSys_GetGlobalFwd("OnGlobalFwdExampleName", fwd_example);
	PrintToServer("LibModSysTester GlobalFwd :: exec_type: '%i' - param_count: '%i'", fwd_example.exec_type, fwd_example.param_count);
	
	fwd_example.Start();
	fwd_example.PushCell(100);
	fwd_example.PushCell(200);
	char s2[] = "hello from global fwd";
	fwd_example.PushString(s2, sizeof(s2));
	int ref1; fwd_example.PushCellRef(ref1);
	int ref2; fwd_example.PushFloatRef(ref2);
	Action result; fwd_example.Finish(result);
	PrintToServer("ref1: %i | ref2: %i | Action: %i", ref1, ref2, result);
	
	
	PrivateFwd pf;
	if( !LibModSys_GetPrivateFwd(IntToAny(1), "OnPrivFwdExampleName", pf) ) {
		PrintToServer("LibModSysTester PrivateFwd :: Error **** Couldn't get private fwd for 'OnPrivFwdExampleName' ****");
		return;
	}
	PrintToServer("LibModSysTester PrivateFwd :: exec_type: '%i' - param_count: '%i' - function count: '%i'", pf.exec_type, pf.param_count, pf.Count());
	
	pf.Start();
	pf.PushCell(100);
	pf.PushCell(200);
	char s3[] = "hello from priv fwd";
	pf.PushString(s3, sizeof(s3));
	ref1 = 0; pf.PushCellRef(ref1);
	ref2 = 0; pf.PushFloatRef(ref2);
	result = Plugin_Continue; pf.Finish(result);
	PrintToServer("ref1: %i | ref2: %i | Action: %i", ref1, ref2, result);
}
