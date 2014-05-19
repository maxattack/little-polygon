using System;
using System.Runtime.InteropServices;
using System.Security;

static class Game {

	static void Main(string[] args) {
		sayHello();
	}

	[SuppressUnmanagedCodeSecurity]
	[DllImport("__Internal")]
	static unsafe extern void sayHello();

}
