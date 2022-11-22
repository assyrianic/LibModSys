#include <sdktools>
#include <libmodulemanager>

#pragma semicolon         1
#pragma newdecls          required

#define PLUGIN_VERSION    "1.0.0a"


public Plugin myinfo = {
	name        = "LibModuleManager",
	author      = "Nergal",
	description = "Plugin that manages systems of subplugins.",
	version     = PLUGIN_VERSION,
	url         = "https://github.com/assyrianic/LibModuleManager"
};


enum struct ModuleManagerCvars {
	ConVar Enabled;
	//ConVar MainGlobalFwdsCfg;
}


enum struct ModuleManagerPlugin {
	ModuleManagerCvars cvars;         /// 
	
	GlobalFwdsManager  glfwd_manager; /// 
	
	ManagerID          manager_ids;   /// manager IDs to track specific plugin managers.
	StringMap          pf_managers;   /// map[ManagerID]PrivateFwdsManager
	StringMap          pl_managers;   /// map[ManagerID]ModuleManager
	
	
	void Init() {
		this.glfwd_manager.Init("configs/plugin_manager/global_fwds.cfg");
		this.pf_managers = new StringMap();
		this.pl_managers = new StringMap();
		this.manager_ids = IntToManagerID(1);
	}
	
	ManagerID GenerateManagerID() {
		if( this.manager_ids==InvalidManagerID ) {
			return InvalidManagerID;
		}
		ManagerID id = this.manager_ids++;
		return id;
	}
	
	void Destroy() {
		this.glfwd_manager.Destroy();
		
		/// destroy Private Forward Managers.
		StringMapSnapshot snap = this.pf_managers.Snapshot();
		if( snap != null ) {
			int len = snap.Length;
			for( int i; i < len; i++ ) {
				int keysize = snap.KeyBufferSize(i) + 1;
				char[] name = new char[keysize];
				snap.GetKey(i, name, keysize);
				
				PrivateFwdsManager pfm;
				if( !this.pf_managers.GetArray(name, pfm, sizeof(pfm)) ) {
					continue;
				}
				
				pfm.Destroy();
			}
			delete snap;
		}
		delete this.pf_managers;
		
		/// destroy Module Managers.
		snap = this.pl_managers.Snapshot();
		if( snap != null ) {
			int len = snap.Length;
			for( int i; i < len; i++ ) {
				int keysize = snap.KeyBufferSize(i) + 1;
				char[] name = new char[keysize];
				snap.GetKey(i, name, keysize);
				
				ModuleManager mm;
				if( !this.pl_managers.GetArray(name, mm, sizeof(mm)) ) {
					continue;
				}
				
				mm.Destroy();
			}
			delete snap;
		}
		delete this.pl_managers;
	}
}

ModuleManagerPlugin g_mmp;

/*
public void OnPluginStart() {
	//g_mmp.cvars.Enabled = CreateConVar("lib_module_manager_" ... "enabled", "1", "Enable module manager plugin", FCVAR_NONE, true, 0.0, true, 1.0);
	
	//RegConsoleCmd("sm_" ... "zzzzzz",         Cmd_ZZZZ);
	//RegAdminCmd("sm_" ... "force",    Cmd_Force, ADMFLAG_GENERIC, "description");
}
*/

/*
public Action Cmd_ZZZZ(int client, int args) {
	return Plugin_Continue;
}
*/

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max) {
	g_mmp.Init();
	
	/// Global Forwards Manager natives.
	CreateNative("LibModuleManager_GetGlobalFwdsManager",   Native_LibModuleManager_GetGlobalFwdManager);
	
	/// Private Forwards Manager natives.
	CreateNative("LibModuleManager_MakePrivateFwdsManager", Native_LibModuleManager_MakePrivateFwdManager);
	CreateNative("LibModuleManager_GetPrivateFwdsManager",  Native_LibModuleManager_GetPrivateFwdManager);
	CreateNative("LibModuleManager_PrivateFwdHook",         Native_LibModuleManager_PrivateFwdHook);
	CreateNative("LibModuleManager_PrivateFwdUnhook",       Native_LibModuleManager_PrivateFwdUnhook);
	CreateNative("LibModuleManager_PrivateFwdUnhookAll",    Native_LibModuleManager_PrivateFwdUnhookAll);
	//CreateNative("LibModuleManager_PrivateFwdUnhookAll",    Native_LibModuleManager_ExecForward);
	
	/// Plugin/Module Manager natives.
	CreateNative("LibModuleManager_MakeModuleManager",      Native_LibModuleManager_MakeModuleManager);
	CreateNative("LibModuleManager_GetModuleManager",       Native_LibModuleManager_GetModuleManager);
	CreateNative("LibModuleManager_RegisterModule",         Native_LibModuleManager_RegisterModule);
	CreateNative("LibModuleManager_UnregisterModule",       Native_LibModuleManager_UnregisterModule);
	
	RegPluginLibrary("LibModuleManager");
	return APLRes_Success;
}


/// bool LibModuleManager_GetGlobalFwdsManager(GlobalFwdsManager glfwd_manager);
public any Native_LibModuleManager_GetGlobalFwdManager(Handle plugin, int numParams) {
	return SetNativeArray(1, g_mmp.glfwd_manager, sizeof(g_mmp.glfwd_manager))==SP_ERROR_NONE;
}

/// ManagerID LibModuleManager_MakePrivateFwdsManager(const char[] cfgfile);
public any Native_LibModuleManager_MakePrivateFwdManager(Handle plugin, int numParams) {
	ManagerID pfm_id = g_mmp.GenerateManagerID();
	if( pfm_id==InvalidManagerID ) {
		LogError("LibModuleManager_MakePrivateFwdsManager :: Error :: **** out of IDs! ****");
		return InvalidManagerID;
	}
	
	/// get length of string and then string itself
	int cfg_filepath_len; GetNativeStringLength(1, cfg_filepath_len);
	cfg_filepath_len++;
	char[] cfg_filepath = new char[cfg_filepath_len];
	GetNativeString(1, cfg_filepath, cfg_filepath_len);
	
	/// setup private forward manager.
	PrivateFwdsManager pfm; pfm.Init(cfg_filepath);
	
	/// pack id as a string and store into map.
	char id_key[CELL_KEY_SIZE]; PackCellToStr(pfm_id, id_key);
	//PrintToServer("LibModuleManager_MakePrivateFwdsManager :: pfm_id == '%i' | id_key == '%i' '%i' '%i' '%i' '%i' '%i'", pfm_id, id_key[0] & 0xFF, id_key[1] & 0xFF, id_key[2] & 0xFF, id_key[3] & 0xFF, id_key[4] & 0xFF, id_key[5] & 0xFF);
	g_mmp.pf_managers.SetArray(id_key, pfm, sizeof(pfm));
	return pfm_id;
}

/// bool LibModuleManager_GetPrivateFwdsManager(ManagerID id, PrivateFwdsManager buf);
public any Native_LibModuleManager_GetPrivateFwdManager(Handle plugin, int numParams) {
	ManagerID pfm_id = GetNativeCell(1);
	if( pfm_id==InvalidManagerID ) {
		LogMessage("LibModuleManager_GetPrivateFwdsManager :: Error :: **** invalid Manager ID! ****");
		return false;
	} else if( g_mmp.pf_managers.Size <= 0 ) {
		LogMessage("LibModuleManager_GetPrivateFwdsManager :: Error :: **** there are no Private Forward Managers. ****");
		return false;
	}
	
	char id_key[CELL_KEY_SIZE]; PackCellToStr(pfm_id, id_key);
	//PrintToServer("LibModuleManager_GetPrivateFwdsManager :: pfm_id == '%i' | id_key == '%i' '%i' '%i' '%i' '%i' '%i'", pfm_id, id_key[0] & 0xFF, id_key[1] & 0xFF, id_key[2] & 0xFF, id_key[3] & 0xFF, id_key[4] & 0xFF, id_key[5] & 0xFF);
	PrivateFwdsManager pfm;
	if( g_mmp.pf_managers.GetArray(id_key, pfm, sizeof(pfm)) ) {
		SetNativeArray(2, pfm, sizeof(pfm));
		return true;
	}
	LogMessage("LibModuleManager_GetPrivateFwdsManager :: Warning :: **** unable to get manager with id '%i' ****", pfm_id);
	return false;
}

/// bool LibModuleManager_PrivateFwdHook(ManagerID id, const char[] fwd_name, Function f);
public any Native_LibModuleManager_PrivateFwdHook(Handle plugin, int numParams) {
	ManagerID pfm_id = GetNativeCell(1);
	if( pfm_id==InvalidManagerID ) {
		LogMessage("LibModuleManager_PrivateFwdHook :: Error :: **** invalid Manager ID! ****");
		return false;
	} else if( g_mmp.pf_managers.Size <= 0 ) {
		LogMessage("LibModuleManager_PrivateFwdHook :: Error :: **** there are no Private Forward Managers. ****");
		return false;
	}
	
	char id_key[CELL_KEY_SIZE]; PackCellToStr(pfm_id, id_key);
	PrivateFwdsManager pfm;
	if( !g_mmp.pf_managers.GetArray(id_key, pfm, sizeof(pfm)) ) {
		LogMessage("LibModuleManager_PrivateFwdHook :: Warning :: **** unable to get forward manager with id '%i' ****", pfm_id);
		return false;
	}
	
	int fwd_name_len; GetNativeStringLength(2, fwd_name_len);
	fwd_name_len++;
	char[] fwd_name = new char[fwd_name_len];
	GetNativeString(2, fwd_name, fwd_name_len);
	
	PrivateFwd pf;
	if( !pfm.GetFwd(fwd_name, pf) ) {
		LogMessage("LibModuleManager_PrivateFwdHook :: Warning :: **** unable to get forward '%s' ****", fwd_name);
		return false;
	}
	
	Function f = GetNativeFunction(3);
	return pf.Hook(plugin, f);
}

/// bool LibModuleManager_PrivateFwdUnhook(ManagerID id, const char[] fwd_name, Function f);
public any Native_LibModuleManager_PrivateFwdUnhook(Handle plugin, int numParams) {
	ManagerID pfm_id = GetNativeCell(1);
	if( pfm_id==InvalidManagerID ) {
		LogMessage("LibModuleManager_PrivateFwdUnhook :: Error :: **** invalid Manager ID! ****");
		return false;
	} else if( g_mmp.pf_managers.Size <= 0 ) {
		LogMessage("LibModuleManager_PrivateFwdUnhook :: Error :: **** there are no Private Forward Managers. ****");
		return false;
	}
	
	char id_key[CELL_KEY_SIZE]; PackCellToStr(pfm_id, id_key);
	PrivateFwdsManager pfm;
	if( !g_mmp.pf_managers.GetArray(id_key, pfm, sizeof(pfm)) ) {
		LogMessage("LibModuleManager_PrivateFwdUnhook :: Warning :: **** unable to get forward manager with id '%i' ****", pfm_id);
		return false;
	}
	
	int fwd_name_len; GetNativeStringLength(2, fwd_name_len);
	fwd_name_len++;
	char[] fwd_name = new char[fwd_name_len];
	GetNativeString(2, fwd_name, fwd_name_len);
	
	PrivateFwd pf;
	if( !pfm.GetFwd(fwd_name, pf) ) {
		LogMessage("LibModuleManager_PrivateFwdUnhook :: Warning :: **** unable to get forward '%s' ****", fwd_name);
		return false;
	}
	
	Function f = GetNativeFunction(3);
	return pf.Unhook(plugin, f);
}

/// bool LibModuleManager_PrivateFwdUnhookAll(ManagerID id, const char[] fwd_name);
public any Native_LibModuleManager_PrivateFwdUnhookAll(Handle plugin, int numParams) {
	ManagerID pfm_id = GetNativeCell(1);
	if( pfm_id==InvalidManagerID ) {
		LogMessage("LibModuleManager_PrivateFwdUnhookAll :: Error :: **** invalid Manager ID! ****");
		return false;
	} else if( g_mmp.pf_managers.Size <= 0 ) {
		LogMessage("LibModuleManager_PrivateFwdUnhookAll :: Error :: **** there are no Private Forward Managers. ****");
		return false;
	}
	
	char id_key[CELL_KEY_SIZE]; PackCellToStr(pfm_id, id_key);
	PrivateFwdsManager pfm;
	if( !g_mmp.pf_managers.GetArray(id_key, pfm, sizeof(pfm)) ) {
		LogMessage("LibModuleManager_PrivateFwdUnhookAll :: Warning :: **** unable to get forward manager with id '%i' ****", pfm_id);
		return false;
	}
	
	int fwd_name_len; GetNativeStringLength(2, fwd_name_len);
	fwd_name_len++;
	char[] fwd_name = new char[fwd_name_len];
	GetNativeString(2, fwd_name, fwd_name_len);
	
	PrivateFwd pf;
	if( !pfm.GetFwd(fwd_name, pf) ) {
		LogMessage("LibModuleManager_PrivateFwdUnhookAll :: Warning :: **** unable to get forward '%s' ****", fwd_name);
		return false;
	}
	return pf.UnhookAll(plugin);
}

/// ManagerID LibModuleManager_MakeModuleManager(const char[] cfgfile);
public any Native_LibModuleManager_MakeModuleManager(Handle plugin, int numParams) {
	ManagerID mm_id = g_mmp.GenerateManagerID();
	if( mm_id==InvalidManagerID ) {
		LogError("LibModuleManager_MakeModuleManager :: Error :: **** out of IDs! ****");
		return InvalidManagerID;
	}
	
	/// get length of string and then string itself
	int cfg_filepath_len; GetNativeStringLength(1, cfg_filepath_len);
	cfg_filepath_len++;
	char[] cfg_filepath = new char[cfg_filepath_len];
	GetNativeString(1, cfg_filepath, cfg_filepath_len);
	
	/// setup private forward manager.
	ModuleManager mm; mm.Init(cfg_filepath);
	
	/// pack id as a string and store into map.
	char id_key[CELL_KEY_SIZE]; PackCellToStr(mm_id, id_key);
	//PrintToServer("LibModuleManager_MakeModuleManager :: mm_id == '%i' | id_key == '%i' '%i' '%i' '%i' '%i' '%i'", mm_id, id_key[0] & 0xFF, id_key[1] & 0xFF, id_key[2] & 0xFF, id_key[3] & 0xFF, id_key[4] & 0xFF, id_key[5] & 0xFF);
	g_mmp.pl_managers.SetArray(id_key, mm, sizeof(mm));
	return mm_id;
}

/// bool LibModuleManager_GetModuleManager(ManagerID id, /** ModuleManager */ any[] buf);
public any Native_LibModuleManager_GetModuleManager(Handle plugin, int numParams) {
	ManagerID mm_id = GetNativeCell(1);
	if( mm_id==InvalidManagerID ) {
		LogMessage("LibModuleManager_GetModuleManager :: Error :: **** invalid Manager ID! ****");
		return false;
	} else if( g_mmp.pl_managers.Size <= 0 ) {
		LogMessage("LibModuleManager_GetModuleManager :: Error :: **** there are no Private Forward Managers. ****");
		return false;
	}
	
	char id_key[CELL_KEY_SIZE]; PackCellToStr(mm_id, id_key);
	//PrintToServer("LibModuleManager_GetModuleManager :: mm_id == '%i' | id_key == '%i' '%i' '%i' '%i' '%i' '%i'", mm_id, id_key[0] & 0xFF, id_key[1] & 0xFF, id_key[2] & 0xFF, id_key[3] & 0xFF, id_key[4] & 0xFF, id_key[5] & 0xFF);
	ModuleManager mm;
	if( g_mmp.pl_managers.GetArray(id_key, mm, sizeof(mm)) ) {
		SetNativeArray(2, mm, sizeof(mm));
		return true;
	}
	LogMessage("LibModuleManager_GetModuleManager :: Warning :: **** unable to get manager with id '%i' ****", mm_id);
	return false;
}

/// bool LibModuleManager_RegisterModule(ManagerID id, const char[] name, int flags=0, int priority=0, int component=0, int group=0);
public any Native_LibModuleManager_RegisterModule(Handle plugin, int numParams) {
	ManagerID mm_id = GetNativeCell(1);
	if( mm_id==InvalidManagerID ) {
		LogMessage("LibModuleManager_RegisterModule :: Error :: **** invalid Manager ID! ****");
		return false;
	} else if( g_mmp.pl_managers.Size <= 0 ) {
		LogMessage("LibModuleManager_RegisterModule :: Error :: **** there are no Module Managers. ****");
		return false;
	}
	
	char id_key[CELL_KEY_SIZE]; PackCellToStr(mm_id, id_key);
	ModuleManager mm;
	if( !g_mmp.pl_managers.GetArray(id_key, mm, sizeof(mm)) ) {
		LogMessage("LibModuleManager_RegisterModule :: Warning :: **** unable to get module manager with id '%i' ****", mm_id);
		return false;
	}
	
	int pl_name_len; GetNativeStringLength(2, pl_name_len);
	pl_name_len++;
	char[] pl_name = new char[pl_name_len];
	GetNativeString(2, pl_name, pl_name_len);
	
	PluginModule pm;
	pm.plugin    = plugin;
	pm.flags     = GetNativeCell(3);
	pm.priority  = GetNativeCell(4);
	pm.component = GetNativeCell(5);
	pm.group     = GetNativeCell(6);
	return mm.modules.SetArray(pl_name, pm, sizeof(pm));
}

/// bool LibModuleManager_UnregisterModule(ManagerID id, const char[] name);
public any Native_LibModuleManager_UnregisterModule(Handle plugin, int numParams) {
	ManagerID mm_id = GetNativeCell(1);
	if( mm_id==InvalidManagerID ) {
		LogMessage("LibModuleManager_UnregisterModule :: Error :: **** invalid Manager ID! ****");
		return false;
	} else if( g_mmp.pl_managers.Size <= 0 ) {
		LogMessage("LibModuleManager_UnregisterModule :: Error :: **** there are no Module Managers. ****");
		return false;
	}
	
	char id_key[CELL_KEY_SIZE]; PackCellToStr(mm_id, id_key);
	ModuleManager mm;
	if( !g_mmp.pl_managers.GetArray(id_key, mm, sizeof(mm)) ) {
		LogMessage("LibModuleManager_UnregisterModule :: Warning :: **** unable to get module manager with id '%i' ****", mm_id);
		return false;
	}
	
	int pl_name_len; GetNativeStringLength(2, pl_name_len);
	pl_name_len++;
	char[] pl_name = new char[pl_name_len];
	GetNativeString(2, pl_name, pl_name_len);
	return mm.modules.Remove(pl_name);
}



/*
/// bool LibModuleManager_ExecForward(ManagerID id, const char[] name, any &result=0, ...);
public any Native_LibModuleManager_ExecForward(Handle plugin, int numParams) {
	ManagerID manager_id = GetNativeCell(1);
	
	int fwd_name_len; GetNativeStringLength(2, fwd_name_len);
	fwd_name_len++;
	char[] fwd_name = new char[fwd_name_len];
	GetNativeString(2, fwd_name, fwd_name_len);
	
	any callres = 0;
	bool res = false;
	if( manager_id==InvalidManagerID ) {
		GlobalFwd gf;
		if( !g_mmp.glfwd_manager.GetFwd(fwd_name, gf) ) {
			LogMessage("LibModuleManager_ExecForward :: Warning :: **** unable to get global forward '%s' ****", fwd_name);
			return false;
		}
		gf.Start();
		res = CallFwd(gf.callable, gf.param_type, 4, numParams, callres);
		SetNativeCellRef(3, callres);
		return res;
	} else {
		if( g_mmp.pf_managers.Size <= 0 ) {
			LogMessage("LibModuleManager_ExecForward :: Error :: **** there are no Private Forward Managers. ****");
			return false;
		}
		
		char id_key[CELL_KEY_SIZE]; PackCellToStr(manager_id, id_key);
		PrivateFwdsManager pfm;
		if( !g_mmp.pf_managers.GetArray(id_key, pfm, sizeof(pfm)) ) {
			LogMessage("LibModuleManager_ExecForward :: Warning :: **** unable to get private forward manager with id '%i' ****", manager_id);
			return false;
		}
		
		PrivateFwd pf;
		if( !pfm.GetFwd(fwd_name, pf) ) {
			LogMessage("LibModuleManager_ExecForward :: Warning :: **** unable to get private forward '%s' ****", fwd_name);
			return false;
		}
		pf.Start();
		res = CallFwd(pf.callable, pf.param_type, 4, numParams, callres);
		SetNativeCellRef(3, callres);
		return res;
	}
}

/// This doesn't work because the `[Set/Get]Native*` API cannot work
/// from outside of a native as it relies on the stack data given to the native.
/// Idea: Use `ArrayStack`?
stock bool CallFwd(Callable call, const ParamType params[MAX_FWD_PARAMS], int param, int numParams, any &result=0) {
	if( param > numParams || (param - 4) > MAX_FWD_PARAMS ) {
		return call.Finish(result);
	}
	switch( params[param - 4] ) {
		case Param_Cell, Param_Float: {
			any value = GetNativeCell(param);
			call.PushCell(value);
			return CallFwd(call, params, param + 1, numParams, result);
		}
		case Param_CellByRef, Param_FloatByRef: {
			any value = GetNativeCellRef(param);
			call.PushCellRef(value);
			bool res = CallFwd(call, params, param + 1, numParams, result);
			SetNativeCellRef(param, value);
			return res;
		}
		case Param_String: {
			if( IsNativeParamNullString(param) ) {
				call.PushNullString();
				return CallFwd(call, params, param + 1, numParams, result);
			}
			int len; GetNativeStringLength(param, len);
			len++;
			char[] cstr = new char[len];
			GetNativeString(param, cstr, len);
			call.PushString(cstr, len);
			bool res = CallFwd(call, params, param + 1, numParams, result);
			/// Is string supposed to be a buffer?
			SetNativeString(param, cstr, len);
			return res;
		}
		case Param_Array: {
			/// ????
			//GetNativeArray(param, any[] local, int size);
		}
	}
	return false;
}
*/