/**
 * plugin_utils.inc
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

#if defined _plugin_utils_included
	#endinput
#endif
#define _plugin_utils_included

#include <sdktools>
#include <cfgmap>


enum { /** some basic, commonly used "defines" */
	MAX_FWD_PARAMS = 32,
	BASE_PATH      = 64,
	OS_MAX_PATH    = PLATFORM_MAX_PATH,
	ENTITY_LIMIT   = 2049,
};


enum SPType { /// Type information for sending to modules.
	InvalidType,
	AnyType,  CharType, IntType,       FloatType, HandleType,
	FuncType, EnumType, MethodMapType, BoolType,  VecType,
	ArrayType = 1 << 4, /// 16
	RefType   = 1 << 5, /// 32
	/// CharType   | ArrayType == char[].
	/// IntType    | ArrayType == int[].
	/// FloatType  | ArrayType == float[].
	/// HandleType | ArrayType == Handle[].
	/// AnyType    | ArrayType == any[]/enum struct.
	/// FuncType   | ArrayType == function[].
};

enum ManagerID {
	InvalidManagerID = 0,
};


stock int AnyToInt(any a) {
	return a;
}

stock any IntToAny(int i) {
	return i;
}


enum struct FuncObj {
	Function fn;
	//Handle owner; /// owner plugin.
}

stock any[] MakeFuncObj(Function fn=INVALID_FUNCTION) {
	FuncObj f; f.fn = fn;
	return f;
}


enum struct Callable {
	bool in_call_setup;
	
	bool StartFunc(Handle plugin, const char[] name) {
		Function f = GetFunctionByName(plugin, name);
		if( f==INVALID_FUNCTION || this.in_call_setup ) {
			return false;
		}
		Call_StartFunction(plugin, f);
		this.in_call_setup = true;
		return true;
	}
	bool StartFunction(Handle plugin, Function f) {
		if( f==INVALID_FUNCTION || this.in_call_setup ) {
			return false;
		}
		Call_StartFunction(plugin, f);
		this.in_call_setup = true;
		return true;
	}
	bool StartFwd(Handle fwd) {
		if( fwd==null || this.in_call_setup ) {
			return false;
		}
		Call_StartForward(fwd);
		this.in_call_setup = true;
		return true;
	}
	bool Cancel() {
		if( !this.in_call_setup ) {
			return false;
		}
		Call_Cancel();
		this.in_call_setup = false;
		return true;
	}
	bool Finish(any &result=0) {
		if( !this.in_call_setup ) {
			return false;
		}
		bool res = Call_Finish(result)==SP_ERROR_NONE;
		this.in_call_setup = false;
		return res;
	}
	
	bool PushArray(any[] value, int size, bool copyback=true) {
		if( !this.in_call_setup ) {
			return false;
		}
		Call_PushArrayEx(value, size, copyback? SM_PARAM_COPYBACK : 0);
		return true;
	}
	/**
	 * SM_PARAM_STRING_UTF8    (1<<0)      < String should be UTF-8 handled
	 * SM_PARAM_STRING_BINARY  (1<<2)      < Treat the string as a binary string
	 */
	bool PushString(char[] value, int size, int flags=0, bool copyback=true) {
		if( !this.in_call_setup ) {
			return false;
		}
		Call_PushStringEx(value, size, SM_PARAM_STRING_COPY | flags, copyback? SM_PARAM_COPYBACK : 0);
		return true;
	}
	bool PushNullString() {
		if( !this.in_call_setup ) {
			return false;
		}
		Call_PushNullString();
		return true;
	}
	bool PushNullVec3() {
		if( !this.in_call_setup ) {
			return false;
		}
		Call_PushNullVector();
		return true;
	}
	bool PushCell(any value) {
		if( !this.in_call_setup ) {
			return false;
		}
		Call_PushCell(value);
		return true;
	}
	bool PushCellRef(any &value) {
		if( !this.in_call_setup ) {
			return false;
		}
		Call_PushCellRef(value);
		return true;
	}
	bool PushFloat(float value) {
		if( !this.in_call_setup ) {
			return false;
		}
		Call_PushFloat(value);
		return true;
	}
	bool PushFloatRef(float &value) {
		if( !this.in_call_setup ) {
			return false;
		}
		Call_PushFloatRef(value);
		return true;
	}
}


/// Use a priority queue for ordered single forward calls?
enum struct PluginModule {
	Handle plugin;
	int    flags;
	int    priority;
	int    component;
	int    group;
	
	
	Function GetFuncByName(const char[] name) {
		return GetFunctionByName(this.plugin, name);
	}
	
	/*
	bool AddFuncToFwd(PrivateForward pf, const char[] name) {
		Function f = this.GetFuncByName(name);
		return( f==INVALID_FUNCTION )? false : pf.AddFunction(this.plugin, f);
	}
	
	bool DelFuncFromFwd(PrivateForward pf, const char[] name) {
		Function f = this.GetFuncByName(name);
		return( f==INVALID_FUNCTION )? false : pf.RemoveFunction(this.plugin, f);
	}
	
	int ClearFuncsFromFwd(PrivateForward pf) {
		return pf.RemoveAllFunctions(this.plugin);
	}
	*/
}
stock any[] MakePluginModule(Handle plugin=null, int component=0, int priority=0, int group=0, int flags=0) {
	PluginModule module;
	module.plugin    = plugin;
	module.group     = group;
	module.priority  = priority;
	module.flags     = flags;
	module.component = component;
	return module;
}


stock bool IsValidModuleName(const char[] name) {
	int len = strlen(name);
	for( int i; i < len; i++ ) {
		int holder = name[i];
		/// Invalid name, names may only contain numbers, underscores, and normal letters.
		if( !( IsCharAlpha(holder) || IsCharNumeric(holder) || holder == '_' ) ) {
			return false;
		}
	}
	/// A name is, of course, only valid if it's 1 or more chars long, though longer is recommended
	return len > 0;
}


/**
 * Purpose is to use with this a forward map 'map[string]Forward'
 * Purpose is to allow addons to register their own events of which they fire across hooked addons/bosses.
 * RegisterEvent() native.
 * Pass ConfigMap Handle to events.
 */
stock PrivateForward MakePrivateForward(ExecType et, const ParamType[] parmtypes, int num_params) {
	/// forwards are only allowed up to 32 params.
	if( num_params < 0 ) {
		num_params = 0;
	} else if( num_params > MAX_FWD_PARAMS ) {
		num_params = MAX_FWD_PARAMS;
	}
	
	switch( num_params ) {
		case 32: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19], parmtypes[20], parmtypes[21], parmtypes[22], parmtypes[23], parmtypes[24], parmtypes[25], parmtypes[26], parmtypes[27], parmtypes[28], parmtypes[29], parmtypes[30], parmtypes[31]);
		}
		case 31: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19], parmtypes[20], parmtypes[21], parmtypes[22], parmtypes[23], parmtypes[24], parmtypes[25], parmtypes[26], parmtypes[27], parmtypes[28], parmtypes[29], parmtypes[30]);
		}
		case 30: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19], parmtypes[20], parmtypes[21], parmtypes[22], parmtypes[23], parmtypes[24], parmtypes[25], parmtypes[26], parmtypes[27], parmtypes[28], parmtypes[29]);
		}
		case 29: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19], parmtypes[20], parmtypes[21], parmtypes[22], parmtypes[23], parmtypes[24], parmtypes[25], parmtypes[26], parmtypes[27], parmtypes[28]);
		}
		case 28: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19], parmtypes[20], parmtypes[21], parmtypes[22], parmtypes[23], parmtypes[24], parmtypes[25], parmtypes[26], parmtypes[27]);
		}
		case 27: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19], parmtypes[20], parmtypes[21], parmtypes[22], parmtypes[23], parmtypes[24], parmtypes[25], parmtypes[26]);
		}
		case 26: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19], parmtypes[20], parmtypes[21], parmtypes[22], parmtypes[23], parmtypes[24], parmtypes[25]);
		}
		case 25: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19], parmtypes[20], parmtypes[21], parmtypes[22], parmtypes[23], parmtypes[24]);
		}
		case 24: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19], parmtypes[20], parmtypes[21], parmtypes[22], parmtypes[23]);
		}
		case 23: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19], parmtypes[20], parmtypes[21], parmtypes[22]);
		}
		case 22: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19], parmtypes[20], parmtypes[21]);
		}
		case 21: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19], parmtypes[20]);
		}
		case 20: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19]);
		}
		case 19: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18]);
		}
		case 18: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17]);
		}
		case 17: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16]);
		}
		case 16: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15]);
		}
		case 15: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14]);
		}
		case 14: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13]);
		}
		case 13: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12]);
		}
		case 12: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11]);
		}
		case 11: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10]);
		}
		case 10: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9]);
		}
		case 9: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8]);
		}
		case 8: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7]);
		}
		case 7: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6]);
		}
		case 6: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5]);
		}
		case 5: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4]);
		}
		case 4: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3]);
		}
		case 3: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1], parmtypes[2]);
		}
		case 2: {
			return new PrivateForward(et, parmtypes[0], parmtypes[1]);
		}
		case 1: {
			return new PrivateForward(et, parmtypes[0]);
		}
		case 0: {
			return new PrivateForward(et);
		}
	}
	return null;
}

stock GlobalForward MakeGlobalForward(const char[] name, ExecType et, const ParamType[] parmtypes, int num_params) {
	/// forwards are only allowed up to 32 params.
	if( num_params < 0 ) {
		num_params = 0;
	} else if( num_params > MAX_FWD_PARAMS ) {
		num_params = MAX_FWD_PARAMS;
	}
	
	switch( num_params ) {
		case 32: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19], parmtypes[20], parmtypes[21], parmtypes[22], parmtypes[23], parmtypes[24], parmtypes[25], parmtypes[26], parmtypes[27], parmtypes[28], parmtypes[29], parmtypes[30], parmtypes[31]);
		}
		case 31: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19], parmtypes[20], parmtypes[21], parmtypes[22], parmtypes[23], parmtypes[24], parmtypes[25], parmtypes[26], parmtypes[27], parmtypes[28], parmtypes[29], parmtypes[30]);
		}
		case 30: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19], parmtypes[20], parmtypes[21], parmtypes[22], parmtypes[23], parmtypes[24], parmtypes[25], parmtypes[26], parmtypes[27], parmtypes[28], parmtypes[29]);
		}
		case 29: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19], parmtypes[20], parmtypes[21], parmtypes[22], parmtypes[23], parmtypes[24], parmtypes[25], parmtypes[26], parmtypes[27], parmtypes[28]);
		}
		case 28: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19], parmtypes[20], parmtypes[21], parmtypes[22], parmtypes[23], parmtypes[24], parmtypes[25], parmtypes[26], parmtypes[27]);
		}
		case 27: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19], parmtypes[20], parmtypes[21], parmtypes[22], parmtypes[23], parmtypes[24], parmtypes[25], parmtypes[26]);
		}
		case 26: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19], parmtypes[20], parmtypes[21], parmtypes[22], parmtypes[23], parmtypes[24], parmtypes[25]);
		}
		case 25: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19], parmtypes[20], parmtypes[21], parmtypes[22], parmtypes[23], parmtypes[24]);
		}
		case 24: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19], parmtypes[20], parmtypes[21], parmtypes[22], parmtypes[23]);
		}
		case 23: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19], parmtypes[20], parmtypes[21], parmtypes[22]);
		}
		case 22: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19], parmtypes[20], parmtypes[21]);
		}
		case 21: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19], parmtypes[20]);
		}
		case 20: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18], parmtypes[19]);
		}
		case 19: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17], parmtypes[18]);
		}
		case 18: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16], parmtypes[17]);
		}
		case 17: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15], parmtypes[16]);
		}
		case 16: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14], parmtypes[15]);
		}
		case 15: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13], parmtypes[14]);
		}
		case 14: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12], parmtypes[13]);
		}
		case 13: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11], parmtypes[12]);
		}
		case 12: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10], parmtypes[11]);
		}
		case 11: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9], parmtypes[10]);
		}
		case 10: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8], parmtypes[9]);
		}
		case 9: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7], parmtypes[8]);
		}
		case 8: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6], parmtypes[7]);
		}
		case 7: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5], parmtypes[6]);
		}
		case 6: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4], parmtypes[5]);
		}
		case 5: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3], parmtypes[4]);
		}
		case 4: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2], parmtypes[3]);
		}
		case 3: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1], parmtypes[2]);
		}
		case 2: {
			return new GlobalForward(name, et, parmtypes[0], parmtypes[1]);
		}
		case 1: {
			return new GlobalForward(name, et, parmtypes[0]);
		}
		case 0: {
			return new GlobalForward(name, et);
		}
	}
	return null;
}


enum struct GlobalFwd {
	GlobalForward  gf;
	ParamType      param_type[MAX_FWD_PARAMS];
	int            param_count;
	ExecType       exec_type;
	Callable       callable;
	
	
	int Count() {
		return this.gf.FunctionCount;
	}
	
	bool Start() {
		return this.callable.StartFwd(this.gf);
	}
	bool Cancel() {
		return this.callable.Cancel();
	}
	
	bool PushArray(any[] value, int size, bool copyback=true) {
		return this.callable.PushArray(value, size, copyback);
	}
	bool PushString(char[] value, int size, int flags=0, bool copyback=true) {
		return this.callable.PushString(value, size, flags, copyback);
	}
	bool PushNullString() {
		return this.callable.PushNullString();
	}
	bool PushNullVec3() {
		return this.callable.PushNullVec3();
	}
	bool PushCell(any value) {
		return this.callable.PushCell(value);
	}
	bool PushCellRef(any &value) {
		return this.callable.PushCellRef(value);
	}
	bool PushFloat(float value) {
		return this.callable.PushFloat(value);
	}
	bool PushFloatRef(float &value) {
		return this.callable.PushFloatRef(value);
	}
	bool Finish(any &result=0) {
		return this.callable.Finish(result);
	}
}

enum struct PrivateFwd {
	PrivateForward pf;
	ParamType      param_type[MAX_FWD_PARAMS];
	int            param_count;
	ExecType       exec_type;
	Callable       callable;
	
	
	bool Hook(Handle plugin, Function f) {
		if( this.pf==null || plugin==null || f==INVALID_FUNCTION ) {
			return false;
		}
		return this.pf.AddFunction(plugin, f);
	}
	bool Unhook(Handle plugin, Function f) {
		if( this.pf==null || plugin==null || f==INVALID_FUNCTION ) {
			return false;
		}
		return this.pf.RemoveFunction(plugin, f);
	}
	bool UnhookAll(Handle plugin) {
		if( this.pf==null || plugin==null ) {
			return false;
		}
		return this.pf.RemoveAllFunctions(plugin) > 0;
	}
	
	/// this would require delaying func adding when waiting for hooks.
	/*
	void AddFuncByNameFromModules(const char[] name, PluginModule[] modules, int num_modules, bool priority_sort=false) {
		if( priority_sort ) {
			for( int i; i < num_modules; i++ ) {
				for( int j; j < num_modules; j++ ) {
					if( i==j ) {
						continue;
					} else if( modules[j].priority < modules[i].priority ) {
						/// highest priority is first index.
						PluginModule tmp; tmp = modules[i];
						modules[i] = modules[j];
						modules[j] = tmp;
					}
				}
			}
		}
		for( int i; i < num_modules; i++ ) {
			Function f = modules[i].GetFuncByName(name);
			this.Hook(modules[i].plugin, f);
		}
	}
	*/
	
	int Count() {
		return this.pf.FunctionCount;
	}
	
	bool Start() {
		return this.callable.StartFwd(this.pf);
	}
	bool Cancel() {
		return this.callable.Cancel();
	}
	
	bool PushArray(any[] value, int size, bool copyback=true) {
		return this.callable.PushArray(value, size, copyback);
	}
	bool PushString(char[] value, int size, int flags=0, bool copyback=true) {
		return this.callable.PushString(value, size, flags, copyback);
	}
	bool PushNullString() {
		return this.callable.PushNullString();
	}
	bool PushNullVec3() {
		return this.callable.PushNullVec3();
	}
	bool PushCell(any value) {
		return this.callable.PushCell(value);
	}
	bool PushCellRef(any &value) {
		return this.callable.PushCellRef(value);
	}
	bool PushFloat(float value) {
		return this.callable.PushFloat(value);
	}
	bool PushFloatRef(float &value) {
		return this.callable.PushFloatRef(value);
	}
	bool Finish(any &result=0) {
		return this.callable.Finish(result);
	}
}


enum struct GlobalFwdsManager {
	ConfigMap global_fwd_cfg;
	StringMap global_fwds;    /// map[string]GlobalFwd
	
	
	void Init(const char[] cfgfile) {
		this.global_fwd_cfg = new ConfigMap(cfgfile);
		this.global_fwds    = new StringMap();
		if( this.global_fwd_cfg==null ) {
			LogMessage("GlobalFwdsManager :: Warning: **** cfg file '%s' is null!. ****", cfgfile);
		}
		this.SetupGlobalFwds();
	}
	
	void Print() {
		StringMapSnapshot snap = this.global_fwds.Snapshot();
		if( snap==null ) {
			return;
		}
		
		int len = snap.Length;
		for( int i; i < len; i++ ) {
			int keysize = snap.KeyBufferSize(i) + 1;
			char[] fwd_name = new char[keysize];
			snap.GetKey(i, fwd_name, keysize);
			PrintToServer("GlobalFwdsManager :: Forward - '%s'", fwd_name);
		}
		delete snap;
	}
	
	void Destroy() {
		DeleteCfg(this.global_fwd_cfg);
		StringMapSnapshot snap = this.global_fwds.Snapshot();
		if( snap==null ) {
			return;
		}
		
		int len = snap.Length;
		for( int i; i < len; i++ ) {
			int keysize = snap.KeyBufferSize(i) + 1;
			char[] fwd_name = new char[keysize];
			GlobalFwd gf_buf;
			if( snap.GetKey(i, fwd_name, keysize) > 0 && this.global_fwds.GetArray(fwd_name, gf_buf, sizeof(gf_buf)) ) {
				delete gf_buf.gf;
			}
		}
		delete snap;
		delete this.global_fwds;
	}
	
	bool GetFwd(const char[] name, GlobalFwd buf) {
		return this.global_fwds.GetArray(name, buf, sizeof(buf));
	}
	
	void SetupGlobalFwds() {
		if( this.global_fwd_cfg==null ) {
			LogError("GlobalFwdsManager :: Error: **** null global forward cfg, unable to initialize global forwards! ****");
			return;
		}
		
		ConfigMap glbl_fwds = this.global_fwd_cfg.GetSection("global forwards");
		if( glbl_fwds==null ) {
			LogError("GlobalFwdsManager :: Error: **** missing 'global forwards' root section in cfg! ****");
			return;
		}
		
		int size = glbl_fwds.Size;
		for( int i; i < size; i++ ) {
			ConfigMap glbl_fwd = glbl_fwds.GetIntKeySection(i);
			if( glbl_fwd==null ) {
				LogError("GlobalFwdsManager :: Error: **** in enumerated global forwards section, enum number '%i' is null! ****", i);
				continue;
			}
			
			int name_len = glbl_fwd.GetSize("name");
			char[] fwd_name = new char[name_len + 1];
			glbl_fwd.Get("name", fwd_name, name_len);
			if( this.global_fwds.ContainsKey(fwd_name) ) {
				LogError("GlobalFwdsManager :: Error: **** duplicate global forward name '%s'! skipping... ****", fwd_name);
				continue;
			}
			
			//PrintToServer("GlobalForward Name: '%s' | len: %i", fwd_name, name_len);
			GlobalFwd gfs; /// global forward struct.
			glbl_fwd.GetInt("exectype", view_as< int >(gfs.exec_type));
			//PrintToServer("GlobalForward ExecType: '%i'", gfs.callable.exec_type);
			
			ConfigMap params_list = glbl_fwd.GetSection("params");
			if( params_list != null ) {
				int num_params = params_list.Size;
				if( num_params > MAX_FWD_PARAMS ) {
					num_params = MAX_FWD_PARAMS;
				}
				gfs.param_count = num_params;
				//PrintToServer("GlobalForward Param Count: '%i'", num_params);
				for( int p; p < num_params; p++ ) {
					params_list.GetIntKeyInt(p, view_as< int >(gfs.param_type[p]));
					//PrintToServer("GlobalForward Param Type: '%i'", gfs.callable.param_type[p]);
				}
				gfs.gf = MakeGlobalForward(fwd_name, gfs.exec_type, gfs.param_type, num_params);
			} else {
				gfs.gf = MakeGlobalForward(fwd_name, gfs.exec_type, gfs.param_type, 0);
			}
			
			if( gfs.gf != null ) {
				this.global_fwds.SetArray(fwd_name, gfs, sizeof(gfs));
			}
		}
	}
}


/**
 * A way to have "ordered" execution control is to use
 * multiple private fwd managers using the same forwards
 * but hooked to different plugins or same.
 * 
 * Also provided is a function to hook, all at once,
 * a bunch of plugins to the same function
 * [PROVIDED THEY ALL HAVE THAT FUNCTION]
 * and using their priority numbers to which
 * plugin will execute first in terms of that forward.
 */
enum struct PrivateFwdsManager {
	ConfigMap private_fwd_cfg;
	StringMap private_fwds;    /// map[string]PrivateFwd
	
	
	void Init(const char[] cfgfile) {
		this.private_fwd_cfg = new ConfigMap(cfgfile);
		this.private_fwds    = new StringMap();
		if( this.private_fwd_cfg==null ) {
			LogMessage("PrivateFwdsManager :: Warning: **** cfg file '%s' is null!. ****", cfgfile);
		}
		this.SetupPrivFwds();
	}
	
	void Print() {
		StringMapSnapshot snap = this.private_fwds.Snapshot();
		if( snap==null ) {
			return;
		}
		
		int len = snap.Length;
		for( int i; i < len; i++ ) {
			int keysize = snap.KeyBufferSize(i) + 1;
			char[] fwd_name = new char[keysize];
			snap.GetKey(i, fwd_name, keysize);
			PrintToServer("PrivateFwdsManager :: Forward - '%s'", fwd_name);
		}
		delete snap;
	}
	
	void Destroy() {
		DeleteCfg(this.private_fwd_cfg);
		StringMapSnapshot snap = this.private_fwds.Snapshot();
		if( snap==null ) {
			return;
		}
		
		int len = snap.Length;
		for( int i; i < len; i++ ) {
			int keysize = snap.KeyBufferSize(i) + 1;
			char[] fwd_name = new char[keysize];
			PrivateFwd pf_buf;
			if( snap.GetKey(i, fwd_name, keysize) > 0 && this.private_fwds.GetArray(fwd_name, pf_buf, sizeof(pf_buf)) ) {
				delete pf_buf.pf;
			}
		}
		delete snap;
		delete this.private_fwds;
	}
	
	bool GetFwd(const char[] name, PrivateFwd buf) {
		return this.private_fwds.GetArray(name, buf, sizeof(buf));
	}
	
	void SetupPrivFwds() {
		if( this.private_fwd_cfg==null ) {
			LogError("PrivateFwdsManager :: Error: **** null private forward cfg, unable to initialize private forwards manager! ****");
			return;
		}
		
		ConfigMap priv_fwds = this.private_fwd_cfg.GetSection("private forwards");
		if( priv_fwds==null ) {
			LogError("PrivateFwdsManager :: Error: **** missing 'private forwards' root section in cfg! ****");
			return;
		}
		
		int size = priv_fwds.Size;
		for( int i; i < size; i++ ) {
			ConfigMap priv_fwd = priv_fwds.GetIntKeySection(i);
			if( priv_fwd==null ) {
				LogError("PrivateFwdsManager :: Error: **** in enumerated private forwards section, enum number '%i' is null! ****", i);
				continue;
			}
			
			int name_len = priv_fwd.GetSize("name\\.of");
			char[] fwd_name = new char[name_len + 1];
			priv_fwd.Get("name\\.of", fwd_name, name_len);
			if( this.private_fwds.ContainsKey(fwd_name) ) {
				LogError("PrivateFwdsManager :: Error: **** duplicate private forward name '%s'! skipping... ****", fwd_name);
				continue;
			}
			
			//PrintToServer("PrivateForward Name: '%s' | len: %i", fwd_name, name_len);
			PrivateFwd pfs;
			priv_fwd.GetInt("exectype", view_as< int >(pfs.exec_type));
			//PrintToServer("PrivateForward ExecType: '%i'", pfs.exec_type);
			
			ConfigMap params_list = priv_fwd.GetSection("params");
			if( params_list != null ) {
				int num_params = params_list.Size;
				if( num_params > MAX_FWD_PARAMS ) {
					num_params = MAX_FWD_PARAMS;
				}
				pfs.param_count = num_params;
				//PrintToServer("PrivateForward Param Count: '%i'", num_params);
				for( int p; p < num_params; p++ ) {
					params_list.GetIntKeyInt(p, view_as< int >(pfs.param_type[p]));
					//PrintToServer("PrivateForward Param Type: '%i'", pfs.param_type[p]);
				}
				pfs.pf = MakePrivateForward(pfs.exec_type, pfs.param_type, num_params);
			} else {
				pfs.pf = MakePrivateForward(pfs.exec_type, pfs.param_type, 0);
			}
			
			if( pfs.pf != null ) {
				this.private_fwds.SetArray(fwd_name, pfs, sizeof(pfs));
			}
		}
	}
}


stock bool StrHasPrefix(const char[] pre, const char[] str) {
    return strncmp(pre, str, strlen(pre))==0;
}

enum struct ModuleManager {
	StringMap modules; /// map[string]PluginModule
	ConfigMap cfg;     /// 
	
	
	void Init(const char[] cfgfile) {
		this.modules = new StringMap();
		this.cfg     = new ConfigMap(cfgfile);
		if( this.cfg==null ) {
			LogMessage("ModuleManager :: Warning: **** cfg file '%s' for module manager is null!. ****", cfgfile);
		}
		this.SetupModules();
	}
	
	void Print() {
		StringMapSnapshot snap = this.modules.Snapshot();
		if( snap==null ) {
			return;
		}
		
		int len = snap.Length;
		for( int i; i < len; i++ ) {
			int keysize = snap.KeyBufferSize(i) + 1;
			char[] name = new char[keysize];
			snap.GetKey(i, name, keysize);
			PrintToServer("ModuleManager :: Plugin - '%s'", name);
		}
		delete snap;
	}
	
	void Destroy() {
		DeleteCfg(this.cfg);
		delete this.modules;
	}
	
	bool GetModule(const char[] name, PluginModule buf) {
		return this.modules.GetArray(name, buf, sizeof(buf));
	}
	
	void SetupModules() {
		if( this.cfg==null ) {
			LogError("ModuleManager :: Error: **** null module manager cfg, unable to initialize module manager! ****");
			return;
		}
		
		bool force_registering;
		this.cfg.GetBool("module manager.require registering", force_registering, .simple=false);
		if( force_registering ) {
			//PrintToServer("========== Forcing to register individual modules");
			return;
		}
		
		Handle me = GetMyHandle();
		int prefix_len = this.cfg.GetSize("module manager.add by name prefix");
		char[] prefix_str = new char[prefix_len];
		this.cfg.Get("module manager.add by name prefix", prefix_str, prefix_len);
		//PrintToServer("========== Plugin Prefix: '%s'", prefix_str);
		
		Handle iter = GetPluginIterator();
		do {
			Handle p = ReadPlugin(iter);
			if( p==me ) {
				/// ignore libmodsys.
				continue;
			}
			
			char pl_name[OS_MAX_PATH];
			GetPluginFilename(p, pl_name, sizeof(pl_name));
			
			char smx_ext[] = ".smx";
			ReplaceString(pl_name, sizeof(pl_name), smx_ext, "\0\0\0\0");
			if( prefix_len > 0 && !StrHasPrefix(prefix_str, pl_name) ) {
				//PrintToServer("========== No Prefix here: pl_name: '%s' != prefix_str '%s'", pl_name, prefix_str);
				continue;
			}
			//PrintToServer("========== Got Prefix!: pl_name: '%s' == prefix_str '%s'", pl_name, prefix_str);
			PluginModule pm;
			pm.plugin = p;
			this.modules.SetArray(pl_name, pm, sizeof(pm));
		} while( MorePlugins(iter) );
		delete iter;
	}
}


enum { CELL_KEY_SIZE=6 };
stock void PackCellToStr(any key, char buffer[CELL_KEY_SIZE]) {
	int k = key;
	buffer[0] = ((k >> (7 * 4)) & 0x7F) | 0x80;
	buffer[1] = ((k >> (7 * 3)) & 0x7F) | 0x80;
	buffer[2] = ((k >> (7 * 2)) & 0x7F) | 0x80;
	buffer[3] = ((k >> (7 * 1)) & 0x7F) | 0x80;
	buffer[4] = ((k >> (7 * 0)) & 0x7F) | 0x80;
	buffer[5] = 0x00;
}
stock any StrToPackCell(const char cell_str[CELL_KEY_SIZE]) {
	int k = 0;
	k |= ((cell_str[0] & ~0x80) & 0x7f) << (7 * 4);
	k |= ((cell_str[1] & ~0x80) & 0x7f) << (7 * 3);
	k |= ((cell_str[2] & ~0x80) & 0x7f) << (7 * 2);
	k |= ((cell_str[3] & ~0x80) & 0x7f) << (7 * 1);
	k |= ((cell_str[4] & ~0x80) & 0x7f) << (7 * 0);
	return k;
}


stock bool IsClientValid(int player) {
	return( 0 < player <= MaxClients && IsClientInGame(player) );
}

stock bool IsEntityValid(int entity) {
	return MaxClients < entity < ENTITY_LIMIT && IsValidEntity(entity);
}

stock bool IsCamBot(int client) {
	return IsClientSourceTV(client) || IsClientReplay(client);
}

stock bool IsValidClient(int client, bool replaycheck=true) {
	if( !IsClientValid(client) || GetEntProp(client, Prop_Send, "m_bIsCoaching") != 0 ) {
		return false;
	} else if( replaycheck && IsCamBot(client) ) {
		return false;
	}
	return true;
}


/// useful for cooldowns.
/// Static Time, saves the current `GetGameTime`.
methodmap StaticTime {
	public StaticTime(float val) {
		return view_as< StaticTime >(val);
	}
	public static StaticTime Update() {
		return view_as< StaticTime >( GetGameTime() );
	}
	
	property float Time {
		public get() { return view_as< float >(this); }
	}
	public float Curr() {
		return this.Time;
	}
	public float Elapsed() {
		return GetGameTime() - this.Time;
	}
	public bool WithinTime(float delay_time) {
		return (GetGameTime() - this.Time) <= delay_time;
	}
}

/// Forward Time, saves the current `GetGameTime` + delta value.
methodmap ForwardTime {
	public ForwardTime(float val) {
		return view_as< ForwardTime >(val);
	}
	public static ForwardTime Update(float delta=1.0) {
		return view_as< ForwardTime >(GetGameTime() + delta);
	}
	
	property float Time {
		public get() { return view_as< float >(this); }
	}
	public float Curr() {
		return this.Time;
	}
	public float Elapsed() {
		return this.Time - GetGameTime();
	}
	public bool WithinTime() {
		return GetGameTime() <= this.Time;
	}
}


stock bool IsIntInBounds(int val, int max, int min) {
	return ((val - min) * (max - val)) >= 0;
}
stock int IntClamp(int val, int max, int min) {
	return (val < min)? min : (val > max)? max : val;
}


stock void MakePawnTimer(Function func, float thinktime=0.1, const any[] args, const int len, bool as_array=false) {
	DataPack thinkpack = new DataPack();
	thinkpack.WriteFunction(func);
	thinkpack.WriteCell(len);
	thinkpack.WriteCell(as_array);
	for( int i; i < len; i++ ) {
		thinkpack.WriteCell(args[i]);
	}
	CreateTimer(thinktime, RunPawnFunc, thinkpack, TIMER_DATA_HNDL_CLOSE|TIMER_FLAG_NO_MAPCHANGE);
}

public Action RunPawnFunc(Handle t, DataPack pack) {
	pack.Reset();
	Function fn = pack.ReadFunction();
	Call_StartFunction(null, fn);
	
	int len = pack.ReadCell();
	bool as_array = pack.ReadCell();
	if( as_array ) {
		any[] parms = new any[len];
		for( int i; i < len; i++ ) {
			parms[i] = pack.ReadCell();
		}
		Call_PushArray(parms, len);
	} else {
		for( int i; i < len; i++ ) {
			Call_PushCell(pack.ReadCell());
		}
	}
	Call_Finish();
	return Plugin_Continue;
}


stock void PawnAwait(Function func, float thinktime=0.1, const any[] args, const int len, bool as_array=false) {
	DataPack thinkpack = new DataPack();
	thinkpack.WriteFunction(func);
	thinkpack.WriteCell(len);
	thinkpack.WriteCell(as_array);
	for( int i; i < len; i++ ) {
		thinkpack.WriteCell(args[i]);
	}
	CreateTimer(thinktime, RunPawnAwait, thinkpack, TIMER_DATA_HNDL_CLOSE|TIMER_REPEAT);
}

public Action RunPawnAwait(Handle t, DataPack pack) {
	pack.Reset();
	Function fn = pack.ReadFunction();
	Call_StartFunction(null, fn);
	
	int len = pack.ReadCell();
	bool as_array = pack.ReadCell();
	if( as_array ) {
		any[] parms = new any[len];
		for( int i; i < len; i++ ) {
			parms[i] = pack.ReadCell();
		}
		Call_PushArray(parms, len);
	} else {
		for( int i; i < len; i++ ) {
			Call_PushCell(pack.ReadCell());
		}
	}
	bool result; Call_Finish(result);
	return result? Plugin_Stop : Plugin_Continue;
}


stock bool CheckDownload(const char[] file) {
	if( FileExists(file, true) ) {
		AddFileToDownloadsTable(file);
		return true;
	}
	LogError("CheckDownload :: File not found: '%s'", file);
	return false;
}

stock void PrepareSound(const char[] sound_path) {
	PrecacheSound(sound_path, true);
	char s[OS_MAX_PATH];
	Format(s, sizeof(s), "sound/%s", sound_path);
	CheckDownload(s);
}

stock void DownloadSoundList(const char[][] file_list, int size) {
	for( int i; i < size; i++ ) {
		PrepareSound(file_list[i]);
	}
}

stock void PrecacheSoundList(const char[][] file_list, int size) {
	for( int i; i < size; i++ ) {
		PrecacheSound(file_list[i], true);
	}
}

stock void PrecacheScriptList(const char[][] file_list, int size) {
	for( int i; i < size; i++ ) {
		PrecacheScriptSound(file_list[i]);
	}
}

/// For single custom materials, omit file extensions as it prepares VMT + VTF
stock void PrepareMaterial(const char[] matpath) {
	char s[OS_MAX_PATH];
	Format(s, sizeof(s), "%s%s", matpath, ".vtf");
	CheckDownload(s);
	Format(s, sizeof(s), "%s%s", matpath, ".vmt");
	CheckDownload(s);
}

stock void DownloadMaterialList(const char[][] file_list, int size) {
	char s[OS_MAX_PATH];
	for( int i; i < size; i++ ) {
		strcopy(s, sizeof(s), file_list[i]);
		CheckDownload(s);
	}
}

/// For custom models, do NOT omit .MDL extension
stock int PrepareModel(const char[] model_path, bool model_only=false) {
	char extensions[][] = { ".mdl", ".dx80.vtx", ".dx90.vtx", ".sw.vtx", ".vvd", ".phy" };
	char model_base[OS_MAX_PATH];
	char path[OS_MAX_PATH];
	
	strcopy(model_base, sizeof(model_base), model_path);
	SplitString(model_base, ".mdl", model_base, sizeof(model_base)); /// Kind of redundant, but eh.
	if( !model_only ) {
		for( int i; i < sizeof(extensions); i++ ) {
			Format(path, OS_MAX_PATH, "%s%s", model_base, extensions[i]);
			CheckDownload(path);
		}
	} else {
		CheckDownload(model_path);
	}
	return PrecacheModel(model_path, true);
}


/// map[string]bool | unordered_map< string, bool >
/*
methodmap StringSet < StringMap {
	public StringSet() {
		return view_as< StringSet >( new StringMap() );
	}
	
	property StringMap AsStringMap {
		public get() { return view_as< StringMap >(this); }
	}
	
	public bool HasString(const char[] key) {
		return this.ContainsKey(key);
	}
	
	public bool AddString(const char[] key) {
		return this.SetValue(key, true);
	}
	
	public bool RemoveString(const char[] key) {
		return this.Remove(key);
	}
	
	public bool HasCell(any key) {
		char key_str[CELL_KEY_SIZE];
		PackCellToStr(key, key_str);
		return this.ContainsKey(key_str);
	}
	
	public bool AddCell(any key) {
		char key_str[CELL_KEY_SIZE];
		PackCellToStr(key, key_str);
		return this.SetValue(key_str, true);
	}
	
	public bool RemoveCell(any key) {
		char key_str[CELL_KEY_SIZE];
		PackCellToStr(key, key_str);
		return this.Remove(key_str);
	}
}


methodmap CellMap < StringMap {
	public CellMap() {
		return view_as< CellMap >( new StringMap() );
	}
	
	property StringMap AsStringMap {
		public get() { return view_as< StringMap >(this); }
	}
	
	public CellMap Clone() {
		return this.AsStringMap.Clone();
	}
	
	public bool SetValue(any key, any value, bool replace=true) {
		char key_str[CELL_KEY_SIZE];
		PackCellToStr(key, key_str);
		return this.AsStringMap.SetValue(key_str, value, replace);
	}
	
	public bool SetArray(any key, const any[] array, int num_items, bool replace=true) {
		char key_str[CELL_KEY_SIZE];
		PackCellToStr(key, key_str);
		return this.AsStringMap.SetArray(key_str, array, num_items, replace);
	}
	
	public bool SetString(any key, const char[] value, bool replace=true) {
		char key_str[CELL_KEY_SIZE];
		PackCellToStr(key, key_str);
		return this.AsStringMap.SetString(key_str, value, replace);
	}
	
	public bool GetValue(any key, any &value) {
		char key_str[CELL_KEY_SIZE];
		PackCellToStr(key, key_str);
		return this.AsStringMap.GetValue(key_str, value);
	}
	
	public bool GetArray(any key, any[] array, int max_size, int &size=0) {
		char key_str[CELL_KEY_SIZE];
		PackCellToStr(key, key_str);
		return this.AsStringMap.GetArray(key_str, array, max_size, size);
	}
	
	public bool GetString(any key, char[] value, int max_size, int &size=0) {
		char key_str[CELL_KEY_SIZE];
		PackCellToStr(key, key_str);
		return this.AsStringMap.GetString(key_str, value, max_size, size);
	}
	
	public bool ContainsKey(any key) {
		char key_str[CELL_KEY_SIZE];
		PackCellToStr(key, key_str);
		return this.AsStringMap.ContainsKey(key_str);
	}
	
	public bool Remove(any key) {
		char key_str[CELL_KEY_SIZE];
		PackCellToStr(key, key_str);
		return this.AsStringMap.Remove(key_str);
	}
}
*/


/// destroys a StringMap of handle types.
stock void DestroyMapOfHandles(StringMap &strmap) {
	if( strmap==null ) {
		return;
	}
	
	StringMapSnapshot snap = strmap.Snapshot();
	if( snap==null ) {
		return;
	}
	
	int len = snap.Length;
	for( int i; i < len; i++ ) {
		int keysize = snap.KeyBufferSize(i) + 1;
		char[] key = new char[keysize];
		snap.GetKey(i, key, keysize);
		
		Handle a; strmap.GetValue(key, a);
		delete a;
	}
	delete snap;
	delete strmap;
}

stock int highest_bit_index(int n) {
	int bit_index = 1;
	for( int i = n >>> 1; i > 0; i >>>= 1 ) {
		bit_index++;
	}
	return bit_index - 1;
}

stock bool IsValidPlugin(Handle plugin) {
	PluginIterator iter = new PluginIterator();
	bool res = false;
	while( iter.Next() && !res ) {
		res = iter.Plugin==plugin;
	}
	delete iter;
	return res;
}

stock bool UpdatePluginHandle(Handle &plugin, const char[] name) {
	PluginIterator iter = new PluginIterator();
	bool res = false;
	while( iter.Next() && !res ) {
		Handle pl = iter.Plugin;
		if( plugin==pl ) {
			/// plugin is valid, no need to update.
			res = true;
			break;
		}
		
		char pl_name[PLATFORM_MAX_PATH];
		GetPluginFilename(pl, pl_name, sizeof(pl_name));
		if( !StrEqual(name, pl_name) ) {
			/// not the same.
			continue;
		}
		plugin = pl;
		res = true;
	}
	delete iter;
	return res;
}


stock int int_hash(int i, int seed) {
	int h = seed;
	h =          (i & 0xFF) + (h << 6) + (h << 16) - h;
	h = ((i >>> 8)  & 0xFF) + (h << 6) + (h << 16) - h;
	h = ((i >>> 16) & 0xFF) + (h << 6) + (h << 16) - h;
	h = ((i >>> 24) & 0xFF) + (h << 6) + (h << 16) - h;
	//for( int n=0; n < 32; n += 8 ) {
	//	h = ((i >> n) & 0xFF) + (h << 6) + (h << 16) - h;
	//}
	return h;
}


/// x < y
stock bool UInt32LT(int x, int y) {
	int sign_x = x & 0x80000000;
	int sign_y = y & 0x80000000;
	/// lower negative = higher positive.
	return( sign_x==sign_y )? x < y : ~sign_x && sign_y;
}

/// x > y
stock bool UInt32GT(int x, int y) {
	return UInt32LT(y, x);
}

/// x >= y
stock bool UInt32GE(int x, int y) {
	int sign_x = x & 0x80000000;
	int sign_y = y & 0x80000000;
	return( sign_x==sign_y )? x >= y : sign_x && ~sign_y;
}

/// x <= y
stock bool UInt32LE(int x, int y) {
	return UInt32GE(y, x);
}
