/**
 * libmodsys.inc
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
#if defined _libmodsys_included
	#endinput
#endif
#define _libmodsys_included

#include <plugin_utils>


/// There's only a single Global Forward Manager so no ID needed.
/// This native will retrieve its data for you.
native bool LibModSys_GetGlobalFwdsManager(/** GlobalFwdsManager */ any[] buf);


/// helper to directly get a GlobalFwd object.
stock bool LibModSys_GetGlobalFwd(const char[] name, GlobalFwd buf) {
	GlobalFwdsManager gfm;
	if( !LibModSys_GetGlobalFwdsManager(gfm) ) {
		return false;
	}
	return gfm.GetFwd(name, buf);
}

stock int LibModSys_NumGlobalFwds() {
	GlobalFwdsManager gfm;
	if( !LibModSys_GetGlobalFwdsManager(gfm) ) {
		return -1;
	}
	return gfm.global_fwds.Size;
}

stock int LibModSys_GlobalFwdFuncCount(const char[] fwd_name) {
	GlobalFwd gf;
	if( !LibModSys_GetGlobalFwd(fwd_name, gf) ) {
		return -1;
	}
	return gf.gf.FunctionCount;
}


/** creates a private forward manager, assigned to an ID.
 * You get the ID whenever you need it.
 * Example usage:
 * ```cs
    /// Making manager + getting its 'handle'.
    ManagerID boss_manager = LibModSys_MakePrivateFwdsManager("configs/my_plugin_folder/forwards.cfg");
    
    /// grabbing private forward.
    PrivateFwd pf;
    LibModSys_GetPrivateFwd(boss_manager, "OnSomethingDoCall", pf);
    
    /// Hooking example.
    pf.Hook(plugin, OnDoCall);
 * ```
 */
native ManagerID LibModSys_MakePrivateFwdsManager(const char[] cfgfile);


/// There can be many different Private Forward Managers.
/// This is to allow a plugin to have different systems
/// use different Private Forward managers for themselves.

/// returns true if valid ID and was able to copy to 'buf'.
/// returns false if invalid ID or copy failed.
native bool LibModSys_GetPrivateFwdsManager(ManagerID id, /** PrivateFwdsManager */ any[] buf);


/// helper to directly get a PrivateFwd object.
stock bool LibModSys_GetPrivateFwd(ManagerID id, const char[] fwd_name, PrivateFwd buf) {
	PrivateFwdsManager pfm;
	if( !LibModSys_GetPrivateFwdsManager(id, pfm) ) {
		return false;
	}
	return pfm.GetFwd(fwd_name, buf);
}


stock int LibModSys_NumPrivateFwds(ManagerID id) {
	PrivateFwdsManager pfm;
	if( !LibModSys_GetPrivateFwdsManager(id, pfm) ) {
		return -1;
	}
	return pfm.private_fwds.Size;
}

stock int LibModSys_PrivateFwdFuncCount(ManagerID id, const char[] fwd_name) {
	PrivateFwd pf;
	if( !LibModSys_GetPrivateFwd(id, fwd_name, pf) ) {
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
native bool LibModSys_PrivateFwdHook(ManagerID id, const char[] fwd_name, Function f);
native bool LibModSys_PrivateFwdUnhook(ManagerID id, const char[] fwd_name, Function f);
native bool LibModSys_PrivateFwdUnhookAll(ManagerID id, const char[] fwd_name);


/// This is for making a general plugin manager
/// that is unrelated with private or global forwards.
/// can be used with them though!
native ManagerID LibModSys_MakeModuleManager(const char[] cfgfile);


/// self explanatory, gets the module manager via an id.
/// returns false if unable to get module manager for any reason.
native bool LibModSys_GetModuleManager(ManagerID id, /** ModuleManager */ any[] buf);


/// calling plugin will be [un]registered to a specific plugin manager via id.
/// `name` is for identifying the plugin by a specific name.
native bool LibModSys_RegisterModule(
	ManagerID id, const char[] name,
	int flags=0, int priority=0, int component=0, int group=0
);
native bool LibModSys_UnregisterModule(ManagerID id, const char[] name);


stock bool LibModSys_GetModule(ManagerID id, const char[] module_name, PluginModule buf) {
	ModuleManager mm;
	if( !LibModSys_GetModuleManager(id, mm) ) {
		return false;
	}
	return mm.GetModule(module_name, buf);
}

stock Function LibModSys_GetModuleFunc(ManagerID id, const char[] module_name, const char[] func_name) {
	PluginModule buf;
	if( !LibModSys_GetModule(id, module_name, buf) ) {
		return INVALID_FUNCTION;
	}
	return buf.GetFuncByName(func_name);
}

stock Handle LibModSys_GetModuleHandle(ManagerID id, const char[] module_name) {
	PluginModule buf;
	if( !LibModSys_GetModule(id, module_name, buf) ) {
		return null;
	}
	return buf.plugin;
}


stock bool LibModSys_SetModuleAttribs(ManagerID id, const char[] module_name, int flags=0, int priority=0, int component=0, int group=0) {
	ModuleManager mm;
	if( !LibModSys_GetModuleManager(id, mm) ) {
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


stock int LibModSys_NumModules(ManagerID id) {
	ModuleManager mm;
	if( !LibModSys_GetModuleManager(id, mm) ) {
		return -1;
	}
	return mm.modules.Size;
}

/**
 * First use `LibModSys_NumModules` to get the length.
 * then allocate an array like:
 * PluginModule[] modules = new PluginModule[len];
 * Then pass it to this function to sort out.
 */
stock bool LibModSys_GetModulesByPriority(ManagerID id, PluginModule[] modules, int num_modules) {
	ModuleManager mm;
	if( !LibModSys_GetModuleManager(id, mm) ) {
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

stock int LibModSys_GetModulesByComponent(ManagerID id, PluginModule[] modules, int component) {
	ModuleManager mm;
	if( !LibModSys_GetModuleManager(id, mm) ) {
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

stock int LibModSys_GetModulesByGroup(ManagerID id, PluginModule[] modules, int group) {
	ModuleManager mm;
	if( !LibModSys_GetModuleManager(id, mm) ) {
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

stock int LibModSys_GetModulesByFlags(ManagerID id, PluginModule[] modules, int flags) {
	ModuleManager mm;
	if( !LibModSys_GetModuleManager(id, mm) ) {
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
//native bool LibModSys_ExecForward(ManagerID id, const char[] name, any &result=0, ...);




/// Shared Map.
/// For plugins and their systems be able to share data.
/// Besides having additional data to set, SharedMaps can LOCK specific props.

/// NOTE:: ALL CREATED PROPS ARE FROZEN & LOCKED BY DEFAULT. OWNER/PROP CREATOR MUST UNFREEZE/UNLOCK THEM.

/// thus preventing non-owning plugins from removing and/or changing the data.
/// for SPType, here's the list:
///     AnyType, CharType, IntType, FloatType, HandleType,
///     FuncType, EnumType, MethodMapType, BoolType,
methodmap SharedMap {
	public native SharedMap(const char[] channel);
	
	public native bool     GetInt(const char[] prop, int &i);
	public native bool     GetFloat(const char[] prop, float &f);
	public native bool     GetAny(const char[] prop, any &a, SPType sp_type=AnyType);
	
	/// -1 on error. returns 'len' on success.
	public native int      GetStrLen(const char[] prop);
	public native int      GetStr(const char[] prop, char[] buf, int len);
	public native int      GetArrLen(const char[] prop);
	public native int      GetArr(const char[] prop, any[] buf, int len, SPType sp_type=AnyType);
	//public native int      GetArrIdxVal(const char[] prop, int index, any &buf, SPType sp_type=AnyType);
	
	/// gets owner who set the property.
	/// can be null if owner was unloaded/crashed.
	public native Handle   GetOwner(const char[] prop);
	
	public native bool     SetInt(const char[] prop, int value);
	public native bool     SetFloat(const char[] prop, float value);
	public native bool     SetAny(const char[] prop, any value, SPType sp_type=AnyType);
	public native bool     SetStr(const char[] prop, const char[] value);
	public native bool     SetArr(const char[] prop, const any[] value, int len, SPType sp_type=AnyType);
	//public native bool     SetArrIdxVal(const char[] prop, int index, any buf, SPType sp_type=AnyType);
	
	/// Useful for not having to make natives.
	public native bool     SetFunc(const char[] prop, Function f, int num_args);
	public native Function GetFunc(const char[] prop);
	
	/**
	 * all args are passed by reference.
	 * use this like .ExecFunc("f", "iii", a, b, c);
	 * Format:
	 * i - int/bool/Handle/etc.
	 * f - float
	 * s - const char[] string - doesn't need 'l' length int.
	 * a - const any[] array
	 * l - length int for an array or string. NOTE:: You NEED an 'a' EXACTLY after the 'l' like: "la"
	 * p - const int[] client userids. doesn't need 'l' length int, uses MaxClients int.
	 * 
	 * I - int&/bool&
	 * F - float&
	 * A - any[] - only allowed ONE buffer array per function.
	 * S - char[] - only allowed ONE buffer string per function.
	 * P - int[] client userids. doesn't need 'l' length int, uses MaxClients int.
	 * 
	 * 
	 * NO WHITESPACE OR ANY OTHER CHARACTERS.
	 * 
	 */
	public native bool     ExecFunc(const char[] prop, const char[] arg_fmt, any &ret=0, any ...);
	
	public native bool     Has(const char[] prop);
	public native SPType   TypeOf(const char[] prop);
	public native bool     Delete(const char[] prop);
	
	/// Locked properties cannot be deleted except by the plugin who set the property first.
	/// -1 if error, 0 if not locked, 1 if locked.
	public native int      IsLocked(const char[] prop);
	/// Frozen properties cannot be changed/modified/set except by the plugin who set the property first.
	/// -1 if error, 0 if not frozen, 1 if frozen.
	public native int      IsFrozen(const char[] prop);
	
	/// NOTE: a prop with no owner cannot be [un]locked or [un]frozen.
	public native bool     Lock(const char[] prop);
	public native bool     Unlock(const char[] prop);
	public native bool     Freeze(const char[] prop);
	public native bool     Unfreeze(const char[] prop);
	
	public bool Protect(const char[] prop) {
		return this.Lock(prop) && this.Freeze(prop);
	}
	
	public bool Unprotect(const char[] prop) {
		return this.Unlock(prop) && this.Unfreeze(prop);
	}
	
	/// integer-based key methods.
	public bool IntGetInt(int prop, int &i) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.GetInt(key, i);
	}
	public bool IntGetFloat(int prop, float &f) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.GetFloat(key, f);
	}
	public bool IntGetAny(int prop, any &a, SPType sp_type=AnyType) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.GetAny(key, a, sp_type);
	}
	
	public int IntGetStrLen(int prop) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.GetStrLen(key);
	}
	public int IntGetStr(int prop, char[] buf, int len) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.GetStr(key, buf, len);
	}
	public int IntGetArrLen(int prop) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.GetArrLen(key);
	}
	public int IntGetArr(int prop, any[] buf, int len, SPType sp_type=AnyType) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.GetArr(key, buf, len, sp_type);
	}
	/*
	public bool IntGetArrIdxVal(int prop, int index, any &buf, SPType sp_type=AnyType) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.GetArrIdxVal(key, index, buf, sp_type);
	}
	*/
	
	public Handle IntGetOwner(int prop) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.GetOwner(key);
	}
	
	public bool IntSetInt(int prop, int value) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.SetInt(key, value);
	}
	public bool IntSetFloat(int prop, float value) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.SetFloat(key, value);
	}
	public bool IntSetAny(int prop, any value, SPType sp_type=AnyType) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.SetAny(key, value, sp_type);
	}
	public bool IntSetStr(int prop, const char[] value) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.SetStr(key, value);
	}
	public bool IntSetArr(int prop, const any[] value, int len, SPType sp_type=AnyType) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.SetArr(key, value, len, sp_type);
	}
	
	public bool IntSetFunc(int prop, Function f, int num_args) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.SetFunc(key, f, num_args);
	}
	public Function IntGetFunc(int prop) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.GetFunc(key);
	}
	
	public bool IntHas(int prop) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.Has(key);
	}
	public SPType IntTypeOf(int prop) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.TypeOf(key);
	}
	public bool IntDelete(int prop) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.Delete(key);
	}
	
	public int IntIsLocked(int prop) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.IsLocked(key);
	}
	public int IntIsFrozen(int prop) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.IsFrozen(key);
	}
	
	public bool IntLock(int prop) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.Lock(key);
	}
	public bool IntUnlock(int prop) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.Unlock(key);
	}
	public bool IntFreeze(int prop) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.Freeze(key);
	}
	public bool IntUnfreeze(int prop) {
		char key[CELL_KEY_SIZE]; PackCellToStr(prop, key);
		return this.Unfreeze(key);
	}
	
	public bool IntProtect(int prop) {
		return this.IntLock(prop) && this.IntFreeze(prop);
	}
	
	public bool IntUnprotect(int prop) {
		return this.IntUnlock(prop) && this.IntUnfreeze(prop);
	}
	
	/// -1 if error.
	property int Len {
		public native get();
	}
};


native bool LibModSys_DestroySharedMap(const char[] channel);
native bool LibModSys_ClearSharedMap(const char[] channel);
native bool LibModSys_ChannelExists(const char[] channel);


public SharedPlugin __pl_libmodsys = {
	name = "LibModSys",
	file = "libmodsys.smx",
#if defined REQUIRE_PLUGIN
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_PLUGIN
public void __pl_libmodsys_SetNTVOptional() {
	MarkNativeAsOptional("LibModSys_GetGlobalFwdsManager");
	
	MarkNativeAsOptional("LibModSys_MakePrivateFwdsManager");
	MarkNativeAsOptional("LibModSys_GetPrivateFwdsManager");
	MarkNativeAsOptional("LibModSys_PrivateFwdHook");
	MarkNativeAsOptional("LibModSys_PrivateFwdUnhook");
	MarkNativeAsOptional("LibModSys_PrivateFwdUnhookAll");
	//MarkNativeAsOptional("LibModSys_ExecForward");
	
	MarkNativeAsOptional("LibModSys_MakeModuleManager");
	MarkNativeAsOptional("LibModSys_GetModuleManager");
	MarkNativeAsOptional("LibModSys_RegisterModule");
	MarkNativeAsOptional("LibModSys_UnregisterModule");
	
	
	MarkNativeAsOptional("SharedMap.SharedMap");
	
	MarkNativeAsOptional("SharedMap.GetInt");
	MarkNativeAsOptional("SharedMap.GetFloat");
	MarkNativeAsOptional("SharedMap.GetAny");
	MarkNativeAsOptional("SharedMap.GetStrLen");
	MarkNativeAsOptional("SharedMap.GetStr");
	MarkNativeAsOptional("SharedMap.GetArrLen");
	MarkNativeAsOptional("SharedMap.GetArr");
	//MarkNativeAsOptional("SharedMap.GetArrIdxVal");
	MarkNativeAsOptional("SharedMap.GetOwner");
	
	MarkNativeAsOptional("SharedMap.SetInt");
	MarkNativeAsOptional("SharedMap.SetFloat");
	MarkNativeAsOptional("SharedMap.SetAny");
	MarkNativeAsOptional("SharedMap.SetStr");
	MarkNativeAsOptional("SharedMap.SetArr");
	
	MarkNativeAsOptional("SharedMap.SetFunc");
	MarkNativeAsOptional("SharedMap.GetFunc");
	MarkNativeAsOptional("SharedMap.ExecFunc");
	
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
	
	MarkNativeAsOptional("LibModSys_DestroySharedMap");
	MarkNativeAsOptional("LibModSys_ClearSharedMap");
	MarkNativeAsOptional("LibModSys_ChannelExists");
}
#endif
