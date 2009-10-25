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

var WshShell = WScript.CreateObject("WScript.Shell");

var tmpExec;

WScript.Echo("#define __TBB_VERSION_STRINGS \\");

//Getting BUILD_HOST
WScript.echo( "\"TBB: BUILD_HOST\\t\\t" + 
			  WshShell.ExpandEnvironmentStrings("%COMPUTERNAME%") +
			  "\" ENDL \\" );

//Getting BUILD_OS
tmpExec = WshShell.Exec("cmd /c ver");
while ( tmpExec.Status == 0 ) {
	WScript.Sleep(100);
}
tmpExec.StdOut.ReadLine();

WScript.echo( "\"TBB: BUILD_OS\\t\\t" + 
			  tmpExec.StdOut.ReadLine() +
			  "\" ENDL \\" );

var Unknown = "Unknown";

WScript.echo( "\"TBB: BUILD_KERNEL\\t" + 
              Unknown +
              "\" ENDL \\" );

//Getting BUILD_COMPILER
tmpExec = WshShell.Exec("icc --version");
while ( tmpExec.Status == 0 ) {
	WScript.Sleep(100);
}
var ccVersion = tmpExec.StdErr.ReadLine();
WScript.echo( "\"TBB: BUILD_GCC\\t" + 
              ccVersion +
              "\" ENDL \\" );
WScript.echo( "\"TBB: BUILD_COMPILER\\t" + 
              ccVersion +
              "\" ENDL \\" );

WScript.echo( "\"TBB: BUILD_GLIBC\\t" + 
              Unknown +
              "\" ENDL \\" );

WScript.echo( "\"TBB: BUILD_LD\\t" + 
              Unknown +
              "\" ENDL \\" );

//Getting BUILD_TARGET
WScript.echo( "\"TBB: BUILD_TARGET\\t" + 
			  WScript.Arguments(1) + 
			  "\" ENDL \\" );

//Getting BUILD_COMMAND
WScript.echo( "\"TBB: BUILD_COMMAND\\t" + WScript.Arguments(2) + "\" ENDL" );

//Getting __TBB_DATETIME and __TBB_VERSION_YMD
var date = new Date();
WScript.echo( "#define __TBB_DATETIME \"" + date.toUTCString() + "\"" );
WScript.echo( "#define __TBB_VERSION_YMD " + date.getUTCFullYear() + ", " + 
			  (date.getUTCMonth() > 8 ? (date.getUTCMonth()+1):("0"+(date.getUTCMonth()+1))) + 
			  (date.getUTCDate() > 9 ? date.getUTCDate():("0"+date.getUTCDate())) );


