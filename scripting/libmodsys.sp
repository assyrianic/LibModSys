#include <sdktools>
#include <libmodsys>


#pragma semicolon         1
#pragma newdecls          required

#define PLUGIN_VERSION    "1.1.1a"


public Plugin myinfo = {
	name        = "LibModSys",
	author      = "Nergal",
	description = "Plugin that manages systems of subplugins.",
	version     = PLUGIN_VERSION,
	url         = "https://github.com/assyrianic/LibModSys"
};


enum {
	FlagLocked = 1 << 0,
	FlagFrozen = 1 << 1,
};
enum struct SharedMapEntry {
	Handle   owner; /// owner that can [un]lock/[un]freeze this entry.
	any      data;  /// if any Array type, this is a DataPack.
	Function fn;
	int      len;
	SPType   tag;   /// see anonymous enum in 'plugin_utils.inc'
	int      access;
	
	
	bool PluginCanMutate(Handle pl) {
		return this.owner==pl || (this.access & FlagFrozen)==0;
	}
	bool PluginCanDelete(Handle pl) {
		return this.owner==pl || (this.access & FlagLocked)==0;
	}
	
	void InitAny(Handle plugin, any cell, SPType sptype=AnyType) {
		this.tag   = sptype;
		this.data  = cell;
		this.owner = plugin;
		this.len   = 1;
	}
	
	void InitFloat(Handle plugin, float cell) {
		this.tag   = FloatType;
		this.data  = cell;
		this.owner = plugin;
		this.len   = 1;
	}
	
	void InitFunc(Handle plugin, Function fn) {
		this.tag   = FuncType;
		this.fn    = fn;
		this.owner = plugin;
		this.len   = 1;
	}
	
	void InitAnyArray(Handle plugin, const any[] data, int len, SPType sptype=AnyType) {
		this.tag    = sptype | ArrayType;
		DataPack dp = new DataPack();
		dp.WriteCellArray(data, len);
		this.owner  = plugin;
		this.len    = len;
		this.data   = dp;
	}
	
	void InitStr(Handle plugin, const char[] data, int len) {
		this.tag    = CharType | ArrayType;
		DataPack dp = new DataPack();
		dp.WriteString(data);
		this.owner  = plugin;
		this.len    = len;
		this.data   = dp;
	}
	
	void Destroy() {
		if( !( this.tag & ArrayType ) ) {
			return;
		}
		DataPack dp = this.data;
		delete dp;
		this.data = 0;
		this.len  = 0;
	}
}

enum struct ModuleManagerCvars {
	ConVar Enabled;
	//ConVar MainGlobalFwdsCfg;
}

enum struct ModuleManagerPlugin {
	ModuleManagerCvars cvars;          /// 
	
	GlobalFwdsManager  glfwd_manager;  /// 
	
	ManagerID          manager_ids;    /// manager IDs to track specific plugin managers.
	
	StringMap          pf_managers;    /// map[ManagerID]PrivateFwdsManager
	StringMap          pl_managers;    /// map[ManagerID]ModuleManager
	
	StringMap          shmap_managers; /// map[string]ManagerID
	StringMap          shmap_ids;      /// map[ManagerID]SharedMap
	
	
	void Init() {
		this.glfwd_manager.Init("configs/plugin_manager/global_fwds.cfg");
		this.pf_managers    = new StringMap();
		this.pl_managers    = new StringMap();
		this.shmap_managers = new StringMap();
		this.shmap_ids      = new StringMap();
		this.manager_ids    = IntToAny(1);
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
		
		StringMap          shmap_managers; /// map[string]ManagerID
		StringMap          shmap_ids;      /// map[ManagerID]SharedMap
		
		/// destroy SharedMaps.
		snap = this.shmap_ids.Snapshot();
		if( snap != null ) {
			int len = snap.Length;
			for( int i; i < len; i++ ) {
				int keysize = snap.KeyBufferSize(i) + 1;
				char[] name = new char[keysize];
				snap.GetKey(i, name, keysize);
				
				StringMap shared_map;
				if( !this.shmap_ids.GetValue(name, shared_map) ) {
					continue;
				}
				
				StringMapSnapshot shared_map_snap = shared_map.Snapshot();
				int shared_map_len = shared_map_snap.Length;
				for( int j; j < shared_map_len; j++ ) {
					int shared_map_keysize = shared_map_snap.KeyBufferSize(j) + 1;
					char[] shared_map_name = new char[shared_map_keysize];
					shared_map_snap.GetKey(j, shared_map_name, shared_map_keysize);
					
					SharedMapEntry sme;
					if( !shared_map.GetArray(shared_map_name, sme, sizeof(sme)) ) {
						continue;
					}
					sme.Destroy();
				}
				delete shared_map_snap;
				delete shared_map;
			}
			delete snap;
		}
		delete this.shmap_managers;
		delete this.shmap_ids;
	}
}

static ModuleManagerPlugin g_mmp;


public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max) {
	g_mmp.Init();
	
	/// Global Forwards Manager natives.
	CreateNative("LibModSys_GetGlobalFwdsManager",   Native_LibModSys_GetGlobalFwdManager);
	
	/// Private Forwards Manager natives.
	CreateNative("LibModSys_MakePrivateFwdsManager", Native_LibModSys_MakePrivateFwdManager);
	CreateNative("LibModSys_GetPrivateFwdsManager",  Native_LibModSys_GetPrivateFwdManager);
	CreateNative("LibModSys_PrivateFwdHook",         Native_LibModSys_PrivateFwdHook);
	CreateNative("LibModSys_PrivateFwdUnhook",       Native_LibModSys_PrivateFwdUnhook);
	CreateNative("LibModSys_PrivateFwdUnhookAll",    Native_LibModSys_PrivateFwdUnhookAll);
	//CreateNative("LibModSys_PrivateFwdUnhookAll",    Native_LibModSys_ExecForward);
	
	/// Plugin/Module Manager natives.
	CreateNative("LibModSys_MakeModuleManager",      Native_LibModSys_MakeModuleManager);
	CreateNative("LibModSys_GetModuleManager",       Native_LibModSys_GetModuleManager);
	CreateNative("LibModSys_RegisterModule",         Native_LibModSys_RegisterModule);
	CreateNative("LibModSys_UnregisterModule",       Native_LibModSys_UnregisterModule);
	
	/// SharedMap natives.
	CreateNative("SharedMap.SharedMap",              Native_SharedMap_SharedMap);
	CreateNative("SharedMap.GetInt",                 Native_SharedMap_GetInt);
	CreateNative("SharedMap.GetFloat",               Native_SharedMap_GetFloat);
	CreateNative("SharedMap.GetAny",                 Native_SharedMap_GetAny);
	CreateNative("SharedMap.GetStrLen",              Native_SharedMap_GetStrLen);
	CreateNative("SharedMap.GetStr",                 Native_SharedMap_GetStr);
	CreateNative("SharedMap.GetArrLen",              Native_SharedMap_GetArrLen);
	CreateNative("SharedMap.GetArr",                 Native_SharedMap_GetArr);
	CreateNative("SharedMap.GetOwner",               Native_SharedMap_GetOwner);
	
	CreateNative("SharedMap.SetInt",                 Native_SharedMap_SetInt);
	CreateNative("SharedMap.SetFloat",               Native_SharedMap_SetFloat);
	CreateNative("SharedMap.SetAny",                 Native_SharedMap_SetAny);
	CreateNative("SharedMap.SetStr",                 Native_SharedMap_SetStr);
	CreateNative("SharedMap.SetArr",                 Native_SharedMap_SetArr);
	
	CreateNative("SharedMap.Has",                    Native_SharedMap_Has);
	CreateNative("SharedMap.TypeOf",                 Native_SharedMap_TypeOf);
	CreateNative("SharedMap.Delete",                 Native_SharedMap_Delete);
	
	CreateNative("SharedMap.IsLocked",               Native_SharedMap_IsLocked);
	CreateNative("SharedMap.IsFrozen",               Native_SharedMap_IsFrozen);
	
	CreateNative("SharedMap.Lock",                   Native_SharedMap_Lock);
	CreateNative("SharedMap.Unlock",                 Native_SharedMap_Unlock);
	CreateNative("SharedMap.Freeze",                 Native_SharedMap_Freeze);
	CreateNative("SharedMap.Unfreeze",               Native_SharedMap_Unfreeze);
	
	CreateNative("SharedMap.Len.get",                Native_SharedMap_Len_get);
	
	/// SharedMap oriented natives.
	CreateNative("LibModSys_DestroySharedMap",       Native_LibModSys_DestroySharedMap);
	CreateNative("LibModSys_ClearSharedMap",         Native_LibModSys_ClearSharedMap);
	CreateNative("LibModSys_ChannelExists",          Native_LibModSys_ChannelExists);
	
	RegPluginLibrary("LibModSys");
	return APLRes_Success;
}


/// bool LibModSys_GetGlobalFwdsManager(GlobalFwdsManager glfwd_manager);
public any Native_LibModSys_GetGlobalFwdManager(Handle plugin, int numParams) {
	return SetNativeArray(1, g_mmp.glfwd_manager, sizeof(g_mmp.glfwd_manager))==SP_ERROR_NONE;
}

/// ManagerID LibModSys_MakePrivateFwdsManager(const char[] cfgfile);
public any Native_LibModSys_MakePrivateFwdManager(Handle plugin, int numParams) {
	ManagerID pfm_id = g_mmp.GenerateManagerID();
	if( pfm_id==InvalidManagerID ) {
		LogError("LibModSys_MakePrivateFwdsManager :: Error :: **** out of IDs! ****");
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
	//PrintToServer("LibModSys_MakePrivateFwdsManager :: pfm_id == '%i' | id_key == '%i' '%i' '%i' '%i' '%i' '%i'", pfm_id, id_key[0] & 0xFF, id_key[1] & 0xFF, id_key[2] & 0xFF, id_key[3] & 0xFF, id_key[4] & 0xFF, id_key[5] & 0xFF);
	g_mmp.pf_managers.SetArray(id_key, pfm, sizeof(pfm));
	return pfm_id;
}

/// bool LibModSys_GetPrivateFwdsManager(ManagerID id, PrivateFwdsManager buf);
public any Native_LibModSys_GetPrivateFwdManager(Handle plugin, int numParams) {
	ManagerID pfm_id = GetNativeCell(1);
	if( pfm_id==InvalidManagerID ) {
		LogMessage("LibModSys_GetPrivateFwdsManager :: Error :: **** invalid Manager ID! ****");
		return false;
	} else if( g_mmp.pf_managers.Size <= 0 ) {
		LogMessage("LibModSys_GetPrivateFwdsManager :: Error :: **** there are no Private Forward Managers. ****");
		return false;
	}
	
	char id_key[CELL_KEY_SIZE]; PackCellToStr(pfm_id, id_key);
	//PrintToServer("LibModSys_GetPrivateFwdsManager :: pfm_id == '%i' | id_key == '%i' '%i' '%i' '%i' '%i' '%i'", pfm_id, id_key[0] & 0xFF, id_key[1] & 0xFF, id_key[2] & 0xFF, id_key[3] & 0xFF, id_key[4] & 0xFF, id_key[5] & 0xFF);
	PrivateFwdsManager pfm;
	if( g_mmp.pf_managers.GetArray(id_key, pfm, sizeof(pfm)) ) {
		SetNativeArray(2, pfm, sizeof(pfm));
		return true;
	}
	LogMessage("LibModSys_GetPrivateFwdsManager :: Warning :: **** unable to get manager with id '%i' ****", pfm_id);
	return false;
}

/// bool LibModSys_PrivateFwdHook(ManagerID id, const char[] fwd_name, Function f);
public any Native_LibModSys_PrivateFwdHook(Handle plugin, int numParams) {
	ManagerID pfm_id = GetNativeCell(1);
	if( pfm_id==InvalidManagerID ) {
		LogMessage("LibModSys_PrivateFwdHook :: Error :: **** invalid Manager ID! ****");
		return false;
	} else if( g_mmp.pf_managers.Size <= 0 ) {
		LogMessage("LibModSys_PrivateFwdHook :: Error :: **** there are no Private Forward Managers. ****");
		return false;
	}
	
	char id_key[CELL_KEY_SIZE]; PackCellToStr(pfm_id, id_key);
	PrivateFwdsManager pfm;
	if( !g_mmp.pf_managers.GetArray(id_key, pfm, sizeof(pfm)) ) {
		LogMessage("LibModSys_PrivateFwdHook :: Warning :: **** unable to get forward manager with id '%i' ****", pfm_id);
		return false;
	}
	
	int fwd_name_len; GetNativeStringLength(2, fwd_name_len);
	fwd_name_len++;
	char[] fwd_name = new char[fwd_name_len];
	GetNativeString(2, fwd_name, fwd_name_len);
	
	PrivateFwd pf;
	if( !pfm.GetFwd(fwd_name, pf) ) {
		LogMessage("LibModSys_PrivateFwdHook :: Warning :: **** unable to get forward '%s' ****", fwd_name);
		return false;
	}
	
	Function f = GetNativeFunction(3);
	return pf.Hook(plugin, f);
}

/// bool LibModSys_PrivateFwdUnhook(ManagerID id, const char[] fwd_name, Function f);
public any Native_LibModSys_PrivateFwdUnhook(Handle plugin, int numParams) {
	ManagerID pfm_id = GetNativeCell(1);
	if( pfm_id==InvalidManagerID ) {
		LogMessage("LibModSys_PrivateFwdUnhook :: Error :: **** invalid Manager ID! ****");
		return false;
	} else if( g_mmp.pf_managers.Size <= 0 ) {
		LogMessage("LibModSys_PrivateFwdUnhook :: Error :: **** there are no Private Forward Managers. ****");
		return false;
	}
	
	char id_key[CELL_KEY_SIZE]; PackCellToStr(pfm_id, id_key);
	PrivateFwdsManager pfm;
	if( !g_mmp.pf_managers.GetArray(id_key, pfm, sizeof(pfm)) ) {
		LogMessage("LibModSys_PrivateFwdUnhook :: Warning :: **** unable to get forward manager with id '%i' ****", pfm_id);
		return false;
	}
	
	int fwd_name_len; GetNativeStringLength(2, fwd_name_len);
	fwd_name_len++;
	char[] fwd_name = new char[fwd_name_len];
	GetNativeString(2, fwd_name, fwd_name_len);
	
	PrivateFwd pf;
	if( !pfm.GetFwd(fwd_name, pf) ) {
		LogMessage("LibModSys_PrivateFwdUnhook :: Warning :: **** unable to get forward '%s' ****", fwd_name);
		return false;
	}
	
	Function f = GetNativeFunction(3);
	return pf.Unhook(plugin, f);
}

/// bool LibModSys_PrivateFwdUnhookAll(ManagerID id, const char[] fwd_name);
public any Native_LibModSys_PrivateFwdUnhookAll(Handle plugin, int numParams) {
	ManagerID pfm_id = GetNativeCell(1);
	if( pfm_id==InvalidManagerID ) {
		LogMessage("LibModSys_PrivateFwdUnhookAll :: Error :: **** invalid Manager ID! ****");
		return false;
	} else if( g_mmp.pf_managers.Size <= 0 ) {
		LogMessage("LibModSys_PrivateFwdUnhookAll :: Error :: **** there are no Private Forward Managers. ****");
		return false;
	}
	
	char id_key[CELL_KEY_SIZE]; PackCellToStr(pfm_id, id_key);
	PrivateFwdsManager pfm;
	if( !g_mmp.pf_managers.GetArray(id_key, pfm, sizeof(pfm)) ) {
		LogMessage("LibModSys_PrivateFwdUnhookAll :: Warning :: **** unable to get forward manager with id '%i' ****", pfm_id);
		return false;
	}
	
	int fwd_name_len; GetNativeStringLength(2, fwd_name_len);
	fwd_name_len++;
	char[] fwd_name = new char[fwd_name_len];
	GetNativeString(2, fwd_name, fwd_name_len);
	
	PrivateFwd pf;
	if( !pfm.GetFwd(fwd_name, pf) ) {
		LogMessage("LibModSys_PrivateFwdUnhookAll :: Warning :: **** unable to get forward '%s' ****", fwd_name);
		return false;
	}
	return pf.UnhookAll(plugin);
}

/// ManagerID LibModSys_MakeModuleManager(const char[] cfgfile);
public any Native_LibModSys_MakeModuleManager(Handle plugin, int numParams) {
	ManagerID mm_id = g_mmp.GenerateManagerID();
	if( mm_id==InvalidManagerID ) {
		LogError("LibModSys_MakeModuleManager :: Error :: **** out of IDs! ****");
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
	//PrintToServer("LibModSys_MakeModuleManager :: mm_id == '%i' | id_key == '%i' '%i' '%i' '%i' '%i' '%i'", mm_id, id_key[0] & 0xFF, id_key[1] & 0xFF, id_key[2] & 0xFF, id_key[3] & 0xFF, id_key[4] & 0xFF, id_key[5] & 0xFF);
	g_mmp.pl_managers.SetArray(id_key, mm, sizeof(mm));
	return mm_id;
}

/// bool LibModSys_GetModuleManager(ManagerID id, /** ModuleManager */ any[] buf);
public any Native_LibModSys_GetModuleManager(Handle plugin, int numParams) {
	ManagerID mm_id = GetNativeCell(1);
	if( mm_id==InvalidManagerID ) {
		LogMessage("LibModSys_GetModuleManager :: Error :: **** invalid Manager ID! ****");
		return false;
	} else if( g_mmp.pl_managers.Size <= 0 ) {
		LogMessage("LibModSys_GetModuleManager :: Error :: **** there are no Module Managers. ****");
		return false;
	}
	
	char id_key[CELL_KEY_SIZE]; PackCellToStr(mm_id, id_key);
	//PrintToServer("LibModSys_GetModuleManager :: mm_id == '%i' | id_key == '%i' '%i' '%i' '%i' '%i' '%i'", mm_id, id_key[0] & 0xFF, id_key[1] & 0xFF, id_key[2] & 0xFF, id_key[3] & 0xFF, id_key[4] & 0xFF, id_key[5] & 0xFF);
	ModuleManager mm;
	if( g_mmp.pl_managers.GetArray(id_key, mm, sizeof(mm)) ) {
		SetNativeArray(2, mm, sizeof(mm));
		return true;
	}
	LogMessage("LibModSys_GetModuleManager :: Warning :: **** unable to get manager with id '%i' ****", mm_id);
	return false;
}

/// bool LibModSys_RegisterModule(ManagerID id, const char[] name, int flags=0, int priority=0, int component=0, int group=0);
public any Native_LibModSys_RegisterModule(Handle plugin, int numParams) {
	ManagerID mm_id = GetNativeCell(1);
	if( mm_id==InvalidManagerID ) {
		LogMessage("LibModSys_RegisterModule :: Error :: **** invalid Manager ID! ****");
		return false;
	} else if( g_mmp.pl_managers.Size <= 0 ) {
		LogMessage("LibModSys_RegisterModule :: Error :: **** there are no Module Managers. ****");
		return false;
	}
	
	char id_key[CELL_KEY_SIZE]; PackCellToStr(mm_id, id_key);
	ModuleManager mm;
	if( !g_mmp.pl_managers.GetArray(id_key, mm, sizeof(mm)) ) {
		LogMessage("LibModSys_RegisterModule :: Warning :: **** unable to get module manager with id '%i' ****", mm_id);
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

/// bool LibModSys_UnregisterModule(ManagerID id, const char[] name);
public any Native_LibModSys_UnregisterModule(Handle plugin, int numParams) {
	ManagerID mm_id = GetNativeCell(1);
	if( mm_id==InvalidManagerID ) {
		LogMessage("LibModSys_UnregisterModule :: Error :: **** invalid Manager ID! ****");
		return false;
	} else if( g_mmp.pl_managers.Size <= 0 ) {
		LogMessage("LibModSys_UnregisterModule :: Error :: **** there are no Module Managers. ****");
		return false;
	}
	
	char id_key[CELL_KEY_SIZE]; PackCellToStr(mm_id, id_key);
	ModuleManager mm;
	if( !g_mmp.pl_managers.GetArray(id_key, mm, sizeof(mm)) ) {
		LogMessage("LibModSys_UnregisterModule :: Warning :: **** unable to get module manager with id '%i' ****", mm_id);
		return false;
	}
	
	int pl_name_len; GetNativeStringLength(2, pl_name_len);
	pl_name_len++;
	char[] pl_name = new char[pl_name_len];
	GetNativeString(2, pl_name, pl_name_len);
	return mm.modules.Remove(pl_name);
}

/// SharedMap(const char[] channel);
public any Native_SharedMap_SharedMap(Handle plugin, int numParams) {
	int len; GetNativeStringLength(1, len);
	len++;
	char[] channel = new char[len];
	GetNativeString(1, channel, len);
	
	ManagerID shmap_id;
	if( g_mmp.shmap_managers.GetValue(channel, shmap_id) && shmap_id != InvalidManagerID ) {
		return shmap_id;
	}
	
	shmap_id = g_mmp.GenerateManagerID();
	StringMap shared_map = new StringMap();
	shared_map.SetValue("__dict_owner__", plugin);
	
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	g_mmp.shmap_ids.SetValue(id_key, shared_map);
	g_mmp.shmap_managers.SetValue(channel, shmap_id);
	return shmap_id;
}

/// bool GetInt(const char[] prop, int &i);
public any Native_SharedMap_GetInt(Handle plugin, int numParams) {
	ManagerID shmap_id = GetNativeCell(1);
	if( shmap_id==InvalidManagerID ) {
		return false;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return false;
	}
	
	int len; GetNativeStringLength(2, len);
	len++;
	char[] prop = new char[len];
	GetNativeString(2, prop, len);
	
	SharedMapEntry entry;
	if( !shared_map.GetArray(prop, entry, sizeof(entry)) || entry.tag != IntType ) {
		return false;
	}
	SetNativeCellRef(3, entry.data);
	return true;
}

/// bool GetFloat(const char[] prop, float &f);
public any Native_SharedMap_GetFloat(Handle plugin, int numParams) {
	ManagerID shmap_id = GetNativeCell(1);
	if( shmap_id==InvalidManagerID ) {
		return false;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return false;
	}
	
	int len; GetNativeStringLength(2, len);
	len++;
	char[] prop = new char[len];
	GetNativeString(2, prop, len);
	
	SharedMapEntry entry;
	if( !shared_map.GetArray(prop, entry, sizeof(entry)) || entry.tag != FloatType ) {
		return false;
	}
	SetNativeCellRef(3, entry.data);
	return true;
}

/// bool GetAny(const char[] prop, any &a, SPType sp_type=AnyType);
public any Native_SharedMap_GetAny(Handle plugin, int numParams) {
	ManagerID shmap_id = GetNativeCell(1);
	if( shmap_id==InvalidManagerID ) {
		return false;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return false;
	}
	
	int len; GetNativeStringLength(2, len);
	len++;
	char[] prop = new char[len];
	GetNativeString(2, prop, len);
	
	SharedMapEntry entry;
	SPType sptype = GetNativeCell(4);
	if( !shared_map.GetArray(prop, entry, sizeof(entry)) || entry.tag != sptype ) {
		return false;
	}
	SetNativeCellRef(3, entry.data);
	return true;
}

/// int GetStrLen(const char[] prop);
public any Native_SharedMap_GetStrLen(Handle plugin, int numParams) {
	ManagerID shmap_id = GetNativeCell(1);
	if( shmap_id==InvalidManagerID ) {
		return -1;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return -1;
	}
	
	int len; GetNativeStringLength(2, len);
	len++;
	char[] prop = new char[len];
	GetNativeString(2, prop, len);
	
	SharedMapEntry entry;
	if( !shared_map.GetArray(prop, entry, sizeof(entry)) || entry.tag != (CharType | ArrayType) ) {
		return -1;
	}
	return entry.len;
}

/// int GetStr(const char[] prop, char[] buf, int len);
public any Native_SharedMap_GetStr(Handle plugin, int numParams) {
	ManagerID shmap_id = GetNativeCell(1);
	if( shmap_id==InvalidManagerID ) {
		return -1;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return -1;
	}
	
	int len; GetNativeStringLength(2, len);
	len++;
	char[] prop = new char[len];
	GetNativeString(2, prop, len);
	
	SharedMapEntry entry;
	if( !shared_map.GetArray(prop, entry, sizeof(entry)) || entry.tag != (CharType | ArrayType) ) {
		return -1;
	}
	
	int buf_len = GetNativeCell(4);
	char[] buf = new char[buf_len];
	DataPack dp = entry.data;
	dp.Reset();
	dp.ReadString(buf, buf_len);
	SetNativeString(3, buf, buf_len);
	return buf_len;
}

/// int GetArrLen(const char[] prop);
public any Native_SharedMap_GetArrLen(Handle plugin, int numParams) {
	ManagerID shmap_id = GetNativeCell(1);
	if( shmap_id==InvalidManagerID ) {
		return -1;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return -1;
	}
	
	int len; GetNativeStringLength(2, len);
	len++;
	char[] prop = new char[len];
	GetNativeString(2, prop, len);
	
	SharedMapEntry entry;
	if( !shared_map.GetArray(prop, entry, sizeof(entry)) || !(entry.tag & ArrayType) ) {
		return -1;
	}
	return entry.len;
}

/// int GetArr(const char[] prop, any[] buf, int len, SPType sp_type=AnyType);
public any Native_SharedMap_GetArr(Handle plugin, int numParams) {
	ManagerID shmap_id = GetNativeCell(1);
	if( shmap_id==InvalidManagerID ) {
		return -1;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return -1;
	}
	
	int len; GetNativeStringLength(2, len);
	len++;
	char[] prop = new char[len];
	GetNativeString(2, prop, len);
	
	SharedMapEntry entry;
	SPType sptype = GetNativeCell(5);
	if( !shared_map.GetArray(prop, entry, sizeof(entry)) || entry.tag != (sptype | ArrayType) ) {
		return -1;
	}
	
	int buf_len = GetNativeCell(4);
	any[] buf = new any[buf_len];
	DataPack dp = entry.data;
	dp.Reset();
	dp.ReadCellArray(buf, buf_len);
	SetNativeArray(3, buf, buf_len);
	return buf_len;
}

/// Handle GetOwner(const char[] prop);
public any Native_SharedMap_GetOwner(Handle plugin, int numParams) {
	ManagerID shmap_id = GetNativeCell(1);
	if( shmap_id==InvalidManagerID ) {
		return 0;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return 0;
	}
	
	int len; GetNativeStringLength(2, len);
	len++;
	char[] prop = new char[len];
	GetNativeString(2, prop, len);
	
	SharedMapEntry entry;
	if( !shared_map.GetArray(prop, entry, sizeof(entry)) ) {
		return 0;
	}
	
	if( !IsValidPlugin(entry.owner) ) {
		entry.owner = null;
	}
	return entry.owner;
}

/// bool SetInt(const char[] prop, int value);
public any Native_SharedMap_SetInt(Handle plugin, int numParams) {
	ManagerID shmap_id = GetNativeCell(1);
	if( shmap_id==InvalidManagerID ) {
		return false;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return false;
	}
	
	int len; GetNativeStringLength(2, len);
	len++;
	char[] prop = new char[len];
	GetNativeString(2, prop, len);
	
	int value = GetNativeCell(3);
	Handle shared_map_owner; shared_map.GetValue("__dict_owner__", shared_map_owner);
	
	SharedMapEntry entry;
	if( !shared_map.ContainsKey(prop) ) {
		entry.InitAny(plugin, value, IntType);
		entry.access |= (FlagFrozen|FlagLocked);
		return shared_map.SetArray(prop, entry, sizeof(entry));
	} else if( !shared_map.GetArray(prop, entry, sizeof(entry)) || entry.tag != IntType ) {
		return false;
	} else if( !( entry.PluginCanMutate(plugin) || shared_map_owner==plugin ) ) {
		return false;
	}
	
	entry.data = value;
	return shared_map.SetArray(prop, entry, sizeof(entry));
}

/// bool SetFloat(const char[] prop, float value);
public any Native_SharedMap_SetFloat(Handle plugin, int numParams) {
	ManagerID shmap_id = GetNativeCell(1);
	if( shmap_id==InvalidManagerID ) {
		return false;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return false;
	}
	
	int len; GetNativeStringLength(2, len);
	len++;
	char[] prop = new char[len];
	GetNativeString(2, prop, len);
	
	float value = GetNativeCell(3);
	Handle shared_map_owner; shared_map.GetValue("__dict_owner__", shared_map_owner);
	
	SharedMapEntry entry;
	if( !shared_map.ContainsKey(prop) ) {
		entry.InitFloat(plugin, value);
		entry.access |= (FlagFrozen|FlagLocked);
		return shared_map.SetArray(prop, entry, sizeof(entry));
	} else if( !shared_map.GetArray(prop, entry, sizeof(entry)) || entry.tag != FloatType ) {
		return false;
	} else if( !( entry.PluginCanMutate(plugin) || shared_map_owner==plugin ) ) {
		return false;
	}
	
	entry.data = value;
	return shared_map.SetArray(prop, entry, sizeof(entry));
}

/// bool SetAny(const char[] prop, any value, SPType sp_type=AnyType);
public any Native_SharedMap_SetAny(Handle plugin, int numParams) {
	ManagerID shmap_id = GetNativeCell(1);
	if( shmap_id==InvalidManagerID ) {
		return false;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return false;
	}
	
	int len; GetNativeStringLength(2, len);
	len++;
	char[] prop = new char[len];
	GetNativeString(2, prop, len);
	
	any value = GetNativeCell(3);
	SPType sptype = GetNativeCell(4);
	Handle shared_map_owner; shared_map.GetValue("__dict_owner__", shared_map_owner);
	
	SharedMapEntry entry;
	if( !shared_map.ContainsKey(prop) ) {
		entry.InitAny(plugin, value, sptype);
		entry.access |= (FlagFrozen|FlagLocked);
		return shared_map.SetArray(prop, entry, sizeof(entry));
	} else if( !shared_map.GetArray(prop, entry, sizeof(entry)) || entry.tag != sptype ) {
		return false;
	} else if( !( entry.PluginCanMutate(plugin) || shared_map_owner==plugin ) ) {
		return false;
	}
	
	entry.data = value;
	return shared_map.SetArray(prop, entry, sizeof(entry));
}

/// bool SetStr(const char[] prop, const char[] value);
public any Native_SharedMap_SetStr(Handle plugin, int numParams) {
	ManagerID shmap_id = GetNativeCell(1);
	if( shmap_id==InvalidManagerID ) {
		return false;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return false;
	}
	
	int len; GetNativeStringLength(2, len);
	len++;
	char[] prop = new char[len];
	GetNativeString(2, prop, len);
	
	int val_len; GetNativeStringLength(3, val_len);
	val_len++;
	char[] val_str = new char[val_len];
	GetNativeString(3, val_str, val_len);
	Handle shared_map_owner; shared_map.GetValue("__dict_owner__", shared_map_owner);
	
	SharedMapEntry entry;
	if( !shared_map.ContainsKey(prop) ) {
		entry.InitStr(plugin, val_str, val_len);
		entry.access |= (FlagFrozen|FlagLocked);
		return shared_map.SetArray(prop, entry, sizeof(entry));
	} else if( !shared_map.GetArray(prop, entry, sizeof(entry)) || entry.tag != (CharType | ArrayType) ) {
		return false;
	} else if( !( entry.PluginCanMutate(plugin) || shared_map_owner==plugin ) ) {
		return false;
	}
	
	DataPack dp = entry.data;
	dp.Reset(true);
	dp.WriteString(val_str);
	entry.len = val_len;
	return shared_map.SetArray(prop, entry, sizeof(entry));
}

/// bool SetArr(const char[] prop, const any[] value, int len, SPType sp_type=AnyType);
public any Native_SharedMap_SetArr(Handle plugin, int numParams) {
	ManagerID shmap_id = GetNativeCell(1);
	if( shmap_id==InvalidManagerID ) {
		return false;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return false;
	}
	
	int len; GetNativeStringLength(2, len);
	len++;
	char[] prop = new char[len];
	GetNativeString(2, prop, len);
	
	int val_len   = GetNativeCell(4);
	SPType sptype = GetNativeCell(5);
	any[] val_arr = new any[val_len];
	GetNativeArray(3, val_arr, val_len);
	Handle shared_map_owner; shared_map.GetValue("__dict_owner__", shared_map_owner);
	
	SharedMapEntry entry;
	if( !shared_map.ContainsKey(prop) ) {
		entry.InitAnyArray(plugin, val_arr, val_len, sptype);
		entry.access |= (FlagFrozen|FlagLocked);
		return shared_map.SetArray(prop, entry, sizeof(entry));
	} else if( !shared_map.GetArray(prop, entry, sizeof(entry)) || entry.tag != (sptype | ArrayType) ) {
		return false;
	} else if( !( entry.PluginCanMutate(plugin) || shared_map_owner==plugin ) ) {
		return false;
	}
	
	DataPack dp = entry.data;
	dp.Reset(true);
	dp.WriteCellArray(val_arr, val_len);
	entry.len = val_len;
	return shared_map.SetArray(prop, entry, sizeof(entry));
}

/// bool Has(const char[] prop);
public any Native_SharedMap_Has(Handle plugin, int numParams) {
	ManagerID shmap_id = GetNativeCell(1);
	if( shmap_id==InvalidManagerID ) {
		return false;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return false;
	}
	
	int len; GetNativeStringLength(2, len);
	len++;
	char[] prop = new char[len];
	GetNativeString(2, prop, len);
	return shared_map.ContainsKey(prop);
}

/// SPType TypeOf(const char[] prop);
public any Native_SharedMap_TypeOf(Handle plugin, int numParams) {
	ManagerID shmap_id = GetNativeCell(1);
	if( shmap_id==InvalidManagerID ) {
		return InvalidType;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return InvalidType;
	}
	
	int len; GetNativeStringLength(2, len);
	len++;
	char[] prop = new char[len];
	GetNativeString(2, prop, len);
	
	SharedMapEntry entry;
	if( !shared_map.GetArray(prop, entry, sizeof(entry)) ) {
		return InvalidType;
	}
	return entry.tag;
}

/// bool Delete(const char[] prop);
public any Native_SharedMap_Delete(Handle plugin, int numParams) {
	ManagerID shmap_id = GetNativeCell(1);
	if( shmap_id==InvalidManagerID ) {
		return false;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return false;
	}
	
	int len; GetNativeStringLength(2, len);
	len++;
	char[] prop = new char[len];
	GetNativeString(2, prop, len);
	Handle shared_map_owner; shared_map.GetValue("__dict_owner__", shared_map_owner);
	
	SharedMapEntry entry;
	if( !shared_map.GetArray(prop, entry, sizeof(entry)) ) {
		return false;
	} else if( !( entry.PluginCanDelete(plugin) || shared_map_owner==plugin ) ) {
		return false;
	}
	entry.Destroy();
	return shared_map.Remove(prop);
}

/// int IsLocked(const char[] prop);
public any Native_SharedMap_IsLocked(Handle plugin, int numParams) {
	ManagerID shmap_id = GetNativeCell(1);
	if( shmap_id==InvalidManagerID ) {
		return -1;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return -1;
	}
	
	int len; GetNativeStringLength(2, len);
	len++;
	char[] prop = new char[len];
	GetNativeString(2, prop, len);
	
	SharedMapEntry entry;
	if( !shared_map.GetArray(prop, entry, sizeof(entry)) ) {
		return -1;
	}
	return (entry.access & FlagLocked) > 0;
}

/// int IsFrozen(const char[] prop);
public any Native_SharedMap_IsFrozen(Handle plugin, int numParams) {
	ManagerID shmap_id = GetNativeCell(1);
	if( shmap_id==InvalidManagerID ) {
		return -1;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return -1;
	}
	
	int len; GetNativeStringLength(2, len);
	len++;
	char[] prop = new char[len];
	GetNativeString(2, prop, len);
	
	SharedMapEntry entry;
	if( !shared_map.GetArray(prop, entry, sizeof(entry)) ) {
		return -1;
	}
	return (entry.access & FlagFrozen) > 0;
}

/// bool Lock(const char[] prop);
public any Native_SharedMap_Lock(Handle plugin, int numParams) {
	ManagerID shmap_id = GetNativeCell(1);
	if( shmap_id==InvalidManagerID ) {
		return false;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return false;
	}
	
	int len; GetNativeStringLength(2, len);
	len++;
	char[] prop = new char[len];
	GetNativeString(2, prop, len);
	Handle shared_map_owner; shared_map.GetValue("__dict_owner__", shared_map_owner);
	
	SharedMapEntry entry;
	if( !shared_map.GetArray(prop, entry, sizeof(entry))
		/// can't lock an already locked prop.
		|| !( entry.owner==plugin || shared_map_owner==plugin ) || (entry.access & FlagLocked) > 0 ) {
		return false;
	}
	entry.access |= FlagLocked;
	return shared_map.SetArray(prop, entry, sizeof(entry));
}

/// bool Unlock(const char[] prop);
public any Native_SharedMap_Unlock(Handle plugin, int numParams) {
	ManagerID shmap_id = GetNativeCell(1);
	if( shmap_id==InvalidManagerID ) {
		return false;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return false;
	}
	
	int len; GetNativeStringLength(2, len);
	len++;
	char[] prop = new char[len];
	GetNativeString(2, prop, len);
	Handle shared_map_owner; shared_map.GetValue("__dict_owner__", shared_map_owner);
	
	SharedMapEntry entry;
	if( !shared_map.GetArray(prop, entry, sizeof(entry))
		/// can't unlock an already unlocked prop.
		|| !( entry.owner==plugin || shared_map_owner==plugin ) || (entry.access & FlagLocked)==0 ) {
		return false;
	}
	entry.access &= ~FlagLocked;
	return shared_map.SetArray(prop, entry, sizeof(entry));
}

/// bool Freeze(const char[] prop);
public any Native_SharedMap_Freeze(Handle plugin, int numParams) {
	ManagerID shmap_id = GetNativeCell(1);
	if( shmap_id==InvalidManagerID ) {
		return false;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return false;
	}
	
	int len; GetNativeStringLength(2, len);
	len++;
	char[] prop = new char[len];
	GetNativeString(2, prop, len);
	Handle shared_map_owner; shared_map.GetValue("__dict_owner__", shared_map_owner);
	
	SharedMapEntry entry;
	if( !shared_map.GetArray(prop, entry, sizeof(entry))
		/// can't lock an already locked prop.
		|| !( entry.owner==plugin || shared_map_owner==plugin ) || (entry.access & FlagFrozen) > 0 ) {
		return false;
	}
	entry.access |= FlagFrozen;
	return shared_map.SetArray(prop, entry, sizeof(entry));
}

/// bool Unfreeze(const char[] prop);
public any Native_SharedMap_Unfreeze(Handle plugin, int numParams) {
	ManagerID shmap_id = GetNativeCell(1);
	if( shmap_id==InvalidManagerID ) {
		return false;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return false;
	}
	
	int len; GetNativeStringLength(2, len);
	len++;
	char[] prop = new char[len];
	GetNativeString(2, prop, len);
	Handle shared_map_owner; shared_map.GetValue("__dict_owner__", shared_map_owner);
	
	SharedMapEntry entry;
	if( !shared_map.GetArray(prop, entry, sizeof(entry))
		/// can't unlock an already unlocked prop.
		|| !( entry.owner==plugin || shared_map_owner==plugin ) || (entry.access & FlagFrozen)==0 ) {
		return false;
	}
	entry.access &= ~FlagFrozen;
	return shared_map.SetArray(prop, entry, sizeof(entry));
}

/// property int Len
public any Native_SharedMap_Len_get(Handle plugin, int numParams) {
	ManagerID shmap_id = GetNativeCell(1);
	if( shmap_id==InvalidManagerID ) {
		return -1;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return -1;
	}
	return shared_map.Size;
}

/// bool LibModSys_DestroySharedMap(const char[] channel);
public any Native_LibModSys_DestroySharedMap(Handle plugin, int numParams) {
	int len; GetNativeStringLength(1, len);
	len++;
	char[] channel = new char[len];
	GetNativeString(1, channel, len);
	
	ManagerID shmap_id;
	if( !g_mmp.shmap_managers.GetValue(channel, shmap_id) || shmap_id==InvalidManagerID ) {
		return false;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return false;
	}
	
	Handle shared_map_owner;
	shared_map.GetValue("__dict_owner__", shared_map_owner);
	
	/// only the creator/owner plugin can destroy.
	if( plugin != shared_map_owner ) {
		return false;
	}
	
	StringMapSnapshot shared_map_snap = shared_map.Snapshot();
	if( shared_map_snap==null ) {
		return false;
	}
	
	int shared_map_len = shared_map_snap.Length;
	for( int i; i < shared_map_len; i++ ) {
		int shared_map_keysize = shared_map_snap.KeyBufferSize(i) + 1;
		char[] shared_map_name = new char[shared_map_keysize];
		shared_map_snap.GetKey(i, shared_map_name, shared_map_keysize);
		
		SharedMapEntry sme;
		if( !shared_map.GetArray(shared_map_name, sme, sizeof(sme)) ) {
			continue;
		}
		sme.Destroy();
	}
	
	delete shared_map_snap;
	delete shared_map;
	
	g_mmp.shmap_ids.Remove(id_key);
	g_mmp.shmap_managers.Remove(channel);
	return true;
}

/// bool LibModSys_ClearSharedMap(const char[] channel);
public any Native_LibModSys_ClearSharedMap(Handle plugin, int numParams) {
	int len; GetNativeStringLength(1, len);
	len++;
	char[] channel = new char[len];
	GetNativeString(1, channel, len);
	
	ManagerID shmap_id;
	if( !g_mmp.shmap_managers.GetValue(channel, shmap_id) || shmap_id==InvalidManagerID ) {
		return false;
	}
	
	StringMap shared_map;
	char id_key[CELL_KEY_SIZE]; PackCellToStr(shmap_id, id_key);
	if( !g_mmp.shmap_ids.GetValue(id_key, shared_map) ) {
		return false;
	}
	
	Handle shared_map_owner; shared_map.GetValue("__dict_owner__", shared_map_owner);
	/// only the creator/owner plugin can destroy.
	if( plugin != shared_map_owner ) {
		return false;
	}
	
	StringMapSnapshot shared_map_snap = shared_map.Snapshot();
	if( shared_map_snap==null ) {
		return false;
	}
	
	int shared_map_len = shared_map_snap.Length;
	for( int i; i < shared_map_len; i++ ) {
		int shared_map_keysize = shared_map_snap.KeyBufferSize(i) + 1;
		char[] shared_map_name = new char[shared_map_keysize];
		shared_map_snap.GetKey(i, shared_map_name, shared_map_keysize);
		
		SharedMapEntry sme;
		if( !shared_map.GetArray(shared_map_name, sme, sizeof(sme)) ) {
			continue;
		}
		sme.Destroy();
	}
	delete shared_map_snap;
	shared_map.Clear();
	return true;
}

/// bool LibModSys_ChannelExists(const char[] channel);
public any Native_LibModSys_ChannelExists(Handle plugin, int numParams) {
	int len; GetNativeStringLength(1, len);
	len++;
	char[] channel = new char[len];
	GetNativeString(1, channel, len);
	return g_mmp.shmap_managers.ContainsKey(channel);
}


/*
/// bool LibModSys_ExecForward(ManagerID id, const char[] name, any &result=0, ...);
public any Native_LibModSys_ExecForward(Handle plugin, int numParams) {
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
			LogMessage("LibModSys_ExecForward :: Warning :: **** unable to get global forward '%s' ****", fwd_name);
			return false;
		}
		gf.Start();
		res = CallFwd(gf.callable, gf.param_type, 4, numParams, callres);
		SetNativeCellRef(3, callres);
		return res;
	} else {
		if( g_mmp.pf_managers.Size <= 0 ) {
			LogMessage("LibModSys_ExecForward :: Error :: **** there are no Private Forward Managers. ****");
			return false;
		}
		
		char id_key[CELL_KEY_SIZE]; PackCellToStr(manager_id, id_key);
		PrivateFwdsManager pfm;
		if( !g_mmp.pf_managers.GetArray(id_key, pfm, sizeof(pfm)) ) {
			LogMessage("LibModSys_ExecForward :: Warning :: **** unable to get private forward manager with id '%i' ****", manager_id);
			return false;
		}
		
		PrivateFwd pf;
		if( !pfm.GetFwd(fwd_name, pf) ) {
			LogMessage("LibModSys_ExecForward :: Warning :: **** unable to get private forward '%s' ****", fwd_name);
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