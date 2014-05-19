#include <littlepolygon/context.h>
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <string>

int main(int argc, char *argv[]) {


	// RESOLVE DLL TO SDL'S RESOURCE DIR
	auto basePath = SDL_GetBasePath();
	char dllPath[128];
	SDL_strlcpy(dllPath, basePath, sizeof(dllPath));
	auto dllName = argc > 1 ? argv[1] : "demo.dll";
	SDL_strlcat(dllPath, dllName, sizeof(dllPath));
	SDL_free(basePath);

	// RUN C# CODE
	auto domain = mono_jit_init("Little Polygon");
	auto assembly = mono_domain_assembly_open(domain, dllPath);
	if (!assembly) {
		LOG(("DLL Not Found: %s\n", dllName));
	} else {
		const char *monoArgs[] = { dllPath, "--gc=sgen" };
		mono_jit_exec(domain, assembly, 2, (char**) monoArgs);
	}
	mono_jit_cleanup(domain);
	
	return 0;

}
