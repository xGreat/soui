#include <SApp.h>



BOOL ExpLua_App(lua_State *L)
{
	try{
		lua_tinker::class_add<SApplication>(L,"SApplication");
		lua_tinker::class_def<SApplication>(L,"AddResProvider",&SApplication::AddResProvider);
		lua_tinker::class_def<SApplication>(L,"RemoveResProvider",&SApplication::RemoveResProvider);
		lua_tinker::class_def<SApplication>(L,"Init",&SApplication::Init);
		lua_tinker::class_def<SApplication>(L,"GetInstance",&SApplication::GetInstance);
		lua_tinker::class_def<SApplication>(L,"GetScriptModule",&SApplication::GetScriptModule);
		lua_tinker::class_def<SApplication>(L,"SetScriptModule",&SApplication::SetScriptModule);
		lua_tinker::class_def<SApplication>(L,"GetTranslator",&SApplication::GetTranslator);
        lua_tinker::class_def<SApplication>(L,"SetTranslator",&SApplication::SetTranslator);
		lua_tinker::def(L,"theApp",&SApplication::getSingletonPtr);

		return TRUE;
	}catch(...)
	{
		return FALSE;
	}
}