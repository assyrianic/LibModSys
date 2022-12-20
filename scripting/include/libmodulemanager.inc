/**
 * libmodulemanager.inc
 *
 * Copyright [2022] Nergal the Ashurian
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE ANDNONINFRINGEMENT.
 *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#if defined _libmodulemanager_included
	#endinput
#endif
#define _libmodulemanager_included

#include <plugin_utils>


/// There's only a single Global Forward Manager so no ID needed.
/// This native will retrieve its data for you.
native bool LibModuleManager_GetGlobalFwdsManager(/** GlobalFwdsManager */ any[] buf);


/// helper to directly get a GlobalFwd object.
stock bool LibModuleManager_GetGlobalFwd(const char[] name, GlobalFwd buf) {
	GlobalFwdsManager gfm;
	if( !LibModuleManager_GetGlobalFwdsManager(gfm) ) {
		return false;
	}
	return gfm.GetFwd(name, buf);
}

stock int LibModuleManager_NumGlobalFwds() {
	GlobalFwdsManager gfm;
	if( !LibModuleManager_GetGlobalFwdsManager(gfm) ) {
		return -1;
	}
	return gfm.global_fwds.Size;
}

stock int LibModuleManager_GlobalFwdFuncCount(const char[] fwd_name) {
	GlobalFwd gf;
	if( !LibModuleManager_GetGlobalFwd(fwd_name, gf) ) {
		return -1;
	}
	return gf.gf.FunctionCount;
}


/** creates a private forward manager, assigned to an ID.
 * You get the ID whenever you need it.
 * Example usage:
 * ```cs
    /// Making manager + getting its 'handle'.
    ManagerID boss_manager = LibModuleManager_MakePrivateFwdsManager("configs/my_plugin_folder/forwards.cfg");
    
    /// grabbing private forward.
    PrivateFwd pf;
    LibModuleManager_GetPrivateFwd(boss_manager, "OnSomethingDoCall", pf);
    
    /// Hooking example.
    pf.Hook(plugin, OnBossSomethingDoCall);
 * ```
 */
native ManagerID LibModuleManager_MakePrivateFwdsManager(const char[] cfgfile);


/// There can be many different Private Forward Managers.
/// This is to allow a plugin to have different systems
/// use different Private Forward managers for themselves.

/// returns true if valid ID and was able to copy to 'buf'.
/// returns false if invalid ID or copy failed.
native bool LibModuleManager_GetPrivateFwdsManager(ManagerID id, /** PrivateFwdsManager */ any[] buf);


/// helper to directly get a PrivateFwd object.
stock bool LibModuleManager_GetPrivateFwd(ManagerID id, const char[] fwd_name, PrivateFwd buf) {
	PrivateFwdsManager pfm;
	if( !LibModuleManager_GetPrivateFwdsManager(id, pfm) ) {
		return false;
	}
	return pfm.GetFwd(fwd_name, buf);
}


stock int LibModuleManager_NumPrivateFwds(ManagerID id) {
	PrivateFwdsManager pfm;
	if( !LibModuleManager_GetPrivateFwdsManager(id, pfm) ) {
		return -1;
	}
	return pfm.private_fwds.Size;
}

stock int LibModuleManager_PrivateFwdFuncCount(ManagerID id, const char[] fwd_name) {
	PrivateFwd pf;
	if( !LibModuleManager_GetPrivateFwd(id, fwd_name, pf) ) {
		return -1;
	}
	return pf.pf.FunctionCount;
}

/**
 * Hooking & Unhooking natives.
 * just put the private forward manager id,
 * name of the forward you want to hook,
 * and the function to hook.
 */
native bool LibModuleManager_PrivateFwdHook(ManagerID id, const char[] fwd_name, Function f);
native bool LibModuleManager_PrivateFwdUnhook(ManagerID id, const char[] fwd_name, Function f);
native bool LibModuleManager_PrivateFwdUnhookAll(ManagerID id, const char[] fwd_name);


/// This is for making a general plugin manager
/// that is unrelated with private or global forwards.
/// can be used with them though!
native ManagerID LibModuleManager_MakeModuleManager(const char[] cfgfile);


/// self explanatory, gets the module manager via an id.
/// returns false if unable to get module manager for any reason.
native bool LibModuleManager_GetModuleManager(ManagerID id, /** ModuleManager */ any[] buf);


/// calling plugin will be [un]registered to a specific plugin manager via id.
/// `name` is for identifying the plugin by a specific name.
native bool LibModuleManager_RegisterModule(
	ManagerID id, const char[] name,
	int flags=0, int priority=0, int component=0, int group=0
);
native bool LibModuleManager_UnregisterModule(ManagerID id, const char[] name);


stock bool LibModuleManager_GetModule(ManagerID id, const char[] module_name, PluginModule buf) {
	ModuleManager mm;
	if( !LibModuleManager_GetModuleManager(id, mm) ) {
		return false;
	}
	return mm.GetModule(module_name, buf);
}

stock Function LibModuleManager_GetModuleFunc(ManagerID id, const char[] module_name, const char[] func_name) {
	PluginModule buf;
	if( !LibModuleManager_GetModule(id, module_name, buf) ) {
		return INVALID_FUNCTION;
	}
	return buf.GetFuncByName(func_name);
}

stock Handle LibModuleManager_GetModuleHandle(ManagerID id, const char[] module_name) {
	PluginModule buf;
	if( !LibModuleManager_GetModule(id, module_name, buf) ) {
		return null;
	}
	return buf.plugin;
}


stock bool LibModuleManager_SetModuleAttribs(ManagerID id, const char[] module_name, int flags=0, int priority=0, int component=0, int group=0) {
	ModuleManager mm;
	if( !LibModuleManager_GetModuleManager(id, mm) ) {
		return false;
	}
	
	PluginModule pl;
	if( !mm.GetModule(module_name, pl) ) {
		return false;
	}
	
	pl.flags     = flags;
	pl.priority  = priority;
	pl.component = component;
	pl.group     = group;
	return mm.modules.SetArray(module_name, pl, sizeof(pl));
}


stock int LibModuleManager_NumModules(ManagerID id) {
	ModuleManager mm;
	if( !LibModuleManager_GetModuleManager(id, mm) ) {
		return -1;
	}
	return mm.modules.Size;
}

/**
 * First use `LibModuleManager_NumModules` to get the length.
 * then allocate an array like:
 * PluginModule[] modules = new PluginModule[len];
 * Then pass it to this function to sort out.
 */
stock bool LibModuleManager_GetModulesByPriority(ManagerID id, PluginModule[] modules, int num_modules) {
	ModuleManager mm;
	if( !LibModuleManager_GetModuleManager(id, mm) ) {
		return false;
	}
	
	StringMapSnapshot snap = mm.modules.Snapshot();
	if( snap==null ) {
		return false;
	}
	
	int len = snap.Length;
	len = (len >= num_modules)? num_modules : len;
	for( int i; i < len; i++ ) {
		int keysize = snap.KeyBufferSize(i) + 1;
		char[] name = new char[keysize];
		snap.GetKey(i, name, keysize);
		if( !mm.modules.GetArray(name, modules[i], sizeof(PluginModule)) ) {
			break;
		}
	}
	delete snap;
	
	for( int i; i < len; i++ ) {
		for( int n; n < len; n++ ) {
			if( i==n ) {
				continue;
			} else if( modules[n].priority < modules[i].priority ) {
				/// highest priority is first index.
				PluginModule tmp; tmp = modules[i];
				modules[i] = modules[n];
				modules[n] = tmp;
			}
		}
	}
	return true;
}

stock int LibModuleManager_GetModulesByComponent(ManagerID id, PluginModule[] modules, int component) {
	ModuleManager mm;
	if( !LibModuleManager_GetModuleManager(id, mm) ) {
		return -1;
	}
	
	StringMapSnapshot snap = mm.modules.Snapshot();
	if( snap==null ) {
		return -1;
	}
	
	int len = snap.Length;
	int has_component;
	for( int i; i < len; i++ ) {
		int keysize = snap.KeyBufferSize(i) + 1;
		char[] name = new char[keysize];
		snap.GetKey(i, name, keysize);
		PluginModule pl;
		if( !mm.modules.GetArray(name, pl, sizeof(pl)) ) {
			break;
		} else if( pl.component==component ) {
			modules[has_component++] = pl;
		}
	}
	delete snap;
	return has_component;
}

stock int LibModuleManager_GetModulesByGroup(ManagerID id, PluginModule[] modules, int group) {
	ModuleManager mm;
	if( !LibModuleManager_GetModuleManager(id, mm) ) {
		return -1;
	}
	
	StringMapSnapshot snap = mm.modules.Snapshot();
	if( snap==null ) {
		return -1;
	}
	
	int len = snap.Length;
	int has_group;
	for( int i; i < len; i++ ) {
		int keysize = snap.KeyBufferSize(i) + 1;
		char[] name = new char[keysize];
		snap.GetKey(i, name, keysize);
		
		PluginModule pl;
		if( !mm.modules.GetArray(name, pl, sizeof(pl)) ) {
			break;
		} else if( pl.group==group ) {
			modules[has_group++] = pl;
		}
	}
	delete snap;
	return has_group;
}

stock int LibModuleManager_GetModulesByFlags(ManagerID id, PluginModule[] modules, int flags) {
	ModuleManager mm;
	if( !LibModuleManager_GetModuleManager(id, mm) ) {
		return -1;
	}
	
	StringMapSnapshot snap = mm.modules.Snapshot();
	if( snap==null ) {
		return -1;
	}
	
	int len = snap.Length;
	int has_flags;
	for( int i; i < len; i++ ) {
		int keysize = snap.KeyBufferSize(i) + 1;
		char[] name = new char[keysize];
		snap.GetKey(i, name, keysize);
		
		PluginModule pl;
		if( !mm.modules.GetArray(name, pl, sizeof(pl)) ) {
			break;
		} else if( pl.flags==flags ) {
			modules[has_flags++] = pl;
		}
	}
	delete snap;
	return has_flags;
}


/**
 * Let's you call a forward in a linear fashion.
 * to execute a Global  fwd, pass 0 as the manager id.
 * to execute a Private fwd, pass appropriate manager id.
 * 
 * CAN'T BE DONE.
 */
//native bool LibModuleManager_ExecForward(ManagerID id, const char[] name, any &result=0, ...);



/// Shared Map.
/// For plugins and their systems be able to share data.
/// Besides having additional data to set, SharedMaps can LOCK specific props.
/// thus preventing non-owning plugins from removing and/or changing the data.
/// for SPType, here's the list:
///     AnyType, CharType, IntType, FloatType, HandleType,
///     FuncType, EnumType, MethodMapType, BoolType,
methodmap SharedMap {
	public native SharedMap(const char[] channel);
	
	public native bool   GetInt(const char[] prop, int &i);
	public native bool   GetFloat(const char[] prop, float &f);
	public native bool   GetAny(const char[] prop, any &a, SPType sp_type=AnyType);
	
	/// -1 on error. returns 'len' on success.
	public native int    GetStrLen(const char[] prop);
	public native int    GetStr(const char[] prop, char[] buf, int len);
	public native int    GetArrLen(const char[] prop);
	public native int    GetArr(const char[] prop, any[] buf, int len, SPType sp_type=AnyType);
	
	/// gets owner who set the property.
	/// can be null if owner was unloaded/crashed.
	public native Handle GetOwner(const char[] prop);
	
	public native bool   SetInt(const char[] prop, int value);
	public native bool   SetFloat(const char[] prop, float value);
	public native bool   SetAny(const char[] prop, any value, SPType sp_type=AnyType);
	public native bool   SetStr(const char[] prop, const char[] value);
	public native bool   SetArr(const char[] prop, const any[] value, int len, SPType sp_type=AnyType);
	
	public native bool   Has(const char[] prop);
	public native SPType TypeOf(const char[] prop);
	public native bool   Delete(const char[] prop);
	
	/// Locked properties cannot be deleted except by the plugin who set the property first.
	/// -1 if error, 0 if not locked, 1 if locked.
	public native int    IsLocked(const char[] prop);
	/// Frozen properties cannot be changed/modified/set except by the plugin who set the property first.
	/// -1 if error, 0 if not frozen, 1 if frozen.
	public native int    IsFrozen(const char[] prop);
	
	/// NOTE: a prop with no owner cannot be [un]locked or [un]frozen.
	public native bool   Lock(const char[] prop);
	public native bool   Unlock(const char[] prop);
	public native bool   Freeze(const char[] prop);
	public native bool   Unfreeze(const char[] prop);
	
	public bool Protect(const char[] prop) {
		return this.Lock(prop) && this.Freeze(prop);
	}
	
	public bool Unprotect(const char[] prop) {
		return this.Unlock(prop) && this.Unfreeze(prop);
	}
	
	/// -1 if error.
	property int Len {
		public native get();
	}
};


native bool LibModuleManager_DestroySharedMap(const char[] channel);
native bool LibModuleManager_ClearSharedMap(const char[] channel);
native bool LibModuleManager_ChannelExists(const char[] channel);


public SharedPlugin __pl_libmodulemanager = {
	name = "LibModuleManager",
	file = "libmodulemanager.smx",
#if defined REQUIRE_PLUGIN
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_PLUGIN
public void __pl_libmodulemanager_SetNTVOptional() {
	MarkNativeAsOptional("LibModuleManager_GetGlobalFwdsManager");
	
	MarkNativeAsOptional("LibModuleManager_MakePrivateFwdsManager");
	MarkNativeAsOptional("LibModuleManager_GetPrivateFwdsManager");
	MarkNativeAsOptional("LibModuleManager_PrivateFwdHook");
	MarkNativeAsOptional("LibModuleManager_PrivateFwdUnhook");
	MarkNativeAsOptional("LibModuleManager_PrivateFwdUnhookAll");
	//MarkNativeAsOptional("LibModuleManager_ExecForward");
	
	MarkNativeAsOptional("LibModuleManager_MakeModuleManager");
	MarkNativeAsOptional("LibModuleManager_GetModuleManager");
	MarkNativeAsOptional("LibModuleManager_RegisterModule");
	MarkNativeAsOptional("LibModuleManager_UnregisterModule");
	
	
	MarkNativeAsOptional("SharedMap.SharedMap");
	
	MarkNativeAsOptional("SharedMap.GetInt");
	MarkNativeAsOptional("SharedMap.GetFloat");
	MarkNativeAsOptional("SharedMap.GetAny");
	MarkNativeAsOptional("SharedMap.GetStrLen");
	MarkNativeAsOptional("SharedMap.GetStr");
	MarkNativeAsOptional("SharedMap.GetArrLen");
	MarkNativeAsOptional("SharedMap.GetArr");
	MarkNativeAsOptional("SharedMap.GetOwner");
	
	MarkNativeAsOptional("SharedMap.SetInt");
	MarkNativeAsOptional("SharedMap.SetFloat");
	MarkNativeAsOptional("SharedMap.SetAny");
	MarkNativeAsOptional("SharedMap.SetStr");
	MarkNativeAsOptional("SharedMap.SetArr");
	
	MarkNativeAsOptional("SharedMap.Has");
	MarkNativeAsOptional("SharedMap.TypeOf");
	MarkNativeAsOptional("SharedMap.Delete");
	
	MarkNativeAsOptional("SharedMap.IsLocked");
	MarkNativeAsOptional("SharedMap.IsFrozen");
	
	MarkNativeAsOptional("SharedMap.Lock");
	MarkNativeAsOptional("SharedMap.Unlock");
	MarkNativeAsOptional("SharedMap.Freeze");
	MarkNativeAsOptional("SharedMap.Unfreeze");
	MarkNativeAsOptional("SharedMap.Len.get");
	
	MarkNativeAsOptional("LibModuleManager_DestroySharedMap");
	MarkNativeAsOptional("LibModuleManager_ClearSharedMap");
	MarkNativeAsOptional("LibModuleManager_ChannelExists");
}
#endif
