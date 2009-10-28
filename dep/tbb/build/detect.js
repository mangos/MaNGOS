// Copyright 2005-2009 Intel Corporation.  All Rights Reserved.
//
// This file is part of Threading Building Blocks.
//
// Threading Building Blocks is free software; you can redistribute it
// and/or modify it under the terms of the GNU General Public License
// version 2 as published by the Free Software Foundation.
//
// Threading Building Blocks is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Threading Building Blocks; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
// As a special exception, you may use this file as part of a free software
// library without restriction.  Specifically, if other files instantiate
// templates or use macros or inline functions from this file, or you compile
// this file and link it with other files to produce an executable, this
// file does not by itself cause the resulting executable to be covered by
// the GNU General Public License.  This exception does not however
// invalidate any other reasons why the executable file might be covered by
// the GNU General Public License.

function doWork() {
		var WshShell = WScript.CreateObject("WScript.Shell");

		var fso = new ActiveXObject("Scripting.FileSystemObject");

		var tmpExec;

		if ( WScript.Arguments.Count() > 1 && WScript.Arguments(1) == "gcc" ) {
			if ( WScript.Arguments(0) == "/arch" ) {
				WScript.Echo( "ia32" );
			}
			else if ( WScript.Arguments(0) == "/runtime" ) {
				WScript.Echo( "mingw" );
			}
			return;
		}

		//Compile binary
		tmpExec = WshShell.Exec("cmd /c echo int main(){return 0;} >detect.c");
		while ( tmpExec.Status == 0 ) {
			WScript.Sleep(100);
		}
		
		tmpExec = WshShell.Exec("cl /MD detect.c /link /MAP");
		while ( tmpExec.Status == 0 ) {
			WScript.Sleep(100);
		}

		if ( WScript.Arguments(0) == "/arch" ) {
			//read compiler banner
			var clVersion = tmpExec.StdErr.ReadAll();
			
			//detect target architecture
			var intel64=/AMD64|EM64T|x64/mgi;
			var ia64=/IA-64|Itanium/mgi;
			var ia32=/80x86/mgi;
			if ( clVersion.match(intel64) ) {
				WScript.Echo( "intel64" );
			} else if ( clVersion.match(ia64) ) {
				WScript.Echo( "ia64" );
			} else if ( clVersion.match(ia32) ) {
				WScript.Echo( "ia32" );
			} else {
				WScript.Echo( "unknown" );
			}
		}

		if ( WScript.Arguments(0) == "/runtime" ) {
			//read map-file
			var map = fso.OpenTextFile("detect.map", 1, 0);
			var mapContext = map.readAll();
			map.Close();
			
			//detect runtime
			var vc71=/MSVCR71\.DLL/mgi;
			var vc80=/MSVCR80\.DLL/mgi;
			var vc90=/MSVCR90\.DLL/mgi;
			var vc100=/MSVCR100\.DLL/mgi;
			var psdk=/MSVCRT\.DLL/mgi;
			if ( mapContext.match(vc71) ) {
				WScript.Echo( "vc7.1" );
			} else if ( mapContext.match(vc80) ) {
				WScript.Echo( "vc8" );
			} else if ( mapContext.match(vc90) ) {
				WScript.Echo( "vc9" );
			} else if ( mapContext.match(vc100) ) {
				WScript.Echo( "vc10" );
			} else if ( mapContext.match(psdk) ) {
				// Our current naming convention assumes vc7.1 for 64-bit Windows PSDK
				WScript.Echo( "vc7.1" ); 
			} else {
				WScript.Echo( "unknown" );
			}
		}

		// delete intermediate files
		if ( fso.FileExists("detect.c") )
			fso.DeleteFile ("detect.c", false);
		if ( fso.FileExists("detect.obj") )
			fso.DeleteFile ("detect.obj", false);
		if ( fso.FileExists("detect.map") )
			fso.DeleteFile ("detect.map", false);
		if ( fso.FileExists("detect.exe") )
			fso.DeleteFile ("detect.exe", false);
		if ( fso.FileExists("detect.exe.manifest") )
			fso.DeleteFile ("detect.exe.manifest", false);
}

if ( WScript.Arguments.Count() > 0 ) {
	
	try {
		doWork();
	} catch( error )
	{
		WScript.Echo( "unknown" );
		WScript.Quit( 0 );
	}

} else {

	WScript.Echo( "/arch or /runtime should be set" );
}

