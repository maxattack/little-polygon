using System;
using System.Runtime.InteropServices;

namespace LittlePolygon {

	public static class FFI {
		[DllImport("__Internal")] public static extern int CreateContext(string caption, int w, int h, string assetPath);
		[DllImport("__Internal")] public static extern int DestroyContext();
		[DllImport("__Internal")] public static extern void ClearScreen();
	}

}